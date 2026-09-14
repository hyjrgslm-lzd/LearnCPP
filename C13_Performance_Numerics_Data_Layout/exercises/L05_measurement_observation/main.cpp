#include "concurrency_study/benchmark.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct particles {
    std::vector<float> x, y, vx, vy, mass;
};

void check(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

bool near(double actual, double expected, double absolute = 1e-12, double relative = 1e-12) {
    return std::isfinite(actual) && std::isfinite(expected)
        && std::abs(actual - expected) <= absolute + relative * std::max(std::abs(actual), std::abs(expected));
}

particles make_particles(std::size_t n, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> pos(-10.0f, 10.0f);
    std::uniform_real_distribution<float> vel(-1.0f, 1.0f);
    std::uniform_real_distribution<float> mass(0.5f, 4.0f);
    particles out;
    out.x.reserve(n);
    out.y.reserve(n);
    out.vx.reserve(n);
    out.vy.reserve(n);
    out.mass.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        out.x.push_back(pos(rng));
        out.y.push_back(pos(rng));
        out.vx.push_back(vel(rng));
        out.vy.push_back(vel(rng));
        out.mass.push_back(mass(rng));
    }
    return out;
}

void validate(const particles& p) {
    const auto n = p.x.size();
    check(p.y.size() == n && p.vx.size() == n && p.vy.size() == n && p.mass.size() == n, "SoA shape mismatch");
}

void update(particles& p, float dt) {
    validate(p);
    for (std::size_t i = 0; i < p.x.size(); ++i) {
        p.x[i] += p.vx[i] * dt;
        p.y[i] += p.vy[i] * dt;
    }
}

double kinetic_energy(const particles& p) {
    validate(p);
    double total = 0.0;
    for (std::size_t i = 0; i < p.x.size(); ++i)
        total += 0.5 * static_cast<double>(p.mass[i])
            * (static_cast<double>(p.vx[i]) * p.vx[i] + static_cast<double>(p.vy[i]) * p.vy[i]);
    return total;
}
} // namespace

int main(int argc, char** argv) try {
    cs::bench::arguments args(argc, argv);
    const auto n = args.number("--size", 10000);
    const auto seed = args.number("--seed", 42);
    const auto steps = args.number("--steps", 1);
    args.finish();
    check(n <= 1'000'000, "size cap");
    check(steps > 0 && steps <= 1000, "steps cap");
    check(seed <= std::numeric_limits<unsigned>::max(), "seed range");

    particles data;
    const double make_ms = cs::bench::measure_ms([&] { data = make_particles(n, static_cast<unsigned>(seed)); });
    const auto before_x = data.x;
    const auto before_y = data.y;
    const auto before_vx = data.vx;
    const auto before_vy = data.vy;
    double update_ms = cs::bench::measure_ms([&] {
        for (std::size_t i = 0; i < steps; ++i) update(data, 0.016f);
    });
    double reduce_ms = 0.0;
    double energy = 0.0;
    reduce_ms = cs::bench::measure_ms([&] { energy = kinetic_energy(data); });

    check(data.x.size() == n, "generated count");
    for (std::size_t i = 0; i < data.x.size(); ++i) {
        const double dt = static_cast<double>(0.016f) * static_cast<double>(steps);
        const double roundings=2.0*static_cast<double>(steps)*std::numeric_limits<float>::epsilon();
        const double gamma=roundings/(1.0-roundings);
        const double x_budget=gamma*(std::abs(before_x[i])+std::abs(before_vx[i])*dt)+1e-7;
        const double y_budget=gamma*(std::abs(before_y[i])+std::abs(before_vy[i])*dt)+1e-7;
        check(near(data.x[i], static_cast<double>(before_x[i]) + static_cast<double>(before_vx[i]) * dt, x_budget, 0),
              "x update oracle");
        check(near(data.y[i], static_cast<double>(before_y[i]) + static_cast<double>(before_vy[i]) * dt, y_budget, 0),
              "y update oracle");
    }
    check(n == 0 || energy > 0.0, "energy summary");

    const auto detail = "seed=" + std::to_string(seed) + "; steps=" + std::to_string(steps);
    cs::bench::emit_row("c13_pipeline", "generate", data.x.size(), 1, make_ms, data.x.size(), "includes rng; " + detail);
    cs::bench::emit_row("c13_pipeline", "update", data.x.size(), 1, update_ms, data.x.size(), "position update only; " + detail);
    cs::bench::emit_row("c13_pipeline", "reduce", data.x.size(), 1, reduce_ms, data.x.size(), "energy reduction");
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
