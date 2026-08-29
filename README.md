`MicroRuntimeFramework` — компактный runtime-слой C++23 для явного выполнения задач, ограниченной очереди, отмены и детерминированного завершения.

Модуль не имеет сторонних runtime-зависимостей, не создаёт detached-потоки и предоставляет header-only цель `vosp::runtime::Executor`.

```cpp
#include <vosp/runtime.hpp>

vosp::runtime::Runtime runtime{{4, 1024}};
auto result = runtime.submit([] { return 42; });
const int answer = result.get();
runtime.shutdown(vosp::runtime::ShutdownMode::drain);
```

При заполненной очереди submission ожидает свободное место. `cancel_pending` удаляет только ещё не начатые задачи; активные задачи должны самостоятельно учитывать `CancellationToken`.

## Сборка

```text
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build -C Release --output-on-failure
```

Публичная цель — `vosp::runtime`. GUI и backend намеренно находятся вне этого модуля.
