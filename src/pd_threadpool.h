#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
typedef std::function<void()> Task;


class ThreadPool {
public:
    ThreadPool() = default;
    ThreadPool(ThreadPool&&) = default;
    explicit ThreadPool(size_t size);
    ~ThreadPool();

    template <class Fn, class... Args>
    void submit(Fn&& fn, Args&&... args);

private:
    struct Pool {
        std::mutex mMutex;
        std::condition_variable mCV;
        bool mIsShutdown = false;
        std::queue<Task> taskQueue;
    };
    std::shared_ptr<Pool> mPool;
};


ThreadPool::~ThreadPool() {
    if (mPool) {
        {
            std::lock_guard<std::mutex> lck(mPool->mMutex);
            mPool->mIsShutdown = true;
        }
        mPool->mCV.notify_all();
    }
}


template <class Fn, class... Args>
void ThreadPool::submit(Fn&& fn, Args&&... args) {
    auto task = std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...);
    {
        std::lock_guard<std::mutex> lck(mPool->mMutex);
        mPool->taskQueue.emplace(task);
    }
    mPool->mCV.notify_one();
}

ThreadPool::ThreadPool(size_t size)
    : mPool(std::make_shared<Pool>()) {
    for (size_t i = 0; i < size; ++i) {
        std::thread([p = mPool] {
            std::unique_lock<std::mutex> lck(p->mMutex);
            for (;;) {
                if (!p->taskQueue.empty()) {
                    auto task = std::move(p->taskQueue.front());
                    p->taskQueue.pop();
                    lck.unlock();
                    task();
                    lck.lock();
                } else if (p->mIsShutdown) {
                    break;
                } else {
                    p->mCV.wait(lck);
                }
            }
        }).detach();
    }
}