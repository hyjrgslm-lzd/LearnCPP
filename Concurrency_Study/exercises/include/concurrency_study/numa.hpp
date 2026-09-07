#pragma once

#include "exercise_check.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <future>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(__linux__)
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <sched.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <linux/mempolicy.h>
#endif

namespace cs::numa {
struct unavailable : std::runtime_error { using std::runtime_error::runtime_error; };
struct cpu {
    unsigned group = 0, logical = 0;
    int node = -1, socket = -1, core = -1;
    bool operator==(const cpu& other) const { return group == other.group && logical == other.logical; }
};
struct topology {
    std::vector<cpu> allowed;
    std::vector<int> memory_nodes;
    std::size_t page_size = 0;
    std::string scope;
};

inline std::set<int> parse_list(const std::string& text) {
    std::set<int> result;
    std::istringstream input(text);
    std::string part;
    while (std::getline(input, part, ',')) {
        const auto dash = part.find('-');
        const int lo = std::stoi(part);
        const int hi = dash == std::string::npos ? lo : std::stoi(part.substr(dash + 1));
        if (lo < 0 || hi < lo || hi > 1048576) throw unavailable("invalid CPU/node list");
        for (int n = lo; n <= hi; ++n) result.insert(n);
    }
    return result;
}

inline topology discover() {
    topology result;
#ifdef _WIN32
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    result.page_size = info.dwPageSize;
    GROUP_AFFINITY affinity{};
    if (!GetThreadGroupAffinity(GetCurrentThread(), &affinity)) throw unavailable("GetThreadGroupAffinity failed");
    DWORD_PTR process_mask = 0, system_mask = 0;
    if (!GetProcessAffinityMask(GetCurrentProcess(), &process_mask, &system_mask))
        throw unavailable("GetProcessAffinityMask failed");
    // Candidate subset in the caller's PRIMARY group, not proof of single-group
    // affinity on Win11. discover never changes the caller's affinity.
    // A zero process mask can mean a multi-group process; do not invent a mask.
    if (!process_mask) throw unavailable("multi-group process mask ambiguous; select one group before this lab");
    affinity.Mask &= process_mask;
    DWORD bytes = 0;
    GetSystemCpuSetInformation(nullptr, 0, &bytes, GetCurrentProcess(), 0);
    if (!bytes) throw unavailable("GetSystemCpuSetInformation unavailable");
    std::vector<unsigned char> storage(bytes);
    if (!GetSystemCpuSetInformation(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(storage.data()), bytes,
                                   &bytes, GetCurrentProcess(), 0)) throw unavailable("CPU set query failed");
    ULONG count = 0;
    GetThreadSelectedCpuSets(GetCurrentThread(), nullptr, 0, &count);
    std::vector<ULONG> selected(count);
    if (count && !GetThreadSelectedCpuSets(GetCurrentThread(), selected.data(), count, &count))
        throw unavailable("thread CPU set query failed");
    if (selected.empty()) {
        GetProcessDefaultCpuSets(GetCurrentProcess(), nullptr, 0, &count);
        selected.resize(count);
        if (count && !GetProcessDefaultCpuSets(GetCurrentProcess(), selected.data(), count, &count))
            throw unavailable("process CPU set query failed");
    }
    for (DWORD offset = 0; offset < bytes;) {
        auto* entry = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(storage.data() + offset);
        if (!entry->Size || offset + entry->Size > bytes) throw unavailable("invalid CPU set record");
        if (entry->Type == CpuSetInformation) {
            const auto& c = entry->CpuSet;
            if (c.Group == affinity.Group && c.LogicalProcessorIndex < sizeof(KAFFINITY) * 8 &&
                (affinity.Mask & (KAFFINITY{1} << c.LogicalProcessorIndex)) &&
                (!c.Allocated || c.AllocatedToTargetProcess) &&
                (selected.empty() || std::find(selected.begin(), selected.end(), c.Id) != selected.end())) {
                PROCESSOR_NUMBER processor{c.Group, c.LogicalProcessorIndex, 0};
                USHORT node = 0;
                if (!GetNumaProcessorNodeEx(&processor, &node) || node == 0xffff)
                    throw unavailable("CPU NUMA node query failed");
                result.allowed.push_back({c.Group, c.LogicalProcessorIndex, node, -1, c.CoreIndex});
            }
        }
        offset += entry->Size;
    }
    bytes = 0;
    GetLogicalProcessorInformationEx(RelationProcessorPackage, nullptr, &bytes);
    storage.resize(bytes);
    if (!bytes || !GetLogicalProcessorInformationEx(RelationProcessorPackage,
        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(storage.data()), &bytes))
        throw unavailable("processor package query failed");
    int socket = 0;
    for (DWORD offset = 0; offset < bytes; ++socket) {
        auto* entry = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(storage.data() + offset);
        if (!entry->Size) throw unavailable("invalid package record");
        for (auto& c : result.allowed)
            for (WORD g = 0; g < entry->Processor.GroupCount; ++g) {
                const auto& mask = entry->Processor.GroupMask[g];
                if (c.group == mask.Group && (mask.Mask & (KAFFINITY{1} << c.logical))) c.socket = socket;
            }
        offset += entry->Size;
    }
    ULONG highest = 0;
    if (!GetNumaHighestNodeNumber(&highest)) throw unavailable("NUMA node enumeration failed");
    for (ULONG node = 0; node <= highest; ++node) {
        ULONGLONG available = 0;
        if (GetNumaAvailableMemoryNodeEx(static_cast<USHORT>(node), &available) && available)
            result.memory_nodes.push_back(static_cast<int>(node));
    }
    result.scope = "Windows: candidate subset in caller primary group intersect process mask/CPU sets; not complete cross-group affinity; fresh workers only";
#elif defined(__linux__)
    const long page = sysconf(_SC_PAGESIZE);
    if (page <= 0) throw unavailable("sysconf page size failed");
    result.page_size = static_cast<std::size_t>(page);
    cpu_set_t allowed;
    CPU_ZERO(&allowed);
    if (sched_getaffinity(0, sizeof(allowed), &allowed))
        throw unavailable("sched_getaffinity failed (including masks larger than CPU_SETSIZE)");
    auto read_int = [](const std::string& path) {
        int value = -1;
        std::ifstream input(path);
        if (!(input >> value)) throw unavailable("cannot read " + path);
        return value;
    };
    for (int id = 0; id < CPU_SETSIZE; ++id) if (CPU_ISSET(id, &allowed)) {
        const std::string base = "/sys/devices/system/cpu/cpu" + std::to_string(id);
        int node = -1;
        for (const auto& entry : std::filesystem::directory_iterator(base)) {
            const auto name = entry.path().filename().string();
            if (name.starts_with("node") && name.size() > 4) node = std::stoi(name.substr(4));
        }
        if (node < 0) throw unavailable("sysfs CPU node unavailable");
        result.allowed.push_back({0, static_cast<unsigned>(id), node,
            read_int(base + "/topology/physical_package_id"), read_int(base + "/topology/core_id")});
    }
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) if (line.starts_with("Mems_allowed_list:")) {
        auto nodes = parse_list(line.substr(line.find(':') + 1));
        result.memory_nodes.assign(nodes.begin(), nodes.end());
    }
    result.scope = "Linux: sched_getaffinity + sysfs socket/core/node + Mems_allowed_list";
#else
    throw unavailable("NUMA observation implemented only for Windows and Linux");
#endif
    if (result.allowed.empty() || result.memory_nodes.empty()) throw unavailable("no eligible CPU or memory node");
    return result;
}

inline cpu current_cpu() {
#ifdef _WIN32
    PROCESSOR_NUMBER p{};
    GetCurrentProcessorNumberEx(&p);
    USHORT node = 0;
    if (!GetNumaProcessorNodeEx(&p, &node)) throw unavailable("actual CPU node query failed");
    return {p.Group, p.Number, node};
#elif defined(__linux__)
    unsigned id = 0, node = 0;
    if (syscall(SYS_getcpu, &id, &node, nullptr)) throw unavailable("getcpu failed");
    return {0, id, static_cast<int>(node)};
#else
    throw unavailable("actual CPU unavailable");
#endif
}

class pages {
    void* address_ = nullptr;
    std::vector<void*> blocks_;
    std::size_t bytes_, page_size_;
    void release() noexcept {
#ifdef _WIN32
        for (auto block : blocks_) if (block && !VirtualFree(block, 0, MEM_RELEASE)) std::terminate();
#elif defined(__linux__)
        if (address_ && munmap(address_, bytes_)) std::terminate();
#endif
    }
public:
    // Empty nodes: platform default/first-touch. One: preferred. Many: interleave.
    pages(std::size_t bytes, std::size_t page_size, const std::vector<int>& nodes = {})
        : bytes_(bytes), page_size_(page_size) {
        if (!bytes || !page_size || bytes % page_size || page_size % sizeof(std::uint64_t))
            throw std::invalid_argument("bytes must be a positive multiple of OS page size");
        for (int node : nodes) if (node < 0 || node > 65535)
            throw std::invalid_argument("node outside supported range");
        blocks_.resize(bytes_ / page_size_); // All pointer storage before OS allocations.
#ifdef _WIN32
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        if (page_size_ != info.dwPageSize) throw std::invalid_argument("page size must match Windows base page");
        try {
            // nndPreferred is ignored when committing an existing reservation.
            // Every variant therefore uses the same independent-page layout.
            for (std::size_t i = 0; i < blocks_.size(); ++i) {
                blocks_[i] = nodes.empty() ? VirtualAlloc(nullptr, page_size_, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
                    : VirtualAllocExNuma(GetCurrentProcess(), nullptr, page_size_, MEM_RESERVE | MEM_COMMIT,
                                         PAGE_READWRITE, static_cast<DWORD>(nodes[i % nodes.size()]));
                if (!blocks_[i]) throw unavailable("fresh page reservation/commit failed; partial allocations released");
            }
        } catch (...) { release(); throw; }
#elif defined(__linux__)
        const long base_page = sysconf(_SC_PAGESIZE);
        if (base_page <= 0) throw unavailable("cannot verify Linux base page size");
        if (page_size_ != static_cast<std::size_t>(base_page))
            throw std::invalid_argument("page size must match Linux base page");
        // Build policy storage before mmap: allocation failure cannot leak a mapping.
        const int highest = nodes.empty() ? -1 : *std::max_element(nodes.begin(), nodes.end());
        constexpr auto bits = sizeof(unsigned long) * 8;
        std::vector<unsigned long> mask(nodes.empty() ? 0 : static_cast<std::size_t>(highest) / bits + 1);
        for (int node : nodes) mask[static_cast<std::size_t>(node) / bits] |= 1UL << (node % bits);
        address_ = mmap(nullptr, bytes_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (address_ == MAP_FAILED) { address_ = nullptr; throw unavailable("mmap failed"); }
        for (std::size_t i = 0; i < blocks_.size(); ++i)
            blocks_[i] = static_cast<char*>(address_) + i * page_size_;
        if (madvise(address_, bytes_, MADV_NOHUGEPAGE)) {
            munmap(address_, bytes_); address_ = nullptr;
            throw unavailable("MADV_NOHUGEPAGE failed; base-page experiment not established");
        }
        if (!nodes.empty()) {
            if (syscall(SYS_mbind, address_, bytes_, nodes.size() == 1 ? MPOL_PREFERRED : MPOL_INTERLEAVE,
                        mask.data(), static_cast<unsigned long>(highest + 1), 0)) {
                const int error = errno;
                munmap(address_, bytes_); address_ = nullptr;
                throw unavailable("mbind unavailable: " + std::string(std::strerror(error)));
            }
        }
#else
        throw unavailable("page allocation unavailable");
#endif
    }
    pages(const pages&) = delete;
    pages& operator=(const pages&) = delete;
    ~pages() { release(); }
    std::size_t count() const { return bytes_ / page_size_; }
    std::size_t words() const { return bytes_ / sizeof(std::uint64_t); }
    void initialize(std::size_t begin_page, std::size_t end_page) {
        if (begin_page > end_page || end_page > count()) throw std::out_of_range("page initialization range");
        for (auto page = begin_page; page < end_page; ++page) {
            auto* values = static_cast<std::uint64_t*>(blocks_[page]);
            for (std::size_t i = 0; i < page_size_ / sizeof(*values); ++i)
                std::construct_at(values + i, std::uint64_t{1});
        }
    }
    std::uint64_t sum(std::size_t begin_page, std::size_t end_page) const {
        if (begin_page > end_page || end_page > count()) throw std::out_of_range("page read range");
        // volatile forces each planned read; it is not a synchronization primitive.
        std::uint64_t total = 0;
        for (auto page = begin_page; page < end_page; ++page) {
            const volatile auto* values = static_cast<const std::uint64_t*>(blocks_[page]);
            for (std::size_t i = 0; i < page_size_ / sizeof(*values); ++i) total += values[i];
        }
        return total;
    }
    std::vector<int> nodes() const {
        std::vector<int> result(count(), -1);
#ifdef _WIN32
        std::vector<PSAPI_WORKING_SET_EX_INFORMATION> entries(count());
        for (std::size_t i = 0; i < count(); ++i) entries[i].VirtualAddress = blocks_[i];
        if (entries.size() > std::numeric_limits<DWORD>::max() / sizeof(entries[0])) throw unavailable("working set query too large");
        if (!QueryWorkingSetEx(GetCurrentProcess(), entries.data(), static_cast<DWORD>(entries.size() * sizeof(entries[0]))))
            throw unavailable("QueryWorkingSetEx failed");
        for (std::size_t i = 0; i < count(); ++i) if (entries[i].VirtualAttributes.Valid) {
            if (entries[i].VirtualAttributes.LargePage) throw unavailable("large page observed in base-page experiment");
            result[i] = static_cast<int>(entries[i].VirtualAttributes.Node);
        }
#elif defined(__linux__)
        if (syscall(SYS_move_pages, 0, count(), blocks_.data(), nullptr, result.data(), 0) < 0)
            throw unavailable("move_pages query denied/unavailable: " + std::string(std::strerror(errno)));
        for (auto& node : result) if (node < 0) node = -1;
#else
        throw unavailable("page residency query unavailable");
#endif
        return result;
    }
    std::string layout() const {
#ifdef _WIN32
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        return "independent-base-page-reservations; reservations=" + std::to_string(count()) +
            "; allocation_granularity=" + std::to_string(info.dwAllocationGranularity) +
            "; page_bytes=" + std::to_string(page_size_);
#else
        return "contiguous-anonymous-VMA; MADV_NOHUGEPAGE; page_bytes=" + std::to_string(page_size_);
#endif
    }
};

// An anonymous VMA's interleave phase need not be zero. Check every base page
// against ascending nodemask order; infer phase once, never from a histogram.
inline std::optional<std::size_t> interleave_phase(const std::vector<int>& actual, std::vector<int> requested) {
    std::sort(requested.begin(), requested.end());
    requested.erase(std::unique(requested.begin(), requested.end()), requested.end());
    if (requested.empty() || actual.size() < requested.size() || requested.front() < 0) return std::nullopt;
    auto first = std::find(requested.begin(), requested.end(), actual.front());
    if (first == requested.end()) return std::nullopt;
    const auto phase = static_cast<std::size_t>(first - requested.begin());
    for (std::size_t i = 0; i < actual.size(); ++i)
        if (actual[i] != requested[(i + phase) % requested.size()]) return std::nullopt;
    return phase;
}

inline void describe(const topology& t, std::ostream& out) {
    out << t.scope << "\nbase_page_bytes=" << t.page_size << " allowed_memory_nodes=";
    for (int n : t.memory_nodes) out << n << ' ';
    out << "\ngroup,cpu,socket,core,node (same group/socket/core rows are SMT siblings)\n";
    for (auto c : t.allowed) out << c.group << ',' << c.logical << ',' << c.socket << ',' << c.core << ',' << c.node << '\n';
}
inline void describe_pages(std::string_view stage, const std::vector<int>& nodes, std::ostream& out) {
    std::map<int, std::size_t> histogram;
    for (int node : nodes) ++histogram[node];
    out << stage << ": ";
    for (auto [node, count] : histogram) out << "node=" << node << " pages=" << count << ' ';
    out << "(-1=not resident/unverifiable)\n";
}
inline void require_placement(const std::vector<int>& actual, const std::vector<int>& expected) {
    if (actual != expected) throw unavailable("requested page placement not observed on every page");
}

// This is the ONLY affinity-setting entry point: create fresh dedicated threads,
// bind within them, and let them exit. No restoration claim for reusable workers.
template<class F> void on_cpus(const std::vector<cpu>& cpus, F&& operation) {
    std::vector<std::future<void>> results;
    std::vector<std::jthread> workers;
    results.reserve(cpus.size()); workers.reserve(cpus.size());
    for (std::size_t i = 0; i < cpus.size(); ++i) {
        std::packaged_task<void()> task([&, i] {
            const auto target = cpus[i];
#ifdef _WIN32
            if (target.logical >= sizeof(KAFFINITY) * 8 || target.group > 65535)
                throw std::invalid_argument("CPU outside group mask range");
            GROUP_AFFINITY desired{};
            desired.Group = static_cast<WORD>(target.group);
            desired.Mask = KAFFINITY{1} << target.logical;
            if (!SetThreadGroupAffinity(GetCurrentThread(), &desired, nullptr))
                throw unavailable("SetThreadGroupAffinity rejected dedicated worker CPU");
#elif defined(__linux__)
            if (target.logical >= CPU_SETSIZE) throw std::invalid_argument("CPU outside supported mask range");
            cpu_set_t desired;
            CPU_ZERO(&desired);
            CPU_SET(target.logical, &desired);
            if (sched_setaffinity(0, sizeof(desired), &desired)) throw unavailable("sched_setaffinity rejected CPU");
#else
            (void)target;
            throw unavailable("dedicated worker affinity unavailable");
#endif
            if (!(current_cpu() == cpus[i])) throw unavailable("actual CPU differs before operation");
            operation(i);
            if (!(current_cpu() == cpus[i])) throw unavailable("actual CPU differs after operation");
        });
        results.push_back(task.get_future());
        workers.emplace_back(std::move(task));
    }
    for (auto& result : results) result.get();
}

class placement_experiment {
    topology topology_;
    std::string variant_;
    std::vector<cpu> cpus_;
    std::vector<int> requested_, expected_;
    std::unique_ptr<pages> memory_;
    std::vector<cpu> init_before_, init_after_, read_before_, read_after_;
    std::vector<std::uint64_t> sums_;
    std::pair<std::size_t, std::size_t> range(std::size_t i) const {
        const auto n = memory_->count(), workers = cpus_.size();
        const auto begin = (n / workers) * i + std::min(i, n % workers);
        return {begin, begin + n / workers + (i < n % workers)};
    }
public:
    placement_experiment(topology t, std::string variant, std::size_t bytes)
        : topology_(std::move(t)), variant_(std::move(variant)) {
        if (variant_ != "local" && variant_ != "remote" && variant_ != "interleaved" &&
            variant_ != "firsttouch" && variant_ != "parallel-init")
            throw std::invalid_argument("variant: local, remote, interleaved, firsttouch, parallel-init");
        for (auto c : topology_.allowed)
            if (std::find(topology_.memory_nodes.begin(), topology_.memory_nodes.end(), c.node) != topology_.memory_nodes.end()) {
                cpus_.push_back(c); break;
            }
        if (cpus_.empty()) throw unavailable("no CPU with eligible local memory");
        if (variant_ == "local") requested_ = {cpus_[0].node};
        if (variant_ == "remote" || variant_ == "interleaved") {
            for (int node : topology_.memory_nodes) if (node != cpus_[0].node) { requested_ = {node}; break; }
            if (requested_.empty()) throw unavailable("only one eligible memory node; remote/interleave comparison unavailable");
            if (variant_ == "interleaved") requested_.insert(requested_.begin(), cpus_[0].node);
        }
        std::sort(requested_.begin(), requested_.end());
        if (variant_ == "firsttouch" || variant_ == "parallel-init") {
            auto second = std::find_if(topology_.allowed.begin(), topology_.allowed.end(), [&](cpu c) {
                return c.node != cpus_[0].node && std::find(topology_.memory_nodes.begin(), topology_.memory_nodes.end(), c.node) != topology_.memory_nodes.end();
            });
            if (second == topology_.allowed.end()) second = std::find_if(topology_.allowed.begin(), topology_.allowed.end(), [&](cpu c) {
                return c.node == cpus_[0].node && (c.socket != cpus_[0].socket || c.core != cpus_[0].core);
            });
            if (second != topology_.allowed.end()) cpus_.push_back(*second);
        }
        memory_ = std::make_unique<pages>(bytes, topology_.page_size, requested_);
        expected_.resize(memory_->count());
        if (variant_ == "parallel-init") {
            for (std::size_t i = 0; i < cpus_.size(); ++i) {
                auto [begin, end] = range(i);
                std::fill(expected_.begin() + begin, expected_.begin() + end, cpus_[i].node);
            }
        } else for (std::size_t i = 0; i < expected_.size(); ++i)
            expected_[i] = requested_.empty() ? cpus_[0].node : requested_[i % requested_.size()];
        sums_.resize(cpus_.size());
        read_before_.resize(cpus_.size()); read_after_.resize(cpus_.size());
    }
    void prepare(std::ostream& out) {
        out << "layout=" << memory_->layout() << '\n';
        describe_pages("before-touch", memory_->nodes(), out);
        const auto initializers = variant_ == "parallel-init" ? cpus_ : std::vector<cpu>{cpus_[0]};
        init_before_.resize(initializers.size()); init_after_.resize(initializers.size());
        on_cpus(initializers, [&](std::size_t i) {
            init_before_[i] = current_cpu();
            if (variant_ == "parallel-init") { auto [begin, end] = range(i); memory_->initialize(begin, end); }
            else memory_->initialize(0, memory_->count());
            init_after_[i] = current_cpu();
        });
        auto actual = memory_->nodes();
        describe_pages("after-touch", actual, out);
#if defined(__linux__)
        if (variant_ == "interleaved") {
            const auto phase = interleave_phase(actual, requested_);
            if (!phase) throw unavailable("base-page placement is not a complete interleave cycle");
            out << "observed_interleave_phase=" << *phase << " (sorted nodemask order)\n";
            expected_ = actual; // All later snapshots must retain this same phase.
        }
#endif
        require_placement(actual, expected_);
        scan(); // Untimed pre-touch/read warm-up, including code and thread creation.
        verify(out, "before-timing");
    }
    void scan() {
        on_cpus(cpus_, [&](std::size_t i) {
            read_before_[i] = current_cpu();
            auto [begin, end] = range(i);
            sums_[i] = memory_->sum(begin, end);
            read_after_[i] = current_cpu();
        });
    }
    void verify(std::ostream& out, std::string_view stage = "after-timing") const {
        const auto actual = memory_->nodes();
        describe_pages(stage, actual, out);
        require_placement(actual, expected_);
        std::uint64_t total = 0;
        for (std::size_t i = 0; i < cpus_.size(); ++i) {
            total += sums_[i];
            out << "reader=" << i << " requested=" << cpus_[i].group << ':' << cpus_[i].logical
                << " actual-before=" << read_before_[i].group << ':' << read_before_[i].logical
                << " node=" << read_before_[i].node << " actual-after=" << read_after_[i].group << ':' << read_after_[i].logical
                << " node=" << read_after_[i].node << '\n';
        }
        for (std::size_t i = 0; i < init_before_.size(); ++i)
            out << "initializer=" << i << " actual-before=" << init_before_[i].group << ':' << init_before_[i].logical
                << " node=" << init_before_[i].node << " actual-after=" << init_after_[i].group << ':' << init_after_[i].logical
                << " node=" << init_after_[i].node << '\n';
        check(total == memory_->words(), "every initialized word read exactly once with value one");
    }
    std::size_t threads() const { return cpus_.size(); }
    std::size_t completed() const { return memory_->words(); }
    std::string layout() const { return memory_->layout(); }
};
} // namespace cs::numa
