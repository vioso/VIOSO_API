#pragma once
#include <thread>
#include <future>
#include <vector>
#include <memory>
#include <cstdarg>
#include <queue>

#define PAT_NOCPY_MOVE( clsname ) \
clsname(clsname const&) = delete; \
clsname& operator=(clsname const&) = delete; \
clsname(clsname&&) = default; \
clsname& operator=(clsname&&) = default

class threadpool
{
protected:
	static unique_ptr<threadpool> _instance;
	std::atomic_bool join;
	std::mutex m;
	std::condition_variable cv;

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
	}


	struct TaskBase
	{
	protected:
		function<void()> f;
	public:
		PAT_NOCPY_MOVE(TaskBase);
		TaskBase() {};
		TaskBase(function<void()> fn) : f(fn) {};
		virtual ~TaskBase() {};
		void invoke() { f(); }
	};

	template< typename _Ty>
	class Task : public TaskBase
	{
	protected:
		promise<_Ty> p;

	public:
		Task(function<_Ty()> func) : TaskBase(
		)
		{
			f = bind([](function<_Ty()> fun, promise<_Ty>* pr) { pr->set_value(fun()); }, func, &p);
		}

		PAT_NOCPY_MOVE(Task);

		future<_Ty> getFuture()
		{
			return p.get_future();
		}
	};

	struct Worker {
		std::thread t;
		std::atomic_bool busy;
		Worker(threadpool& pool) : busy{ false } { t = std::thread(threadFn, std::ref(*this), std::ref(pool)); }
		~Worker() { if( t.joinable() ) t.join(); } // TODO: see if join is true and nobody is busy...
	};

	static void threadFn(Worker& me, threadpool& pool )
	{
		while (1)
		{
			{
				std::unique_lock lk(m);
				cv.wait(lk, [&pool]() { return pool.join && !pool.waiting.empty() && pool.waiting.front().toDo });
				if (pool.join)
					break;

				//cv.wait( []{ 
			}
			// wait for a job
		}
	}

	//static void _stdCombine(std::vector<Task>& tasks, std::promise<std::shared_ptr<void>>& promise ) // we just wait and signal
	//{
	//	for (auto& task : tasks)
	//	{
	//		task.result.get_future().get();
	//	}
	//	promise.set_value(nullptr);
	//}

	struct JobBase
	{
	};

	template< typename _Ty >
	struct Job : JobBase
	{
		//std::function< void(std::queue<Task> const& tasks, std::promise<std::shared_ptr<void>>& promise)> combine; // is called once all tasks of a job are done
		std::promise<std::shared_ptr<Ty>> result;
		std::vector<Task<_Ty>> tasks;
		std::size_t toDo;

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
	std::vector<Worker> pool;
public:
	static threadpool& instance()
	{
		return *_instance;
	};
	static bool init(int nThreads = 2 * std::thread::hardware_concurrency())
	{
		_instance = make_unique<threadpool>(nThreads);
	}
	static bool test();
};


