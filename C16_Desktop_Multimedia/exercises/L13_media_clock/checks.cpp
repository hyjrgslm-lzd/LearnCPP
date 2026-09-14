#include <solution.hpp>
#include <c16/check.hpp>

#include <limits>
#include <stdexcept>

#ifdef _MSC_VER
#pragma warning(disable : 4702)
#endif

namespace {

template <class Exception, class F>
void require_throws(F&& body, const char* message)
{
    try {
        body();
    } catch (const Exception&) {
        return;
    }
    c16::require(false, message);
}

void check_pause_rate_seek()
{
    c16_l13::ManualClock clock;
    c16_l13::MediaClock media(clock);
    media.start(1000);
    clock.advance_us(500000);
    c16::require(media.position_us() == 501000, "running clock maps monotonic time to media time");
    media.pause();
    clock.advance_us(500000);
    c16::require(media.position_us() == 501000, "pause freezes media time");
    media.resume();
    media.set_rate(2.0);
    clock.advance_us(100000);
    c16::require(media.position_us() == 701000, "rate changes slope after settling anchor");
    const auto generation = media.seek(2000000);
    c16::require(generation == 1 && media.position_us() == 2000000, "seek resets position and bumps generation");
}

void check_frame_decision()
{
    c16_l13::ManualClock clock;
    c16_l13::MediaClock media(clock);
    media.start(0);
    c16::require(media.classify({10000, 0}) == c16_l13::Decision::Display, "near frame displays");
    c16::require(media.classify({90000, 0}) == c16_l13::Decision::Wait, "future frame waits");
    c16::require(media.classify({-90000, 0}) == c16_l13::Decision::Drop, "late frame drops");
    const auto generation = media.seek(1000000);
    c16::require(media.classify({1000000, generation - 1}) == c16_l13::Decision::Drop, "old seek generation drops");
    c16::require(media.classify({1000000, generation}) == c16_l13::Decision::Display, "current generation frame displays");
}

void check_time_domain_boundaries()
{
    c16_l13::ManualClock clock;
    c16_l13::MediaClock media(clock);
    require_throws<std::invalid_argument>([&] { clock.advance_us(-1); }, "manual clock rejects negative advance");
    c16::require(clock.now_us() == 0, "rejected negative advance does not move clock");

    clock.advance_us(1000);
    require_throws<std::invalid_argument>([&] { media.start(-1); }, "start rejects negative media time");
    require_throws<std::invalid_argument>([&] { media.seek(-1); }, "seek rejects negative media time");
    c16::require(media.position_us() == 0, "rejected seek does not rewrite paused anchor");

    require_throws<std::invalid_argument>([&] { media.set_rate(65.0); }, "rate above declared domain is rejected");
    require_throws<std::invalid_argument>([&] { media.set_rate(std::numeric_limits<double>::infinity()); }, "infinite rate is rejected");
    c16::require(media.position_us() == 0, "rejected rate does not rewrite anchor");

    media.start(std::numeric_limits<long long>::max() - 10);
    require_throws<std::overflow_error>([&] { clock.advance_us(11); media.position_us(); }, "position overflow is reported");

    c16_l13::ManualClock near_limit;
    near_limit.advance_us(std::numeric_limits<long long>::max() - 1);
    require_throws<std::overflow_error>([&] { near_limit.advance_us(2); }, "manual clock rejects overflow advance");

    c16_l13::ManualClock classify_clock;
    c16_l13::MediaClock classify_media(classify_clock);
    classify_media.start(std::numeric_limits<long long>::max() - 1000);
    c16::require(classify_media.classify({std::numeric_limits<long long>::min(), 0}) == c16_l13::Decision::Drop,
        "classification handles extreme negative PTS without integer subtraction overflow");
}

} // namespace

int main()
{
    return c16::run([] {
        check_pause_rate_seek();
        check_frame_decision();
        check_time_domain_boundaries();
    });
}
