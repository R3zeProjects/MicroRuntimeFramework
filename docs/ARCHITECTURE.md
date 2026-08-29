# Architecture

The runtime owns its worker threads and its bounded FIFO queue. Submission
waits for capacity, so overload is visible to the caller instead of silently
creating unbounded memory pressure. Shutdown closes admission first, then
workers finish or discard pending work according to `ShutdownMode` and return.

The module is intentionally independent of the other vosp repositories. It
uses only C++23 and standard-library synchronization primitives. A future
integration layer may compose this executor with workflow and service modules.
