#include <iostream>
#include <vosp/runtime.hpp>

int main() {
    vosp::runtime::Runtime runtime{{2, 64}};
    auto result = runtime.submit([] { return "task completed"; });
    std::cout << result.get() << '\n';
    runtime.shutdown(vosp::runtime::ShutdownMode::drain);
}
