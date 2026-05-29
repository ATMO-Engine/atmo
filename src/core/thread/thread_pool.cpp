#include "thread_pool.hpp"

#include <algorithm>

namespace atmo::core
{
    TaskHandle::TaskHandle(std::shared_ptr<State> state) noexcept : m_state(std::move(state)) {}

    void TaskHandle::cancel() noexcept
    {
        m_state->cancelled.store(true, std::memory_order_relaxed);
    }

    bool TaskHandle::isCancelled() const noexcept
    {
        return m_state->cancelled.load(std::memory_order_relaxed);
    }

    void ThreadPool::WorkQueue::push(TaskItem item)
    {
        std::lock_guard lock(m_mutex);
        m_tasks.push_back(std::move(item));
    }

    std::optional<ThreadPool::TaskItem> ThreadPool::WorkQueue::pop()
    {
        std::lock_guard lock(m_mutex);
        if (m_tasks.empty())
            return std::nullopt;
        auto item = std::move(m_tasks.back());
        m_tasks.pop_back();
        return item;
    }

    std::optional<ThreadPool::TaskItem> ThreadPool::WorkQueue::steal()
    {
        std::lock_guard lock(m_mutex);
        if (m_tasks.empty())
            return std::nullopt;
        auto item = std::move(m_tasks.front());
        m_tasks.pop_front();
        return item;
    }

    ThreadPool::ThreadPool() : m_worker_count(std::max(1u, std::thread::hardware_concurrency()))
    {
        m_queues.reserve(m_worker_count);
        for (std::size_t i = 0; i < m_worker_count; ++i) m_queues.push_back(std::make_unique<WorkQueue>());

        m_scheduler = std::jthread([this](std::stop_token st) { schedulerLoop(st); });

        m_workers.reserve(m_worker_count);
        for (std::size_t i = 0; i < m_worker_count; ++i) m_workers.emplace_back([this, i](std::stop_token st) { workerLoop(st, i); });
    }

    ThreadPool &ThreadPool::Instance()
    {
        static ThreadPool pool;
        return pool;
    }

    void ThreadPool::submit(std::function<void()> fn)
    {
        enqueue({ .fn = std::move(fn), .control = nullptr, .remaining = 1, .delay = {} });
    }

    TaskHandle ThreadPool::repeat(std::function<void()> fn, int count, std::chrono::milliseconds delay)
    {
        auto state = std::make_shared<TaskHandle::State>();
        enqueue({ .fn = std::move(fn), .control = state, .remaining = count, .delay = delay });
        return TaskHandle(std::move(state));
    }

    void ThreadPool::enqueue(TaskItem item)
    {
        std::size_t idx = m_dispatch_idx.fetch_add(1, std::memory_order_relaxed) % m_worker_count;
        m_queues[idx]->push(std::move(item));
        m_work_cv.notify_one();
    }

    void ThreadPool::reschedule(TaskItem item)
    {
        auto when = std::chrono::steady_clock::now() + item.delay;
        {
            std::lock_guard lock(m_timer_mutex);
            m_timer_heap.push_back({ when, std::move(item) });
            std::push_heap(m_timer_heap.begin(), m_timer_heap.end(), std::greater<TimerEntry>{});
        }
        m_sched_cv.notify_one();
    }

    bool ThreadPool::hasWork() const
    {
        for (const auto &q : m_queues) {
            std::lock_guard lock(q->m_mutex);
            if (!q->m_tasks.empty())
                return true;
        }
        return false;
    }

    std::optional<ThreadPool::TaskItem> ThreadPool::tryGetTask(std::size_t idx)
    {
        if (auto task = m_queues[idx]->pop())
            return task;

        for (std::size_t i = 1; i < m_worker_count; ++i) {
            if (auto task = m_queues[(idx + i) % m_worker_count]->steal())
                return task;
        }

        return std::nullopt;
    }

    void ThreadPool::workerLoop(std::stop_token stoken, std::size_t idx)
    {
        while (!stoken.stop_requested()) {
            auto task = tryGetTask(idx);

            if (!task) {
                std::unique_lock lock(m_work_mutex);
                m_work_cv.wait(lock, stoken, [this] { return hasWork(); });
                continue;
            }

            if (task->control && task->control->cancelled.load(std::memory_order_relaxed))
                continue;

            task->fn();

            bool infinite = task->remaining == -1;
            bool has_more = infinite || task->remaining > 1;

            if (!has_more)
                continue;

            if (task->control && task->control->cancelled.load(std::memory_order_relaxed))
                continue;

            TaskItem next{
                .fn = task->fn,
                .control = task->control,
                .remaining = infinite ? -1 : task->remaining - 1,
                .delay = task->delay,
            };

            if (next.delay.count() > 0)
                reschedule(std::move(next));
            else
                enqueue(std::move(next));
        }
    }

    void ThreadPool::schedulerLoop(std::stop_token stoken)
    {
        while (!stoken.stop_requested()) {
            std::unique_lock lock(m_timer_mutex);

            if (m_timer_heap.empty()) {
                m_sched_cv.wait(lock, stoken, [this] { return !m_timer_heap.empty(); });
                continue;
            }

            auto next_when = m_timer_heap.front().when;
            if (next_when > std::chrono::steady_clock::now()) {
                m_sched_cv.wait_until(lock, stoken, next_when, [this, &next_when] { return m_timer_heap.empty() || m_timer_heap.front().when < next_when; });
                continue;
            }

            auto now = std::chrono::steady_clock::now();
            std::vector<TaskItem> ready;
            while (!m_timer_heap.empty() && m_timer_heap.front().when <= now) {
                std::pop_heap(m_timer_heap.begin(), m_timer_heap.end(), std::greater<TimerEntry>{});
                ready.push_back(std::move(m_timer_heap.back().item));
                m_timer_heap.pop_back();
            }
            lock.unlock();

            for (auto &item : ready) enqueue(std::move(item));
        }
    }
} // namespace atmo::core
