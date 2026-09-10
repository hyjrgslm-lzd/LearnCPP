#include <completion.hpp>
#include <check.hpp>
#include <c07/completion_io.hpp>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

namespace {

c07::completion_probe_report valid_ledger_report() {
    c07::completion_probe_report report;
    report.payload = "completion identity + payload";
    report.events = {
        c07::read_accepted(17, report.payload.size()),
        c07::read_completed(17, report.payload.size()),
        c07::read_accepted(29, 1),
        c07::cancel_submitted(31, 29),
        c07::target_completed(29, 0, std::make_error_code(std::errc::operation_canceled)),
        c07::cancel_completed(31, 29),
    };
    return report;
}

c07::completion_probe_report completed_before_cancel_report() {
    c07::completion_probe_report report;
    report.payload = "completion identity + payload";
    report.events = {
        c07::read_accepted(41, report.payload.size()),
        c07::read_completed(41, report.payload.size()),
        c07::read_accepted(53, 1),
        c07::target_completed(53, 0),
        c07::cancel_submitted(59, 53),
        c07::cancel_completed(59, 53, std::make_error_code(std::errc::no_such_process)),
    };
    return report;
}

void expect_valid(const c07::completion_probe_report& report, std::string_view message) {
    check(c07_l08::validate_completion_ledger(report).has_value(), message);
}

void expect_invalid(c07::completion_probe_report report, std::string_view message) {
    check(!c07_l08::validate_completion_ledger(report), message);
}

} // namespace

int main(int argc, char** argv) {
    const bool run_platform = argc == 2 && std::strcmp(argv[1], "--platform") == 0;
    auto report = valid_ledger_report();
    expect_valid(report, "normal ledger is accepted");

    expect_valid(completed_before_cancel_report(), "completed-before-cancel race is accepted");

    auto missing_target = report;
    std::erase_if(missing_target.events, [](const c07::completion_event& event) {
        return event.request_id == 29 && event.kind == c07::completion_event_kind::target_completed;
    });
    expect_invalid(missing_target, "missing target completion is rejected");

    auto duplicate = report;
    duplicate.events.push_back(c07::read_completed(17, report.payload.size()));
    expect_invalid(duplicate, "duplicate completion is rejected");

    auto unknown = report;
    unknown.events.push_back(c07::target_completed(700, 0));
    expect_invalid(unknown, "unknown completion id is rejected");

    auto wrong_order = report;
    std::swap(wrong_order.events[0], wrong_order.events[1]);
    expect_invalid(wrong_order, "completion before acceptance is rejected");

    auto wrong_bytes = report;
    wrong_bytes.events[1] = c07::read_completed(17, report.payload.size() - 1);
    expect_invalid(wrong_bytes, "wrong payload byte count is rejected");

    auto missing_cancel = report;
    std::erase_if(missing_cancel.events, [](const c07::completion_event& event) {
        return event.request_id == 31 && event.kind == c07::completion_event_kind::cancel_completed;
    });
    expect_invalid(missing_cancel, "missing cancel completion is rejected");

    auto bad_cancel_error = report;
    bad_cancel_error.events[5] = c07::cancel_completed(31, 29, std::make_error_code(std::errc::io_error));
    expect_invalid(bad_cancel_error, "unexpected cancel error is rejected");

    auto bad_target_error = report;
    bad_target_error.events[4] = c07::target_completed(29, 0, std::make_error_code(std::errc::io_error));
    expect_invalid(bad_target_error, "unexpected target error is rejected");

    if (!run_platform) {
        std::cout << "L08 completion ledger checks passed\n";
        return 0;
    }

    const auto platform = c07::run_completion_probe(report.payload);
    if (!platform && platform.error() == std::make_error_code(std::errc::function_not_supported)) {
        std::cout << "SKIP: platform completion path is not enabled or not supported\n";
        return 77;
    }
    if (!platform) std::cerr << "completion probe error: " << platform.error().message() << '\n';
    check(platform.has_value(), "platform completion path is available");
    check(platform->payload == report.payload, "completion reports the real transferred payload");
    expect_valid(*platform, "platform ledger is accepted");

    std::cout << "L08 completion platform checks passed\n";
}
