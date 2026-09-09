// 安全模拟朴素延迟释放/ABA 的反例，不在默认实验中真的解引用悬垂指针。
#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <iostream>
#include <memory>

static void delay_is_not_protection() {
    // 有限状态调度：R 已取到地址并暂停，W 摘除、等任意有限轮数再释放。
    bool reader_has_address = true;
    bool storage_alive = true;
    constexpr int guessed_delay = 100;
    for (int tick = 0; tick < guessed_delay; ++tick) {
        cs::check(reader_has_address, "scheduler keeps reader paused");
    }
    storage_alive = false;
    cs::check(reader_has_address && !storage_alive, "finite delay cannot prove lifetime");
    // 故意不执行读者的解引用。这里验证时序可达，未运行 UB。
}

static void reference_count_branch() {
    int destroyed = 0;
    struct value {
        int* destroyed;
        int payload;
        ~value() { ++*destroyed; }
    };
    std::atomic<std::shared_ptr<const value>> current;
    current.store(std::shared_ptr<const value>(new value{&destroyed, 42}));
    auto owned = current.load(); // 获取强引用是 atomic<shared_ptr> 的组成部分。
    std::weak_ptr<const value> witness = owned;
    current.store({});
    cs::check(owned->payload == 42 && destroyed == 0, "strong reference lost lifetime");
    owned.reset();
    cs::check(destroyed == 1 && witness.expired(), "last strong reference did not destroy");
    cs::check(!witness.lock(), "weak_ptr resurrected destroyed object");
    std::cout << "atomic<shared_ptr>.is_lock_free=" << current.is_lock_free() << '\n';
}

static void aba_branch() {
    struct tagged { unsigned address, generation; };
    // address 是模型中的地址编号，不是可解引用的指针。
    const tagged saved{7, 1};
    tagged head{8, 2}; // A -> B
    head = {7, 3};    // B -> A，同地址但不同逻辑版本。
    cs::check(saved.address == head.address, "plain CAS would accept ABA");
    cs::check(saved.generation != head.generation, "tag should reject this ABA");
    bool old_lifetime_ended = true;
    cs::check(old_lifetime_ended, "tag never promised lifetime");
    // 2 位标签的有限状态回绕，不靠整数 UB。
    unsigned tag = 1;
    for (int i = 0; i < 4; ++i) tag = (tag + 1) % 4;
    cs::check(tag == 1, "finite tags can wrap");
}

int main() {
    delay_is_not_protection();
    reference_count_branch();
    aba_branch();
    std::cout << "delay model, strong ownership, ABA/tag model PASS\n";
}
