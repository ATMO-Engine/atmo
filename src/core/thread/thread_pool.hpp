#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>
#include <vector>

namespace atmo::core
{
    class TaskHandle
    {
    public:
        void cancel() noexcept;
        bool isCancelled() const noexcept;

    private:
        friend class ThreadPool;

        struct State {
            std::atomic<bool> cancelled{ false };
        };

        explicit TaskHandle(std::shared_ptr<State> state) noexcept;
        std::shared_ptr<State> m_state;
    };

    class ThreadPool
    {
    public:
        static ThreadPool &Instance();

        void submit(std::function<void()> fn);
        [[nodiscard]] TaskHandle repeat(std::function<void()> fn, int count, std::chrono::milliseconds delay = std::chrono::milliseconds{ 0 });

    private:
        ThreadPool();

        struct TaskItem {
            std::function<void()> fn;
            std::shared_ptr<TaskHandle::State> control;
            int remaining;
            std::chrono::steady_clock::duration delay;
        };

        struct WorkQueue {
            void push(TaskItem item);
            std::optional<TaskItem> pop();
            std::optional<TaskItem> steal();

            mutable std::mutex m_mutex;
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

        void workerLoop(std::stop_token stoken, std::size_t idx);
        void schedulerLoop(std::stop_token stoken);

        std::optional<TaskItem> tryGetTask(std::size_t idx);
        void enqueue(TaskItem item);
        void reschedule(TaskItem item);
        bool hasWork() const;

        std::size_t m_worker_count;
        std::vector<std::unique_ptr<WorkQueue>> m_queues;
        std::atomic<std::size_t> m_dispatch_idx{ 0 };

        std::mutex m_work_mutex;
        std::condition_variable_any m_work_cv;

        mutable std::mutex m_timer_mutex;
        std::condition_variable_any m_sched_cv;
        std::vector<TimerEntry> m_timer_heap;

        std::vector<std::jthread> m_workers;
        std::jthread m_scheduler;
    };
} // namespace atmo::core
