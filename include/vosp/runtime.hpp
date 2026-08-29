/**
 * @file runtime.hpp
 * @brief Small explicit executor and cancellation primitives for vosp.
 */
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <future>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace vosp::runtime {

/** @brief Maximum number of workers accepted by the public runtime API. */
inline constexpr std::size_t max_worker_count = 1024;

/** @brief Controls what happens to tasks that are still queued during shutdown. */
enum class ShutdownMode : std::uint8_t { drain, cancel_pending };

/** @brief Lightweight read-only cancellation token. */
class CancellationToken final {
public:
    CancellationToken() = default;

    [[nodiscard]] bool stop_requested() const noexcept {
        return token_.stop_requested();
    }

    [[nodiscard]] std::stop_token native_token() const noexcept { return token_; }

private:
    friend class CancellationSource;
    explicit CancellationToken(std::stop_token token) noexcept : token_(token) {}
    std::stop_token token_{};
};

/** @brief Owns a cancellation request that can be passed to an operation. */
class CancellationSource final {
public:
    [[nodiscard]] CancellationToken token() const noexcept {
        return CancellationToken{source_.get_token()};
    }

    [[nodiscard]] bool request_stop() noexcept { return source_.request_stop(); }

    [[nodiscard]] bool stop_requested() const noexcept {
        return source_.stop_requested();
    }

private:
    std::stop_source source_{};
};

/** @brief Configuration for the bounded executor. */
struct ExecutorOptions {
    std::size_t worker_count{};
    std::size_t queue_capacity{1024};
};

/**
 * @brief Bounded FIFO executor with explicit, deterministic shutdown.
 *
 * Submitting to a full queue waits for capacity. Shutdown first stops new
 * submissions, then workers finish the selected queue policy, and finally
 * become idle. No worker is detached and no hidden global thread is created.
 */
class Executor final {
public:
    explicit Executor(ExecutorOptions options = {})
        : queue_capacity_(options.queue_capacity),
          workers_(normalise_worker_count(options.worker_count)) {
        if (queue_capacity_ == 0) {
            throw std::invalid_argument{"vosp::runtime::Executor queue capacity must be positive"};
        }
        for (std::size_t index = 0; index < workers_.size(); ++index) {
            workers_[index] = std::jthread{[this](std::stop_token) { worker_loop(); }};
        }
    }

    Executor(const Executor&) = delete;
    Executor& operator=(const Executor&) = delete;

    ~Executor() { shutdown(ShutdownMode::drain); }

    /**
     * @brief Queues a callable and returns its result future.
     * @throws std::runtime_error when shutdown has started.
     */
    template<typename Callable>
    [[nodiscard]] auto submit(Callable&& callable)
        -> std::future<std::invoke_result_t<Callable&>> {
        using Result = std::invoke_result_t<Callable&>;
        auto operation = std::make_shared<std::packaged_task<Result()>>(
            std::forward<Callable>(callable));
        auto result = operation->get_future();

        {
            std::unique_lock lock{mutex_};
            not_full_.wait(lock, [this] {
                return !accepting_ || queue_.size() < queue_capacity_;
            });
            if (!accepting_) {
                throw std::runtime_error{"vosp::runtime::Executor is shutting down"};
            }
            queue_.emplace_back([operation] { (*operation)(); });
        }
        not_empty_.notify_one();
        return result;
    }

    /** @brief Prevents new work and waits until all active work has stopped. */
    void shutdown(ShutdownMode mode = ShutdownMode::drain) noexcept {
        {
            std::lock_guard lock{mutex_};
            if (!accepting_ && active_workers_ == 0) return;
            accepting_ = false;
            if (mode == ShutdownMode::cancel_pending) queue_.clear();
        }
        not_empty_.notify_all();
        not_full_.notify_all();
        std::unique_lock lock{mutex_};
        stopped_.wait(lock, [this] { return active_workers_ == 0; });
    }

    [[nodiscard]] bool accepting() const noexcept {
        std::lock_guard lock{mutex_};
        return accepting_;
    }

    [[nodiscard]] std::size_t pending() const noexcept {
        std::lock_guard lock{mutex_};
        return queue_.size();
    }

    [[nodiscard]] std::size_t worker_count() const noexcept { return workers_.size(); }

private:
    using Task = std::function<void()>;

    static std::size_t normalise_worker_count(std::size_t requested) noexcept {
        const auto detected = static_cast<std::size_t>(std::thread::hardware_concurrency());
        const auto selected = requested == 0 ? (detected == 0 ? 1U : detected) : requested;
        return selected > max_worker_count ? max_worker_count : selected;
    }

    void worker_loop() noexcept {
        {
            std::lock_guard lock{mutex_};
            ++active_workers_;
        }
        for (;;) {
            Task task;
            {
                std::unique_lock lock{mutex_};
                not_empty_.wait(lock, [this] { return !queue_.empty() || !accepting_; });
                if (queue_.empty() && !accepting_) break;
                task = std::move(queue_.front());
                queue_.pop_front();
            }
            not_full_.notify_one();
            task();
        }
        {
            std::lock_guard lock{mutex_};
            --active_workers_;
        }
        stopped_.notify_all();
    }

    const std::size_t queue_capacity_;
    mutable std::mutex mutex_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::condition_variable stopped_;
    std::deque<Task> queue_;
    std::vector<std::jthread> workers_;
    std::size_t active_workers_{};
    bool accepting_{true};
};

/** @brief Named facade for the default application runtime. */
using Runtime = Executor;

} // namespace vosp::runtime
