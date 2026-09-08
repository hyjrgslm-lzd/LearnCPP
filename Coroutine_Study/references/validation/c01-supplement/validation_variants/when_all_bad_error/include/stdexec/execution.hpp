#pragma once
#include <exception>
namespace stdexec {
template <typename T>
struct just_sender { T value; };
struct error_sender { std::exception_ptr error; };
struct stopped_sender { bool stopped = true; };
template <typename T>
just_sender<T> just(T value) { return {value}; }
inline error_sender just_error(std::exception_ptr error) { return {error}; }
inline stopped_sender just_stopped() { return {}; }
} // namespace stdexec
