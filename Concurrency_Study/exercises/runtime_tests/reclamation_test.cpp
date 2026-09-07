#include "concurrency_study/exercise_check.hpp"
#include "concurrency_study/hazard_pointer.hpp"
#include "concurrency_study/rcu.hpp"
#include "../I2_hazard_pointer/reference.hpp"
#include "../I3_rcu/reference.hpp"
#include "../R1_epoch_reclamation/reference.hpp"
#include "../R2_qsbr/reference.hpp"
#include <chrono>
#include <array>
#include <future>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>
using namespace std::chrono_literals;
namespace lab = reclamation_experiment;

struct hp_node : cs::hazard_pointer_obj_base<hp_node,
                                             std::move_only_function<void(hp_node*)>> {
    int value = 42;
};

static void hp_handles_and_exit() {
    int destroyed = 0;
    auto del = [&](hp_node* p) { ++destroyed; delete p; };
    auto* a = new hp_node;
    auto* b = new hp_node;
    std::atomic<hp_node*> source_a{a}, source_b{b};
    auto first = cs::make_hazard_pointer();
    auto second = cs::make_hazard_pointer();
    cs::check(first.protect(source_a) == a && second.protect(source_b) == b, "HP protect");
    auto writer = std::async(std::launch::async, [&] {
        source_a.exchange(nullptr)->retire(del);
        source_b.exchange(nullptr)->retire(del);
    });
    writer.get(); // 原实现的 TLS 退休列表在这里丢失。
    cs::check(cs::hazard_pointer_cleanup() == 0 && destroyed == 0, "HP early deletion");
    first = std::move(second); // 解除 a，转移 b；不能清掉转入槽的保护。
    cs::check(second.empty() && !first.empty(), "HP move assignment");
    second.reset_protection(); // 空句柄允许 reset。
    cs::check(cs::hazard_pointer_cleanup() == 1 && destroyed == 1, "HP move released wrong node");
    cs::check(b->value == 42, "HP moved protection lost");
    auto third = std::move(first);
    cs::check(first.empty(), "HP move constructor");
    auto& alias = third;
    third = std::move(alias); // 自移动不能取消保护。
    cs::check(cs::hazard_pointer_cleanup() == 0, "HP self move lost protection");
    third.reset_protection();
    cs::check(!third.empty(), "reset must retain active slot");
    cs::check(cs::hazard_pointer_cleanup() == 1 && destroyed == 2, "HP exit leftovers lost");

    auto* c = new hp_node;
    std::atomic<hp_node*> source_c{c};
    hp_node* expected = nullptr;
    cs::check(!third.try_protect(expected, source_c) && expected == c, "try_protect refresh");
    cs::check(third.try_protect(expected, source_c), "try_protect retry");
    source_c.exchange(nullptr)->retire(del);
    cs::check(cs::hazard_pointer_cleanup() == 0, "try_protect did not protect");
    third.reset_protection();
    cs::hazard_pointer_cleanup();
    cs::check(destroyed == 3, "exactly once");

    // 析构释放槽；句柄数有明确上限，reset 不能充当释放槽。
    third = {};
    std::vector<cs::hazard_pointer> handles;
    for (std::size_t i = 0; i < cs::kMaxHazardPointers; ++i)
        handles.push_back(cs::make_hazard_pointer());
    bool exhausted = false;
    try { auto extra = cs::make_hazard_pointer(); } catch (const std::bad_alloc&) { exhausted = true; }
    cs::check(exhausted, "HP slot capacity");
    handles.clear();
    auto reusable = cs::make_hazard_pointer();
    cs::check(!reusable.empty(), "HP slots not reusable");
    auto* d = new hp_node;
    std::atomic<hp_node*> source_d{d};
    cs::check(reusable.protect(source_d) == d, "HP transfer source");
    source_d.exchange(nullptr)->retire(del);
    auto transferred = cs::make_hazard_pointer();
    transferred.reset_protection(d); // 从仍有效的保护转交已退休对象。
    reusable.reset_protection();
    cs::check(cs::hazard_pointer_cleanup() == 0 && d->value == 42, "HP raw reset transfer");
    transferred.reset_protection();
    cs::check(cs::hazard_pointer_cleanup() == 1 && destroyed == 4, "HP transferred cleanup");
}

static void hp_state_and_reentry() {
    int total = 0;
    std::size_t nested_result = 999;
    auto* parent = new hp_node;
    parent->retire([&, weight = std::make_unique<int>(7)](hp_node* p) {
        total += *weight; // 只能移动、有状态、无默认构造的删除器。
        auto* child = new hp_node;
        child->retire([&](hp_node* q) { total += 11; delete q; });
        nested_result = cs::hazard_pointer_cleanup();
        delete p;
    });
    cs::check(cs::hazard_pointer_cleanup() == 2, "HP reentrant child lost");
    cs::check(total == 18 && nested_result == 0, "HP state/reentry contract");
    cs::check(cs::hazard_pointer_cleanup() == 0, "HP double deletion");
}

static void rcu_nested_domains_and_exit() {
    cs::rcu_domain a, b;
    int destroyed_a = 0, destroyed_b = 0;
    auto task = std::async(std::launch::async, [&] {
        cs::rcu_reader outer(a);
        {
            cs::rcu_reader nested(a);
            a.retire(new int(7), [&](int* p) { ++destroyed_a; delete p; });
        }
        cs::check(a.collect() == 0, "inner unlock ended outer protection");
        bool rejected = false;
        try { a.barrier(); } catch (const std::logic_error&) { rejected = true; }
        cs::check(rejected, "same-domain wait must reject self-deadlock");
        b.retire(new int(9), [&](int* p) { ++destroyed_b; delete p; });
        b.barrier(); // 独立域不受 a 的读者阻挡。
        cs::check(destroyed_b == 1 && destroyed_a == 0, "domain isolation");
    });
    task.get();
    // 重复短命线程，覆盖 thread id 重用；域不保存已销毁的 TLS 地址。
    for (int i = 0; i < 32; ++i)
        std::async(std::launch::async, [&] { cs::rcu_reader guard(a); }).get();
    a.synchronize();
    cs::check(destroyed_a == 0, "synchronize unexpectedly ran callbacks");
    a.barrier();
    cs::check(destroyed_a == 1 && a.pending() == 0, "RCU reader exit registration");
    // 默认域和侵入式旧 API 仍可用。
    struct item : cs::rcu_obj_base<item> {};
    { cs::rcu_reader guard; }
    (new item)->retire();
    cs::rcu_synchronize();
    cs::rcu_barrier();
}

static void rcu_state_and_callback_reentry() {
    cs::rcu_domain d;
    int total = 0;
    bool wait_rejected = false;
    cs::rcu_retire(new int(1), [&, weight = std::make_unique<int>(5)](int* p) {
        total += *weight;
        cs::rcu_retire(new int(2), [&](int* q) { total += *q; delete q; }, d);
        cs::check(d.collect() == 0, "callback collect must not recurse");
        try { d.barrier(); } catch (const std::logic_error&) { wait_rejected = true; }
        delete p;
    }, d);
    d.barrier(); // 固定批次，回调中新退休的 child 不属于该批。
    cs::check(total == 5 && d.pending() == 1 && wait_rejected, "RCU callback/batch contract");
    d.barrier();
    cs::check(total == 7 && d.pending() == 0, "RCU stateful deleter lost");
}

// collector A 已取走批次、正在回调时，barrier B 必须等待 A 回调结束。
static void concurrent_barriers(lab::failure fault = lab::failure::none,
                                lab::audit* observed = nullptr) {
    lab::audit local;
    auto& counts = observed ? *observed : local;
    cs::rcu_domain d;
    // 回调与收集 worker 共同持有通知；主线程只等 future。
    auto in_callback = std::make_shared<lab::signal>();
    auto callback_ready = in_callback->get_future();
    lab::signal second_started;
    auto second_ready = second_started.get_future();
    lab::gate finish_callback;
    std::future<void> first;
    std::future<int> second;
    int callback_result = 0;
    const auto finish = [&] {
        finish_callback.open();
        if (first.valid()) first.wait();
        if (second.valid()) second.wait();
        d.barrier(); // first 启动失败时，仍需执行已经退休的回调。
    };
    try {
        d.retire(new lab::value(17, counts), [&, started = in_callback](lab::value* p) {
            started->ready();
            finish_callback.wait();
            callback_result = p->payload;
            delete p;
        });
        first = std::async(std::launch::async, [&, started = std::move(in_callback)] {
            lab::worker_lifetime lifetime(counts);
            try { lab::inject(fault, lab::failure::before_callback); d.barrier(); }
            catch (...) { started->fail(std::current_exception()); throw; }
        });
        callback_ready.get();
        const auto in_flight = d.pending();
        lab::inject(fault, lab::failure::before_secondary_launch);
        second = std::async(std::launch::async, [&, started = std::move(second_started)]() mutable {
            lab::worker_lifetime lifetime(counts);
            try {
                started.ready();
                d.barrier();
                return callback_result;
            } catch (...) { started.fail(std::current_exception()); throw; }
        });
        second_ready.get();
        const bool blocked = second.wait_for(40ms) == std::future_status::timeout;
        finish_callback.open();
        first.get();
        const int result = second.get();
        finish();
        cs::check(blocked && in_flight == 1, "concurrent barrier skipped in-flight callback");
        cs::check(result == 17 && d.pending() == 0, "barrier returned before callback end");
    } catch (...) {
        finish();
        throw;
    }
}
// 宽限期与回调执行之间新退休的对象，不能借用旧批次的宽限期。
static void late_retirement(lab::failure fault = lab::failure::none,
                            lab::audit* observed = nullptr) {
    lab::audit local;
    auto& counts = observed ? *observed : local;
    cs::rcu_domain d;
    auto callback_entered = std::make_shared<lab::signal>();
    auto callback_ready = callback_entered->get_future();
    lab::signal reader_entered;
    auto reader_ready = reader_entered.get_future();
    lab::gate callback_release, reader_release;
    std::atomic<lab::value*> source{nullptr};
    std::atomic<int> old_destroyed{0};
    std::future<void> collector, reader;
    const auto finish = [&] {
        callback_release.open();
        reader_release.open(); // 两个线程的门全部打开，才允许等待任一 future。
        if (collector.valid()) collector.wait();
        if (reader.valid()) reader.wait();
        delete source.exchange(nullptr);
        d.barrier();
    };
    try {
        d.retire(new lab::value(1, counts), [&, started = callback_entered](lab::value* p) {
            started->ready();
            callback_release.wait();
            delete p;
        });
        collector = std::async(std::launch::async, [&, started = std::move(callback_entered)] {
            lab::worker_lifetime lifetime(counts);
            try { lab::inject(fault, lab::failure::before_callback); d.barrier(); }
            catch (...) { started->fail(std::current_exception()); throw; }
        });
        callback_ready.get();
        source.store(new lab::value(42, counts));
        lab::inject(fault, lab::failure::before_secondary_launch);
        reader = std::async(std::launch::async, [&, started = std::move(reader_entered)]() mutable {
            lab::worker_lifetime lifetime(counts);
            try {
                lab::inject(fault, lab::failure::before_registration);
                cs::rcu_reader guard(d);
                auto* p = source.load();
                started.ready();
                reader_release.wait();
                cs::check(p->payload == 42, "late retirement reclaimed with wrong grace period");
            } catch (...) { started.fail(std::current_exception()); throw; }
        });
        reader_ready.get();
        lab::inject(fault, lab::failure::before_allocation);
        auto fresh = std::make_unique<lab::value>(43, counts);
        lab::inject(fault, lab::failure::before_publish);
        d.retire(source.exchange(fresh.release()), [&](lab::value* p) {
            ++old_destroyed;
            delete p;
        });
        callback_release.open();
        collector.get();
        const auto early = d.collect();
        const int deleted_before_exit = old_destroyed.load();
        reader_release.open();
        reader.get();
        finish();
        cs::check(early == 0 && deleted_before_exit == 0, "late retirement used old grace");
        cs::check(old_destroyed == 1, "late retirement not reclaimed exactly once");
    } catch (...) {
        finish();
        throw;
    }
}
static void qsbr_contracts_and_offline() {
    cs::epoch_domain d;
    int destroyed = 0;
    {
        cs::qsbr_participant p(d);
        bool duplicate = false, mixed = false;
        try { cs::qsbr_participant another(d); } catch (const std::logic_error&) { duplicate = true; }
        try { cs::epoch_guard guard(d); } catch (const std::logic_error&) { mixed = true; }
        cs::check(duplicate && mixed, "QSBR domain mixing contract");
        d.retire(new int(1), [&](int* q) { ++destroyed; delete q; });
        cs::check(d.collect() == 0, "online QSBR reader forgotten");
        p.offline();
        d.barrier();
        cs::check(destroyed == 1, "offline did not unblock");
        bool rejected = false;
        try { p.quiescent(); } catch (const std::logic_error&) { rejected = true; }
        cs::check(rejected, "offline quiescent must reject");
        p.online();
        d.retire(new int(2), [&](int* q) { ++destroyed; delete q; });
        p.quiescent();
        cs::check(d.collect() == 1, "online/quiescent cycle");
    }
    d.barrier();
    cs::check(destroyed == 2, "QSBR destructor left registration");
}

static void later_generation_does_not_block() {
    cs::epoch_domain d;
    int destroyed = 0;
    d.retire(new int(3), [&](int* p) { ++destroyed; delete p; });
    {
        cs::epoch_guard later(d); // 退休之后才进入，不能取得旧对象借用。
        cs::check(d.collect() == 1 && destroyed == 1, "new generation blocked old retirement");
    }
    d.barrier();
}

static void synchronize_waits_for_old_reader(lab::failure fault = lab::failure::none,
                                             lab::audit* observed = nullptr) {
    lab::audit local;
    auto& counts = observed ? *observed : local;
    cs::rcu_domain d;
    std::atomic<lab::value*> source{nullptr};
    lab::signal entered, waiting;
    auto reader_ready = entered.get_future();
    auto sync_ready = waiting.get_future();
    lab::gate release;
    std::future<void> reader, synchronizer;
    int destroyed = 0;
    const auto finish = [&] {
        release.open();
        if (reader.valid()) reader.wait();
        if (synchronizer.valid()) synchronizer.wait();
        delete source.exchange(nullptr);
        d.barrier();
    };
    try {
        source.store(new lab::value(42, counts));
        reader = std::async(std::launch::async, [&, started = std::move(entered)]() mutable {
            lab::worker_lifetime lifetime(counts);
            try {
                lab::inject(fault, lab::failure::before_registration);
                cs::rcu_reader guard(d);
                auto* p = source.load();
                started.ready();
                release.wait();
                cs::check(p->payload == 42, "long RCU reader lost protection");
            } catch (...) { started.fail(std::current_exception()); throw; }
        });
        reader_ready.get();
        lab::inject(fault, lab::failure::before_allocation);
        auto fresh = std::make_unique<lab::value>(43, counts);
        lab::inject(fault, lab::failure::before_publish);
        d.retire(source.exchange(fresh.release()), [&](lab::value* p) { ++destroyed; delete p; });
        lab::inject(fault, lab::failure::before_secondary_launch);
        synchronizer = std::async(std::launch::async, [&, started = std::move(waiting)]() mutable {
            lab::worker_lifetime lifetime(counts);
            try {
                started.ready();
                d.synchronize();
            } catch (...) { started.fail(std::current_exception()); throw; }
        });
        sync_ready.get();
        const bool blocked = synchronizer.wait_for(40ms) == std::future_status::timeout;
        release.open();
        reader.get();
        synchronizer.get();
        cs::check(blocked && destroyed == 0, "synchronize wait/callback separation");
        finish();
        cs::check(destroyed == 1, "long RCU reader final cleanup");
    } catch (...) {
        finish();
        throw;
    }
}
static void failure_paths() {
    using failure = lab::failure;
    struct scenario {
        const char* name;
        void (*run)(failure, lab::audit*);
        failure point;
        int objects, workers;
    };
    const std::array cases{
        scenario{"R1 registration", ebr_exercise::run, failure::before_registration, 1, 1},
        scenario{"R1 allocation after 8 retires", ebr_exercise::run, failure::before_allocation, 9, 1},
        scenario{"R1 unpublished value", ebr_exercise::run, failure::before_publish, 10, 1},
        scenario{"R2 registration", qsbr_exercise::run, failure::before_registration, 1, 1},
        scenario{"R2 allocation after 8 retires", qsbr_exercise::run, failure::before_allocation, 9, 1},
        scenario{"R2 unpublished value", qsbr_exercise::run, failure::before_publish, 10, 1},
        scenario{"R2 report", qsbr_exercise::run, failure::before_report, 65, 1},
        scenario{"I3 registration", rcu_exercise::run, failure::before_registration, 65, 4},
        scenario{"I3 allocation after 8 retires", rcu_exercise::run, failure::before_allocation, 9, 4},
        scenario{"I3 unpublished value", rcu_exercise::run, failure::before_publish, 10, 4},
        scenario{"barrier before callback", concurrent_barriers, failure::before_callback, 1, 1},
        scenario{"barrier second launch", concurrent_barriers, failure::before_secondary_launch, 1, 1},
        scenario{"late before callback", late_retirement, failure::before_callback, 1, 1},
        scenario{"late reader registration", late_retirement, failure::before_registration, 2, 2},
        scenario{"late reader launch", late_retirement, failure::before_secondary_launch, 2, 1},
        scenario{"late allocation", late_retirement, failure::before_allocation, 2, 2},
        scenario{"late unpublished value", late_retirement, failure::before_publish, 3, 2},
        scenario{"synchronize registration", synchronize_waits_for_old_reader, failure::before_registration, 1, 1},
        scenario{"synchronize allocation", synchronize_waits_for_old_reader, failure::before_allocation, 1, 1},
        scenario{"synchronize unpublished value", synchronize_waits_for_old_reader, failure::before_publish, 2, 1},
        scenario{"synchronize second launch", synchronize_waits_for_old_reader, failure::before_secondary_launch, 2, 1}
    };
    for (const auto& c : cases) {
        std::cout << "inject: " << c.name << std::endl; // 外部超时日志能定位未结束场景。
        lab::audit counts;
        bool caught_original = false;
        const auto begin = std::chrono::steady_clock::now();
        try { c.run(c.point, &counts); }
        catch (const lab::injected_bad_alloc& error) {
            caught_original = error.point == c.point
                && std::string_view(error.what()) == "reclamation injected bad_alloc";
            std::cout << "  caught original: " << error.what() << '\n';
        }
        cs::check(caught_original, "original injected exception was lost/replaced");
        cs::check(counts.started == c.workers && counts.finished == c.workers,
                  "a launched worker was not joined before propagation");
        cs::check(counts.created == c.objects && counts.destroyed == c.objects,
                  "published/retired/unpublished ownership remains");
        cs::check(std::chrono::steady_clock::now() - begin < 5s, "fault cleanup took too long");
        // 域已正常析构，亦检查了空读者表和零 outstanding；不是只检查计数。
    }
    std::cout << "21 injected failure paths: original exceptions, joins, exact destruction PASS\n";
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--failure-paths") {
        failure_paths();
        return 0;
    }
    cs::check(argc == 1, "usage: reclamation_test [--failure-paths]");
    hp_handles_and_exit();
    hp_state_and_reentry();
    rcu_nested_domains_and_exit();
    rcu_state_and_callback_reentry();
    concurrent_barriers();
    late_retirement();
    qsbr_contracts_and_offline();
    later_generation_does_not_block();
    synchronize_waits_for_old_reader();
    hp_exercise::run();
    rcu_exercise::run();
    ebr_exercise::run();
    qsbr_exercise::run();
    failure_paths();
    std::cout << "reclamation runtime checks PASS\n";
}
