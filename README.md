# MicroRuntimeFramework

`MicroRuntimeFramework` is a small C++23 runtime layer for explicit task execution, bounded queues, cancellation and deterministic shutdown.

It has no third-party runtime dependencies, creates no detached threads and exposes `vosp::runtime::Executor` through a header-only target.

```cpp
#include <vosp/runtime.hpp>

vosp::runtime::Runtime runtime{{4, 1024}};
auto result = runtime.submit([] { return 42; });
const int answer = result.get();
runtime.shutdown(vosp::runtime::ShutdownMode::drain);
```

The queue applies backpressure by waiting for capacity. `cancel_pending` discards only tasks that have not started; active tasks must implement their own `CancellationToken` policy.

## Build

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build -C Release --output-on-failure
```

The public target is `vosp::runtime`. GUI and backend facilities are intentionally outside this module.
