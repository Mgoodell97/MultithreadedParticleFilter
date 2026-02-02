// Custom Non‑Commercial License

// Copyright (c) 2025 Mgoodell97

// Permission is hereby granted, free of charge, to any individual or
// non‑commercial entity obtaining a copy of this software and associated
// documentation files (the "Software"), to use, copy, modify, merge, publish,
// and distribute the Software for personal, educational, or research purposes,
// subject to the following conditions:

// 1. Commercial Use:
//    Any company, corporation, or organization intending to use the Software
//    must first notify the copyright holder and obtain explicit written
//    permission. Commercial use without such permission is strictly prohibited.

// 2. Unauthorized Commercial Use:
//    If a company is found to be using the Software without prior authorization,
//    the copyright holder is entitled to receive 1% of the company’s gross
//    profits moving forward, enforceable as a licensing fee.

// 3. Artificial Intelligence / Machine Learning Use:
//    If the Software is incorporated into machine learning
//    models, neural networks, generative pre‑trained transformers (GPTs), or similar AI systems,
//    the company deploying such use is solely responsible for compliance with
//    this license. Responsibility cannot be shifted to the provider of training
//    data or third‑party services.

// 4. Attribution:
//    The above copyright notice and this permission notice shall be included in
//    all copies or substantial portions of the Software.

// Disclaimer:
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <mutex>
#include <future>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <cmath>
#include <atomic>

class ThreadPool{
public:
    // Constructor that spawns a number of threads equal to size
    ThreadPool(const int64_t size) : busy_threads(size), m_threads(std::vector<std::thread>(size)), m_shutdown_requested(false)
    {
        m_number_of_threads = size;
        for (int64_t i = 0; i < size; ++i)
        {
            m_threads[i] = std::thread(ThreadWorker(this));
        }
    }

    ~ThreadPool()
    {
        Shutdown();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&)      = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&)      = delete;

    int getQueueSize()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

    void waitUntilAllTasksFinished()
    {
        while (true)
        {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (m_queue.empty() && busy_threads.load() == 0)
                {
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }

    std::vector<std::vector<int64_t>> getSplitWorkIndices(const int64_t input_size) 
    {
        const int64_t p = std::min(m_number_of_threads, input_size);
        std::vector<std::vector<int64_t>> result;
        for (int64_t i = 0; i < p; ++i)
        {
            std::vector<int64_t> chunk = {};

            int64_t start_index = std::floor((i * input_size)/(p));
            int64_t end_index   = std::floor(((i+1) * input_size)/(p));
            for (int64_t j = start_index; j < end_index; ++j)
            {
                chunk.push_back(j);
            }
            result.push_back(chunk);
        }

        return result;
    }

    // You need to profile if this is worth it
    template<typename T>
    void copyVector(std::vector<T>& output_vec, const std::vector<T>& input_vec)
    {
        auto copyLocal = [](const std::vector<T>& input_vec, std::vector<T>& output_vec, const std::vector<int64_t>& indices_chunk) 
        { 
            for (int64_t i = indices_chunk.front(); i <= indices_chunk.back(); ++i)
            {
                output_vec[i] = input_vec[i];
            }
        };

        // Split work evenly
        std::vector<std::vector<int64_t>> mutation_indicies_chunks = getSplitWorkIndices(input_vec.size());

        std::vector<std::future<void>> futures;
        for (const auto& indices_chunk : mutation_indicies_chunks)
        {
            auto future = this->AddTask(copyLocal, std::ref(input_vec), std::ref(output_vec), std::ref(indices_chunk));
            futures.push_back(std::move(future));
        }
        this->waitUntilAllTasksFinished();
    }

    // Template by F (function type) and args because we have an unknown amount of args
    // Takes any number of parameters.  We use auto since we don't not the parameter types at this point in time.
    // AddTask returns a future of the decltype of the function 
    // The function returns a std::future that will eventually contain the result of f(args...).
    // decltype() deduces the return type of f when called with args...
    template<typename F, typename... Args>
    auto AddTask(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
    {
        // std::forward<F>(f) preserve the value category (lvalue/rvalue) of the original arguments with effiecent moving
        // std::bind() creates a callable object that encapsulates the function f and its arguments args...
        auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

        // Cleaver way to create a packaged task regarrdless of function that goes into the same queue
        // decltype(f(args...)) - Deduces the return type of calling f(args...).
        // std::packaged_task - std::packaged_task<R()> is a wrapper around a callable that returns type R.
        // func - callable object with no args since we bount the args with std::bind
        // make_shared is here to make it copyable otherwise it would be a problem since packaged_task is move only
        // we need to be able to copy it to put it into the lambda
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

        auto wrapper_func = [task_ptr]() { (*task_ptr)(); };

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(wrapper_func);
            // Wake up one thread if its waiting
            m_condition_variable.notify_one(); // Notify threads by using the condition variable that there is a new task
        }

        return task_ptr->get_future();
    }

    // Waits until threads finish their current task and shutdowns the pool
    void Shutdown()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shutdown_requested = true;
            m_condition_variable.notify_all();
        }

        for (int64_t i = 0; i < m_threads.size(); ++i)
        {
            if (m_threads[i].joinable())
            {
                m_threads[i].join();
            }
        }
    }

    class ThreadWorker
    {
      public:
        ThreadWorker(ThreadPool* pool) : thread_pool(pool)
        {
        }

        void operator()() // Function the thread work will immidiatly execute when it is spawned regaredless of tasks
        {
            std::unique_lock<std::mutex> lock(thread_pool->m_mutex); // Inquire the lock
            while(!thread_pool->m_shutdown_requested || (thread_pool->m_shutdown_requested && !thread_pool->m_queue.empty()))
            { // Keep doing something until the program is shutdown
                thread_pool->busy_threads--;
                thread_pool->m_condition_variable.wait(lock, [this]{ // Basically the thread goes to sleep until we need to shutdown or there is a new task to do
                    // There can be spurious wakeups so we need to check the condition in a loop
                    // DIFFICULT BUG TO FIND!
                    return this->thread_pool->m_shutdown_requested || !this->thread_pool->m_queue.empty();
                });
                thread_pool->busy_threads++;

                if (!this->thread_pool->m_queue.empty()) // If the queue is not empty then we have work to do!
                {
                    auto func = thread_pool->m_queue.front();
                    thread_pool->m_queue.pop();

                    lock.unlock(); // We only unlock while doing the work everything above has a lock
                    func();
                    lock.lock(); // Relock
                }
            }
        }
    private:
        // The work needs ascess to the thread pool since it needs access to the queue, mutex, etc
        ThreadPool* thread_pool;
    };

  public:
    std::atomic<int64_t> busy_threads{0};
    int64_t m_number_of_threads;

  private:
    mutable std::mutex m_mutex;
    std::condition_variable m_condition_variable;

    std::vector<std::thread> m_threads;
    bool m_shutdown_requested{false};

    std::queue<std::function<void()>> m_queue;
};


