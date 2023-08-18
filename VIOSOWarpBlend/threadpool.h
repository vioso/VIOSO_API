#pragma once
#include <thread>
#include <future>
#include <vector>
#include <memory>
#include <cstdarg>
#include <queue>

class threadpool
{
protected:
	std::atomic_bool join;
	struct Worker {
		std::thread t;
		std::atomic_bool busy;
		Worker(threadpool& pool)
			: busy{ false }
		{
			t = std::thread(threadFn, std::ref(*this), std::ref(pool));
		}
	};

	static void threadFn( Worker& me, threadpool& pool )
	{
		while (!pool.join.load())
		{
			// wait for a job
		}
	}
	struct Task
	{
		std::promise<std::shared_ptr<void>> result;
		std::function<std::shared_ptr<void>()> task;
		Task(Task const&) = delete;
		Task& operator=(Task const&) = delete;
	};

	static void _stdCombine(std::vector<Task>& tasks, std::promise<std::shared_ptr<void>>& promise ) // we just wait and signal
	{
		for (auto& task : tasks)
		{
			task.result.get_future().get();
		}
		promise.set_value(nullptr);
	}
	struct Job
	{
		std::queue<Task> tasks;
		std::function< void(std::queue<Task> const& tasks, std::promise<std::shared_ptr<void>>& promise)> combine; // is called once all tasks of a job are done
		std::promise<std::shared_ptr<void>> result;
		template<typename T>
		std::promise<T>& getPromise()
		{
			combine(tasks, result);
			return *reinterpret_cast<std::promise<T>&>(result);
		}
		Job(Job const&) = delete;
		Job& operator=(Job const&) = delete;
	};
	std::queue<Job> waiting;
	std::queue<Job> busy;

	std::vector<std::shared_ptr<Worker>> pool;
public:
	threadpool(int nThreads = 2 * std::thread::hardware_concurrency()) 
		: join{ false }
	{
		for (int i = 0; i != nThreads; i++)
		{
			//auto p = make_shared<
			//auto w = std::make_shared<Worker>( 
			//pool.push_back( Worker{ std::thread( fn, 
		}
	}
	~threadpool()
	{
		join.store(true);
		for (auto& worker : pool) worker->t.join();
	}

	static bool test();
};


