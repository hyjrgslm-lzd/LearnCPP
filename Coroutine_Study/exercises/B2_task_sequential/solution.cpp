#include "coroutine_study/exercise_check.hpp"
#include "coroutine_study/lazy_task.hpp"

#include <format>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

struct User { int id; std::string name; };
struct Profile { int user_id; std::string display_name; };
struct ValidatedProfile { int user_id; std::string display_name; bool is_valid; int score; };

std::string trace;

coroutine_study::lazy_task<User> fetch_user(int id) {
    trace += "fetch;";
    if (id < 0) throw std::runtime_error("fetch failed");
    co_return User{id, "alice"};
}

coroutine_study::lazy_task<Profile> parse_profile(User user) {
    trace += "parse;";
    co_return Profile{user.id, user.name + "_display"};
}

coroutine_study::lazy_task<ValidatedProfile> validate_profile(Profile profile) {
    trace += "validate;";
    co_return ValidatedProfile{profile.user_id, profile.display_name, true, 88};
}

coroutine_study::lazy_task<std::string> process_user(int id) {
    auto user = co_await fetch_user(id);
    auto profile = co_await parse_profile(std::move(user));
    auto result = co_await validate_profile(std::move(profile));
    co_return std::format("User {} ({}) validated, score={}",
                          result.user_id, result.display_name, result.score);
}

} // namespace

int main() {
    using coroutine_study::check;

    auto text = coroutine_study::sync_wait(process_user(42));
    check(text == "User 42 (alice_display) validated, score=88", "sequential value flow");
    check(trace == "fetch;parse;validate;", "sequential await order");

    bool thrown = false;
    try {
        (void)coroutine_study::sync_wait(process_user(-1));
    } catch (const std::runtime_error&) {
        thrown = true;
    }
    check(thrown, "exception propagates through co_await chain");

    std::cout << "B2_reference OK\n";
}
