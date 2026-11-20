#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <source_location>
#include <stop_token>
#include <thread>
#include <vector>

#include "impl/profiler.hpp"

namespace atmo::core
{
    class TaskHandle
    {
    public:
        void cancel() noexcept;
        bool isCancelled() const noexcept;
        void wait();

    private:
        friend class ThreadPool;

        struct State {
            std::atomic<bool> cancelled{ false };
            std::mutex mutex;
            std::condition_variable cv;
            bool done = false;
        };

        explicit TaskHandle(std::shared_ptr<State> state) noexcept;
        std::shared_ptr<State> m_state;
    };

    struct TaskHandleGroup {
        std::vector<TaskHandle> tasks;
    };

    class ThreadPool
    {
    public:
        static ThreadPool &Instance();

        /// @param origin Call site, shown on the task's profiler zone. Leave defaulted.
        TaskHandle submit(std::function<void()> fn, std::source_location origin = std::source_location::current());
        TaskHandle repeat(
            std::function<void()> fn,
            int count,
            std::chrono::milliseconds delay = std::chrono::milliseconds{ 0 },
            std::source_location origin = std::source_location::current());

        std::size_t getWorkerCount() const;

    private:
        ThreadPool();

        struct TaskItem {
            std::function<void()> fn;
            std::shared_ptr<TaskHandle::State> control;
            int remaining;
            std::chrono::steady_clock::duration delay;
            std::source_location origin;
            /// Set on enqueue in profiling builds, to measure queue latency.
            std::chrono::steady_clock::time_point enqueued_at;
        };

        struct WorkQueue {
            void push(TaskItem item);
            std::optional<TaskItem> pop();
            std::optional<TaskItem> steal();

            mutable ATMO_PROFILE_LOCKABLE_N(std::mutex, m_mutex, "ThreadPool::WorkQueue");
            std::deque<TaskItem> m_tasks;
        };

        struct TimerEntry {
            std::chrono::steady_clock::time_point when;
            TaskItem item;

            bool operator>(const TimerEntry &other) const noexcept
            {
                return when > other.when;
            }
        };

        void runTask(TaskItem &task);
        void workerLoop(std::stop_token stoken, std::size_t idx);
        void schedulerLoop(std::stop_token stoken);

        std::optional<TaskItem> tryGetTask(std::size_t idx);
        void enqueue(TaskItem item);
        void reschedule(TaskItem item);
        bool hasWork() const;
        static void FinishTask(const std::shared_ptr<TaskHandle::State> &control);

        std::size_t m_worker_count;
        std::vector<std::unique_ptr<WorkQueue>> m_queues;
        std::atomic<std::size_t> m_dispatch_idx{ 0 };

        ATMO_PROFILE_LOCKABLE_N(std::mutex, m_work_mutex, "ThreadPool::Work");
        std::condition_variable_any m_work_cv;

        mutable ATMO_PROFILE_LOCKABLE_N(std::mutex, m_timer_mutex, "ThreadPool::Timer");
        std::condition_variable_any m_sched_cv;
        std::vector<TimerEntry> m_timer_heap;

        std::atomic<std::int64_t> m_active_workers{ 0 };

        std::vector<std::jthread> m_workers;
        std::jthread m_scheduler;
    };
} // namespace atmo::core
