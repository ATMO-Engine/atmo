#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template <typename T>
class SafeQueue
{
    public:
        SafeQueue() = default;
        ~SafeQueue() { shutdown(); }

        SafeQueue(const SafeQueue &) = delete;
        SafeQueue &operator=(const SafeQueue &) = delete;

        SafeQueue(SafeQueue &&other) noexcept
        {
            std::lock_guard<std::mutex> lock(other.mtx);
            queue = std::move(other.queue);
        }

        SafeQueue &operator=(SafeQueue &&other) noexcept
        {
            if (this != &other) {
                std::lock_guard<std::mutex> lock(other.mtx);
                queue = std::move(other.queue);
            }
            return *this;
        }

        void enqueue(T value)
        {
            {
                std::lock_guard<std::mutex> lock(mtx);
                queue.push(std::move(value));
            }
            cv.notify_one();
        }

        T dequeue()
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return !queue.empty() || stop; });
            if (stop)
                throw std::runtime_error("Queue stopped.");

            T value = std::move(queue.front());
            queue.pop();
            return value;
        }

        std::optional<T> try_dequeue()
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (queue.empty())
                return std::nullopt;

            T value = std::move(queue.front());
            queue.pop();
            return value;
        }

        bool empty() const
        {
            std::lock_guard<std::mutex> lock(mtx);
            return queue.empty();
        }

        void shutdown()
        {
            {
                std::lock_guard<std::mutex> lock(mtx);
                stop = true;
            }
            cv.notify_all();
        }

    private:
        std::queue<T> queue;
        mutable std::mutex mtx;
        std::condition_variable cv;
        bool stop = false;
};
