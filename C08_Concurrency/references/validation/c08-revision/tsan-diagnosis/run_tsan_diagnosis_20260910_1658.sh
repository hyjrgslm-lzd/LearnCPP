#!/usr/bin/env bash
set -u

work_dir="/root/learncpp-c08-builds/tsan-diagnosis-20260910-1658"
repo_src="/root/learncpp-c08-src/baseline-20260910-162842-fc6bb723"
tsan_build="/root/learncpp-c08-builds/baseline-20260910-162842-fc6bb723/linux-tsan"
repo_report="/mnt/f/CPPTrain/LearnCPP/C08_Concurrency/references/validation/c08-revision/tsan-diagnosis/agent-run-20260910-1658"

mkdir -p "$work_dir" "$repo_report"
cd "$work_dir" || exit 2

cat > call_once_probe.cpp <<'CPP'
#include <atomic>
#include <cassert>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

void retry_body() {
    std::once_flag flag;
    int attempts = 0;
    std::unique_ptr<int> value;
    for (;;) {
        try {
            std::call_once(flag, [&] {
                if (++attempts == 1) throw std::runtime_error("retry");
                value = std::make_unique<int>(123);
            });
            break;
        } catch (const std::runtime_error&) {}
    }
    assert(attempts == 2);
    assert(*value == 123);
}

void no_throw_body() {
    std::once_flag flag;
    int attempts = 0;
    int value = 0;
    std::call_once(flag, [&] { ++attempts; value = 123; });
    std::call_once(flag, [&] { assert(false); });
    assert(attempts == 1);
    assert(value == 123);
}

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "same-thread-retry";
    if (mode == "same-thread-retry") retry_body();
    else if (mode == "thread-retry") {
        std::thread t(retry_body);
        t.join();
    } else if (mode == "async-retry") {
        auto f = std::async(std::launch::async, retry_body);
        f.get();
    } else if (mode == "async-8-retry") {
        std::once_flag flag;
        std::unique_ptr<int> value;
        int attempts = 0;
        std::atomic<int> failures{0};
        std::vector<std::future<void>> fs;
        for (int i = 0; i < 8; ++i) {
            fs.push_back(std::async(std::launch::async, [&] {
                for (;;) {
                    try {
                        std::call_once(flag, [&] {
                            if (++attempts == 1) throw std::runtime_error("retry");
                            value = std::make_unique<int>(123);
                        });
                        break;
                    } catch (const std::runtime_error&) { ++failures; }
                }
                assert(*value == 123);
            }));
        }
        for (auto& f : fs) f.get();
        assert(attempts == 2);
        assert(failures == 1);
    } else if (mode == "async-8-control") {
        std::once_flag flag;
        std::unique_ptr<int> value;
        int attempts = 0;
        std::vector<std::future<void>> fs;
        for (int i = 0; i < 8; ++i) {
            fs.push_back(std::async(std::launch::async, [&] {
                std::call_once(flag, [&] {
                    ++attempts;
                    value = std::make_unique<int>(123);
                });
                assert(*value == 123);
            }));
        }
        for (auto& f : fs) f.get();
        assert(attempts == 1);
    } else {
        no_throw_body();
    }
    std::cout << "ok " << mode << '\n';
}
CPP

cat > fence_probe.cpp <<'CPP'
#include <atomic>
#include <cassert>
#include <future>
#include <iostream>

void fence_publication(int form) {
    int data = 0;
    std::atomic<bool> ready{false};
    auto producer = std::async(std::launch::async, [&] {
        data = 42;
        if (form != 1) std::atomic_thread_fence(std::memory_order_release);
        ready.store(true, form == 1 ? std::memory_order_release : std::memory_order_relaxed);
        ready.notify_one();
    });
    ready.wait(false, form == 0 ? std::memory_order_acquire : std::memory_order_relaxed);
    if (form != 0) std::atomic_thread_fence(std::memory_order_acquire);
    int observed = data;
    producer.get();
    assert(observed == 42);
}

void atomic_control() {
    int data = 0;
    std::atomic<bool> ready{false};
    auto producer = std::async(std::launch::async, [&] {
        data = 42;
        ready.store(true, std::memory_order_release);
        ready.notify_one();
    });
    ready.wait(false, std::memory_order_acquire);
    int observed = data;
    producer.get();
    assert(observed == 42);
}

int main(int argc, char** argv) {
    std::string mode = argc > 1 ? argv[1] : "fence0";
    if (mode == "atomic") atomic_control();
    else if (mode == "fence0") fence_publication(0);
    else if (mode == "fence1") fence_publication(1);
    else if (mode == "fence2") fence_publication(2);
    else assert(false);
    std::cout << "ok " << mode << '\n';
}
CPP

cat > global_new_probe.cpp <<'CPP'
#include <cstdlib>
#include <iostream>
#include <new>

void* operator new(std::size_t n) {
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}

void operator delete(void* p) noexcept {
    std::free(p);
}

int main() {
    int* p = new int(7);
    std::cout << *p << '\n';
    delete p;
}
CPP

cat > local_alloc_probe.cpp <<'CPP'
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <new>

struct fail_once_resource {
    bool fail = false;

    void* allocate(std::size_t n) {
        if (fail) {
            fail = false;
            throw std::bad_alloc();
        }
        if (void* p = std::malloc(n ? n : 1)) return p;
        throw std::bad_alloc();
    }

    void deallocate(void* p) noexcept {
        std::free(p);
    }
};

int main() {
    fail_once_resource r;
    r.fail = true;
    bool caught = false;
    try {
        void* p = r.allocate(4);
        r.deallocate(p);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    assert(caught);
    void* p = r.allocate(4);
    r.deallocate(p);
    std::cout << "ok local allocation injection\n";
}
CPP

{
    printf 'work_dir=%s\n' "$work_dir"
    printf 'repo_src=%s\n' "$repo_src"
    printf 'tsan_build=%s\n' "$tsan_build"
    clang++-18 --version | head -n 1
    g++ --version | head -n 1
    cmake --version | head -n 1
    ninja --version
} > environment.txt

sha256sum \
    "$repo_src/C08_Concurrency/exercises/B3_call_once/solution.cpp" \
    "$repo_src/C08_Concurrency/exercises/F3_seqcst_fence/solution.cpp" \
    "$repo_src/C08_Concurrency/exercises/runtime_tests/scheduling_test.cpp" \
    > source-hashes.txt

clang++-18 -std=c++23 -O1 -g -fsanitize=thread -fno-omit-frame-pointer call_once_probe.cpp -pthread -o call_once_tsan > build-call-once-tsan.txt 2>&1
clang++-18 -std=c++23 -O1 -g call_once_probe.cpp -pthread -o call_once_plain > build-call-once-plain.txt 2>&1
clang++-18 -std=c++23 -O1 -g -fsanitize=thread -fno-omit-frame-pointer fence_probe.cpp -pthread -o fence_tsan > build-fence-tsan.txt 2>&1
clang++-18 -std=c++23 -O1 -g fence_probe.cpp -pthread -o fence_plain > build-fence-plain.txt 2>&1
clang++-18 -std=c++23 -O1 -g global_new_probe.cpp -pthread -o global_new_plain > build-global-new-plain.txt 2>&1
clang++-18 -std=c++23 -O1 -g -fsanitize=thread -fno-omit-frame-pointer global_new_probe.cpp -pthread -o global_new_tsan > build-global-new-tsan.txt 2>&1
printf '%s\n' "$?" > status-build-global-new-tsan.txt
clang++-18 -std=c++23 -O1 -g -fsanitize=thread -fno-omit-frame-pointer local_alloc_probe.cpp -pthread -o local_alloc_tsan > build-local-alloc-tsan.txt 2>&1

run_case() {
    label="$1"
    shift
    /usr/bin/timeout 8 "$@" > "run-${label}.txt" 2>&1
    printf '%s\n' "$?" > "status-${label}.txt"
}

for mode in same-thread-retry thread-retry async-retry async-8-retry async-8-control no-throw; do
    run_case "call-once-tsan-${mode}" ./call_once_tsan "$mode"
done

for mode in same-thread-retry async-8-retry async-8-control; do
    run_case "call-once-plain-${mode}" ./call_once_plain "$mode"
done

for mode in atomic fence0 fence1 fence2; do
    run_case "fence-tsan-${mode}" ./fence_tsan "$mode"
done

for mode in atomic fence0 fence1 fence2; do
    run_case "fence-plain-${mode}" ./fence_plain "$mode"
done

run_case "global-new-plain" ./global_new_plain
if [ -x ./global_new_tsan ]; then
    run_case "global-new-tsan" ./global_new_tsan
else
    printf 'not-built\n' > status-global-new-tsan.txt
fi
run_case "local-alloc-tsan" ./local_alloc_tsan

if [ -x "$tsan_build/B3_call_once/B3_call_once_reference" ]; then
    /usr/bin/timeout 15 "$tsan_build/B3_call_once/B3_call_once_reference" > run-existing-B3-tsan.txt 2>&1
    printf '%s\n' "$?" > status-existing-B3-tsan.txt
fi

if [ -x "$tsan_build/F3_seqcst_fence/F3_seqcst_fence_reference" ]; then
    /usr/bin/timeout 15 "$tsan_build/F3_seqcst_fence/F3_seqcst_fence_reference" > run-existing-F3-tsan.txt 2>&1
    printf '%s\n' "$?" > status-existing-F3-tsan.txt
fi

find "$work_dir" -maxdepth 1 -type f -printf '%f\n' | sort > file-list.txt
for f in status-*.txt; do
    printf '%s=' "$f"
    cat "$f"
done | sort > status-summary.txt

python3 - <<'PY'
import json
from pathlib import Path
statuses = {}
for path in sorted(Path('.').glob('status-*.txt')):
    statuses[path.name] = path.read_text(encoding='utf-8', errors='replace').strip()
Path('status-summary.json').write_text(json.dumps(statuses, indent=2, sort_keys=True) + '\n', encoding='utf-8')
PY

cp ./*.cpp ./*.txt ./*.json "$repo_report"/
cat status-summary.txt
