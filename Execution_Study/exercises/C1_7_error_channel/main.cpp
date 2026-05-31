#include <stdexec/execution.hpp>
#include <iostream>
#include <string>
#include <stdexcept>
#include <sstream>

namespace ex = stdexec;

// ============ Types ============
struct ParseRequest {
    std::string raw;
};

struct ParseResult {
    bool ok;
    std::string data;
    int error_code;
};

// ============ Parse function ============
// Parses "name,int,double" format. Throws on invalid input.
ParseResult parse(ParseRequest req) {
    std::cout << "[parse] Attempting to parse: \"" << req.raw << "\"\n";

    if (req.raw.empty()) {
        throw std::invalid_argument("Empty input string");
    }

    // Expect at least two commas
    auto first_comma = req.raw.find(',');
    auto second_comma = req.raw.find(',', first_comma + 1);
    if (first_comma == std::string::npos || second_comma == std::string::npos) {
        throw std::invalid_argument("Expected format: name,int,double");
    }

    std::string name = req.raw.substr(0, first_comma);
    std::string int_part = req.raw.substr(first_comma + 1, second_comma - first_comma - 1);
    std::string dbl_part = req.raw.substr(second_comma + 1);

    // Validate numeric fields (will throw on failure)
    std::stoi(int_part);
    std::stod(dbl_part);

    return ParseResult{true, name + " [parsed OK]", 0};
}

// ============ Recovery function for upon_error ============
ParseResult recover_from_error(std::exception_ptr ep) {
    try {
        if (ep) std::rethrow_exception(ep);
    } catch (const std::exception& e) {
        std::cout << "[upon_error] Caught exception: " << e.what() << "\n";
    }
    return ParseResult{false, "BadRequestResult", -1};
}

int main() {
    std::cout << "===== Exercise 7: Error Channel =====\n\n";

    // ---- Test 1: Valid input ----
    {
        std::cout << "--- Test 1: Valid input ---\n";
        ParseRequest input{"name,42,3.14"};

        auto sndr = ex::just(std::move(input))
            | ex::then([](ParseRequest req) -> ParseResult {
                return parse(std::move(req));
            })
            // TODO [必做]: Add upon_error to recover from exceptions.
            //   upon_error receives a std::exception_ptr.
            //   Return a ParseResult with ok=false as fallback.
            | ex::upon_error([](std::exception_ptr ep) -> ParseResult {
                return recover_from_error(ep);
            });

        auto [result] = ex::sync_wait(std::move(sndr)).value();
        std::cout << "  ok=" << result.ok
                  << " data=\"" << result.data
                  << "\" error_code=" << result.error_code << "\n\n";
    }

    // ---- Test 2: Invalid input (empty string) ----
    {
        std::cout << "--- Test 2: Invalid input (empty) ---\n";
        ParseRequest input{""};

        auto sndr = ex::just(std::move(input))
            | ex::then([](ParseRequest req) -> ParseResult {
                return parse(std::move(req));
            })
            // TODO [必做]: Add upon_error here (same pattern as above)
            | ex::upon_error([](std::exception_ptr ep) -> ParseResult {
                return recover_from_error(ep);
            });

        auto [result] = ex::sync_wait(std::move(sndr)).value();
        std::cout << "  ok=" << result.ok
                  << " data=\"" << result.data
                  << "\" error_code=" << result.error_code << "\n\n";
    }

    // ---- Test 3: let_error version ----
    {
        std::cout << "--- Test 3: let_error version (sender-level recovery) ---\n";
        ParseRequest input{""};

        // TODO [必做]: Rewrite using let_error instead of upon_error.
        //   let_error should return a *sender*, e.g.:
        //     ex::let_error([](std::exception_ptr ep) {
        //         // Log the error
        //         // Return a new sender, e.g. ex::just(ParseResult{...})
        //     })
        //   This allows async recovery (the returned sender could do more work).

        // Placeholder - replace with let_error version:
        auto sndr = ex::just(std::move(input))
            | ex::then([](ParseRequest req) -> ParseResult {
                return parse(std::move(req));
            });
            // | ex::let_error([](std::exception_ptr ep) {
            //     std::cout << "[let_error] Recovering via sender chain\n";
            //     try { if (ep) std::rethrow_exception(ep); }
            //     catch (const std::exception& e) {
            //         std::cout << "[let_error] Error was: " << e.what() << "\n";
            //     }
            //     return ex::just(ParseResult{false, "Recovered via let_error", -2});
            // });

        // TODO [必做]: Uncomment the let_error chain above and remove the
        //   try/catch wrapper below once it compiles.
        try {
            auto [result] = ex::sync_wait(std::move(sndr)).value();
            std::cout << "  ok=" << result.ok
                      << " data=\"" << result.data
                      << "\" error_code=" << result.error_code << "\n\n";
        } catch (const std::exception& e) {
            std::cout << "  (Exception escaped - let_error not yet wired): "
                      << e.what() << "\n\n";
        }
    }

    // ---- TODO [进阶]: Error after upon_error ----
    // Intentionally throw again *after* upon_error to prove that
    // error recovery does not make subsequent stages immune to failure.
    // {
    //     auto sndr = ex::just(ParseRequest{""})
    //         | ex::then([](ParseRequest req) -> ParseResult {
    //             return parse(std::move(req));
    //         })
    //         | ex::upon_error([](std::exception_ptr) -> ParseResult {
    //             return ParseResult{false, "recovered", -1};
    //         })
    //         | ex::then([](ParseResult r) -> ParseResult {
    //             throw std::runtime_error("Error after recovery!");
    //             return r;
    //         })
    //         | ex::upon_error([](std::exception_ptr ep) -> ParseResult {
    //             // Second error handler
    //             return ParseResult{false, "double-fault recovered", -99};
    //         });
    //     auto [result] = ex::sync_wait(std::move(sndr)).value();
    //     std::cout << "  [进阶] ok=" << result.ok
    //               << " data=\"" << result.data << "\"\n";
    // }

    // ---- TODO [进阶]: let_error retry (retry once) ----
    // Build a retry-once pattern using let_error:
    //   On error, return a sender that re-attempts the parse on the same input.
    //   If it fails again, return a fallback result.

    std::cout << "===== Done =====\n";
    return 0;
}
