# C08 queue revision measurements

This directory is reserved for the queue revision sampling window.

Current status: no formal samples collected in this slice. See `topics/performance/c08-revision-queue-evidence.md` for the command list and attribution plan.

`queue_diagnostics.csv` is the r1 author self-check from `queue_diagnostics.cpp`. It records public queue API calls and completed elements only; it is not a timing sample.

`queue_diagnostics-r2.csv` is the r2 author self-check after adding diagnostic-only header counters. It records public API calls, instrumented mutex acquisitions, SPSC remote-index loads, and isolated hot-region operator-new calls.
