#pragma once
// [CMN-T34] timer.cuh -- CUDA event timer and host-side timer
//
// [CMN-T35] Timing strategy:
//   CudaEventTimer   : precise device-side timing (GPU hardware timestamps;
//                      excludes CPU->GPU scheduling jitter).
//                      Suitable for kernels, memcpy, stream regions.
//                      Resolution ~0.5 us.
//   HostTimer        : host-side steady_clock; suitable for end-to-end
//                      wall-clock measurement, including launch latency
//                      and CPU-side sync cost.
//   ScopedCudaTimer  : RAII wrapper that auto-prints elapsed time on
//                      destruction. Suitable for quick profiling probes.

#include <cstdio>
#include <chrono>
#include <string>
#include <cuda_runtime.h>

// --------------------------------------------------------------------------
// [CMN-T36] CudaEventTimer -- device-side timer based on cudaEvent
// --------------------------------------------------------------------------
struct CudaEventTimer {
    CudaEventTimer() {
        cudaEventCreate(&start_);
        cudaEventCreate(&stop_);
    }

    // [CMN-T37] Forbid copy (event handles cannot be duplicated).
    CudaEventTimer(const CudaEventTimer&)            = delete;
    CudaEventTimer& operator=(const CudaEventTimer&) = delete;

    ~CudaEventTimer() {
        cudaEventDestroy(start_);
        cudaEventDestroy(stop_);
    }

    // [CMN-T38] Record the start timestamp on the given stream.
    void start(cudaStream_t stream = 0) {
        cudaEventRecord(start_, stream);
    }

    // [CMN-T39] Record the stop timestamp and CPU-side wait until done.
    void stop(cudaStream_t stream = 0) {
        cudaEventRecord(stop_, stream);
        cudaEventSynchronize(stop_);
    }

    // [CMN-T40] Return milliseconds between start and stop (call stop() first).
    float elapsed_ms() const {
        float ms = 0.0f;
        cudaEventElapsedTime(&ms, start_, stop_);
        return ms;
    }

private:
    cudaEvent_t start_{};
    cudaEvent_t stop_{};
};

// --------------------------------------------------------------------------
// [CMN-T41] HostTimer -- host-side timer based on std::chrono::steady_clock
// --------------------------------------------------------------------------
struct HostTimer {
    HostTimer() : begin_(clock_t::now()) {}

    // [CMN-T42] Reset the start point.
    void reset() { begin_ = clock_t::now(); }

    // [CMN-T43] Return milliseconds since construction (or last reset).
    double ms() const {
        using namespace std::chrono;
        return duration<double, std::milli>(clock_t::now() - begin_).count();
    }

private:
    using clock_t = std::chrono::steady_clock;
    clock_t::time_point begin_;
};

// --------------------------------------------------------------------------
// [CMN-T44] ScopedCudaTimer -- RAII timer; on destruction prints label + time.
// [CMN-T45] Usage: declare at top of the scope to time:
//   ScopedCudaTimer t("my_kernel");
// On scope exit it prints: [my_kernel] 1.234 ms
// --------------------------------------------------------------------------
struct ScopedCudaTimer {
    explicit ScopedCudaTimer(const char* label, cudaStream_t stream = 0)
        : label_(label), stream_(stream) {
        timer_.start(stream_);
    }

    // [CMN-T46] Forbid copy.
    ScopedCudaTimer(const ScopedCudaTimer&)            = delete;
    ScopedCudaTimer& operator=(const ScopedCudaTimer&) = delete;

    ~ScopedCudaTimer() {
        timer_.stop(stream_);
        std::fprintf(stdout, "[%s] %.3f ms\n", label_, timer_.elapsed_ms());
    }

private:
    const char*    label_;
    cudaStream_t   stream_;
    CudaEventTimer timer_;
};
