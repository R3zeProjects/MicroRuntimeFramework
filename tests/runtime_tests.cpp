#ifdef NDEBUG
#  undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <stdexcept>
#include <vosp/runtime.hpp>

int main() {
    using namespace std::chrono_literals;

    vosp::runtime::CancellationSource cancellation;
    assert(!cancellation.stop_requested());
    assert(cancellation.request_stop());
    assert(cancellation.token().stop_requested());

    vosp::runtime::Executor executor{{2, 8}};
    auto value = executor.submit([] { return 42; });
    assert(value.get() == 42);

    auto delayed = executor.submit([] {
        std::this_thread::sleep_for(1ms);
        return 7;
    });
    assert(delayed.get() == 7);
    assert(executor.worker_count() == 2);
    executor.shutdown();
    assert(!executor.accepting());

    bool rejected = false;
    try {
        (void)executor.submit([] {});
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    assert(rejected);

    bool invalid = false;
    try {
        vosp::runtime::Executor invalid_executor{{1, 0}};
    } catch (const std::invalid_argument&) {
        invalid = true;
    }
    assert(invalid);
    return 0;
}
