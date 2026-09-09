#include <document.hpp>
#include <check.hpp>
#include <array>
#include <concepts>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <new>
#include <unordered_set>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif

// Injection is compiled only into separate allocation targets. Normal Debug
// targets retain the standard allocator and MSVC's default iterator diagnostics.
namespace allocation_probe {
    bool active = false;
    int calls = 0;
    int fail_at = -1;
    const char* phase = "initialization";
}
#ifdef C03_ALLOCATION_INJECTION
void* operator new(std::size_t bytes) {
    if (allocation_probe::active) {
        const int index = allocation_probe::calls++;
        if (index == allocation_probe::fail_at) {
            allocation_probe::active = false;
            throw std::bad_alloc();
        }
    }
    if (void* memory = std::malloc(bytes == 0 ? 1 : bytes)) return memory;
    throw std::bad_alloc();
}
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
#endif

namespace {
using namespace c03;
ElementId id(std::uint64_t n) { return ElementId::make(n).value(); }
PositiveLength length(int n) { return PositiveLength::make(n).value(); }
Element rectangle(std::uint64_t n, int w = 2, int h = 3) {
    return {id(n), Rectangle{length(w), length(h)}, std::nullopt};
}
Element circle(std::uint64_t n, int r = 5) {
    return {id(n), Circle{length(r)}, std::nullopt};
}
Document initial() {
    Document d;
    check(d.apply(Add{rectangle(1)}).has_value(), "first Add succeeds");
    check(d.size() == 1 && d.find(id(1)) == rectangle(1), "Add inserts an actual element");
    check(d.apply(Add{circle(2)}).has_value(), "second Add succeeds");
    return d;
}

#ifndef C03_ALLOCATION_INJECTION
void values_and_reads() {
    static_assert(std::regular<Document>);
    static_assert(!std::default_initializable<ElementId>);
    static_assert(!std::is_convertible_v<int, PositiveLength>);
    static_assert(noexcept(std::declval<Document&>().swap(std::declval<Document&>())));
    check(!ElementId::make(0), "zero ID rejected");
    check(!PositiveLength::make(0) && !PositiveLength::make(-1), "nonpositive length rejected");
    check(ElementId::make(std::numeric_limits<std::uint64_t>::max()).has_value(), "largest ID representable");
    std::unordered_set<ElementId> ids{id(1), id(1), id(2)};
    check(ids.size() == 2 && ids.contains(id(1)), "ID equality and hash agree");
    Document empty;
    check(empty.size() == 0 && !empty.find(id(1)), "empty document and missing query");
    check(empty.apply_batch({}).has_value(), "empty batch succeeds");
    auto d = initial();
    auto snapshot = d.snapshot();
    snapshot[0].shape = Circle{length(9)};
    auto found = d.find(id(1));
    found->label = "local change";
    check(d.find(id(1)) == rectangle(1), "query and snapshot are independent values");
}

void commands_and_atomicity() {
    auto d = initial();
    const auto before = d;
    auto result = d.apply(Add{rectangle(1, 9, 9)});
    check(!result && result.error() == EditError{EditErrorCode::duplicate_id, id(1)}, "duplicate identifies offending ID");
    check(d == before, "duplicate refusal leaves document unchanged");
    result = d.apply(Replace{rectangle(99)});
    check(!result && result.error().code == EditErrorCode::missing_id, "missing Replace rejected");
    result = d.apply(Erase{id(99)});
    check(!result && result.error().id == id(99) && d == before, "missing Erase rejected without effect");

    const std::array<Edit, 3> invalid{Replace{rectangle(1, 8, 9)}, Add{rectangle(3)}, Erase{id(99)}};
    result = d.apply_batch(invalid);
    check(!result && result.error().code == EditErrorCode::missing_id, "late batch failure returned");
    check(d == before, "failed batch preserves the entire original document");

    const std::array<Edit, 3> valid{Replace{rectangle(1, 8, 9)}, Erase{id(2)}, Add{circle(3)}};
    check(d.apply_batch(valid).has_value(), "valid batch succeeds");
    check(d.snapshot() == std::vector<Element>{rectangle(1, 8, 9), circle(3)}, "replace preserves position and erase preserves remaining order");
    const std::array<Edit, 3> sequential{Add{rectangle(7)}, Replace{circle(7)}, Erase{id(7)}};
    const auto prior = d;
    check(d.apply_batch(sequential).has_value() && d == prior, "batch sees its earlier operations in order");
    const std::array<Edit, 2> duplicate{Add{rectangle(8)}, Add{circle(8)}};
    check(!d.apply_batch(duplicate) && d == prior, "duplicate within a batch rolls everything back");
}

void copies_and_moves() {
    auto a = initial();
    Document b = a;
    check(a == b, "copy preserves value");
    check(b.apply(Erase{id(1)}).has_value() && a.size() == 2, "copies do not share mutable state");
    b = a;
    check(b == a, "copy assignment replaces previous value");
    Document* same = &b;
    b = *same;
    check(b == a, "self-copy retains value");
    Document moved(std::move(b));
    check(moved == a && b.size() == 0, "move construction transfers value and empties source");
    b = std::move(moved);
    check(b == a && moved.size() == 0, "move assignment transfers value and empties source");
    b = std::move(*same);
    check(b == a, "self-move retains value");
    check(moved.apply(Add{circle(10)}).has_value(), "moved-from document reusable");
}
#else

void allocation_failures() {
    allocation_probe::phase = "allocation experiment setup";
    auto original = initial();
    auto labeled = rectangle(1, 6, 7);
    labeled.label = std::string(256, 'x');
    const std::array<Edit, 2> edits{Replace{labeled}, Add{circle(3)}};
    auto complete = original;
    check(complete.apply_batch(edits).has_value(), "allocation experiment baseline correct");

    auto measured = original;
    allocation_probe::calls = 0;
    allocation_probe::fail_at = -1;
    allocation_probe::active = true;
    allocation_probe::phase = "measure batch allocations";
    const auto success = measured.apply_batch(edits);
    allocation_probe::active = false;
    const int count = allocation_probe::calls;
    check(success.has_value() && measured == complete && count > 0, "count actual ordinary allocations before injecting failures");
    for (int k = 0; k < count; ++k) {
        auto target = original;
        allocation_probe::calls = 0;
        allocation_probe::fail_at = k;
        allocation_probe::active = true;
        allocation_probe::phase = "inject batch allocation failure";
        bool failed = false;
        try { (void)target.apply_batch(edits); }
        catch (const std::bad_alloc&) { failed = true; }
        allocation_probe::active = false;
        check(failed, "injected allocation failure reached");
        check(target == original, "allocation failure preserves the original document");
        check(target.apply_batch(edits).has_value() && target == complete, "document usable after allocation failure");
    }
    // The long label makes the copy path nontrivial independently of vector capacity.
    auto target = original;
    allocation_probe::calls = 0;
    allocation_probe::fail_at = 0;
    allocation_probe::active = true;
    allocation_probe::phase = "inject copy-assignment allocation failure";
    bool threw = false;
    try { target = complete; }
    catch (const std::bad_alloc&) { threw = true; }
    allocation_probe::active = false;
    check(threw && target == original, "copy assignment failure preserves target");
    std::cout << "ordinary allocation failure points checked: " << count << '\n';
}
#endif
}

int main() {
#ifdef _MSC_VER
    // Process-local: unattended failures remain failures, without interactive CRT dialogs.
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    std::set_terminate([] {
        std::fprintf(stderr, "unexpected terminate: phase=%s fail_at=%d allocations=%d\n",
            allocation_probe::phase, allocation_probe::fail_at, allocation_probe::calls);
        std::_Exit(86);
    });
#ifdef C03_ALLOCATION_INJECTION
    allocation_failures();
#else
    allocation_probe::phase = "value and query checks";
    values_and_reads();
    allocation_probe::phase = "command and transaction checks";
    commands_and_atomicity();
    allocation_probe::phase = "copy and move checks";
    copies_and_moves();
#endif
    std::cout << "document contract checks passed\n";
}
