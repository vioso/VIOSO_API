// VIOSO API
// http://github.com/vioso/vioso_api
// Copyright VIOSO GmbH 2015-2026
// This code is published under BSD 2-Clause license
// see LICENSE.md
// https://opensource.org/license/bsd-2-clause

#pragma once
#include <thread>
#include <future>
#include <vector>
#include <memory>
#include <cstdarg>
#include <queue>
#include <exception>

#define PAT_NOCPY_MOVE( clsname ) \
clsname(clsname const&) = delete; \
clsname& operator=(clsname const&) = delete; \
clsname(clsname&&) = default; \
clsname& operator=(clsname&&) = default

class threadpool
{
protected:
	static std::unique_ptr<threadpool> _instance;
	std::atomic_bool join;
	std::mutex m;
	std::condition_variable cv;

	struct Worker {
		std::thread t;
		Worker() {};
		Worker(threadpool* pool) : t( std::thread( threadFn, &t, std::ref( *pool ) ) ){}
		~Worker() { if( t.joinable() ) t.join(); }
		PAT_NOCPY_MOVE( Worker );
	};

	static void threadFn( std::thread* t, threadpool& pool )
	{
		while (1)
		{
			std::function<void()> fn;
			{
				// wait for a job and task
				std::unique_lock lk(pool.m);

				pool.cv.wait( lk, [&pool]() { return pool.join || !pool.waiting.empty(); } );

				if( pool.join )
					break;

				// if all tasks are done
				if( !pool.waiting.front().toDo )
				{
					pool.waiting.front().combineFn(); // this also sets the promise value
					pool.waiting.pop();
					if( pool.waiting.empty() )
						continue;
				}

				fn = pool.waiting.front().tasks[--pool.waiting.front().toDo];
			}
			// notify next
			pool.cv.notify_one();
			// invoke task
			fn();
		}
	}

	struct JobBase
	{
		std::function<void()> combineFn;
		std::size_t toDo;
		std::vector<std::function<void()>> tasks;

		JobBase( std::vector<std::function<void()>> const& taskList = {} )
			: toDo( taskList.size() )
			, tasks( taskList )
		{}

		PAT_NOCPY_MOVE( JobBase );

		virtual ~JobBase() {}
	};

	template< typename _Ty >
	struct Job : JobBase
	{
		std::shared_ptr<std::promise<_Ty>> result;
		std::shared_ptr<std::vector<std::future<_Ty>>> futures;

		Job() {}
		Job( std::vector<std::function<_Ty()>> const& taskList = {}, std::function<_Ty( _Ty const&, _Ty const& )> const& comb = []( _Ty const& a, _Ty const& b ) { return a + b; } )
		{
			if( taskList.empty() )
				throw( std::runtime_error( "taskList cannot be empty" ) );

			toDo = taskList.size();
			result = std::make_shared<std::promise<_Ty>>();
			futures = std::make_shared<std::vector<std::future<_Ty>>>();
			for( auto const& fn : taskList )
			{
				auto task = std::make_shared<std::packaged_task < _Ty()>>( fn );
				futures->emplace_back( task->get_future() );
				tasks.emplace_back( [task]() {
					( *task )( );
				} );
			}
			// there are no tasks, we are actually ready;

			combineFn = [promise = result, futs = futures, comb]() {
				_Ty res{};
				for( auto& fut : *futs )
				{
					res = comb( res, fut.get() );
				}
				promise->set_value( res );
			};
		}

		typename std::future<_Ty> get_future() { return result->get_future(); }

		virtual ~Job() {}

		PAT_NOCPY_MOVE( Job );
	};

	template<>
	struct Job<void> : JobBase
	{
		std::shared_ptr<std::promise<void>> result;
		std::shared_ptr<std::vector<std::future<void>>> futures;

		Job() {}
		Job( std::vector<std::function<void()>> const& taskList = {} )
		{
			if( taskList.empty() )
				throw( std::runtime_error( "taskList cannot be empty" ) );

			toDo = taskList.size();
			result = std::make_shared<std::promise<void>>();
			futures = std::make_shared<std::vector<std::future<void>>>();
			for( auto const& fn : taskList )
			{
				auto task = std::make_shared<std::packaged_task < void()>>( fn );
				futures->emplace_back( task->get_future() );
				tasks.emplace_back( [task]() {
					( *task )( );
				} );
			}
			// there are no tasks, we are actually ready;

			combineFn = [promise = result, futs = futures]() {
				for( auto& fut : *futs )
				{
					fut.get();
				}
				promise->set_value();
			};
		}

		typename std::future<void> get_future() { return result->get_future(); }
	};

	std::queue<JobBase> waiting;
	std::vector<Worker> pool;

	threadpool( int nThreads = 2 * std::thread::hardware_concurrency() )
		: join{ false }
	{
		for( int i = 0; i != nThreads; i++ )
		{
			pool.emplace_back( this );
		}
	}

public:
	static bool init( int nThreads = 2 * std::thread::hardware_concurrency() )
	{
		_instance.reset( new threadpool( nThreads ) );
		return true;
	}
	static bool destroy()
	{
		_instance.reset( nullptr );
		return true;
	}
	template< typename _Ty, typename _FTy = std::function<_Ty( _Ty const&, _Ty const& )>>
	static std::future<_Ty> enqueue( std::vector<std::function<_Ty()>> const& taskList = {}, _FTy const& comb = []( _Ty const& a, _Ty const& b ) { return a + b; } )
	{
		Job<_Ty> job( taskList, comb );
		std::future<_Ty> f = job.get_future();
		{
			std::lock_guard lk( _instance->m );
			_instance->waiting.emplace( std::move( job ) );
		}
		_instance->cv.notify_one();
		return f;
	}

	static std::future<void> enqueue( std::vector<std::function<void()>> const& taskList )
	{
		Job<void> job( taskList );
		std::future<void> f = job.get_future();
		{
			std::lock_guard lk( _instance->m );
			_instance->waiting.emplace( std::move( job ) );
		}
		_instance->cv.notify_one();
		return f;
	}

	template< typename _ITy >
	static void parallel_foreach( _ITy begin, _ITy end, std::function<void(_ITy)> fn ) 
	{
		std::vector<std::function<void()>> tasks;
		for( _ITy it = begin; it != end; it++ )
		{
			tasks.emplace_back( [it, fn]() { fn( it ); } );
		}
		enqueue( tasks ).get();
	}
		
	template< typename _Ty >
	static void parallel_foreach( _Ty& container, std::function<void(typename _Ty::iterator)> fn ) 
	{
		parallel_foreach( container.begin(), container.end(), fn );
	}

	template< typename _ITy >
	static void parallel_for_i( _ITy const& init, std::function<bool( _ITy )> check, std::function<void( _ITy& )> iterate, std::function<void( _ITy )> fn )
	{
		std::vector<std::function<void()>> tasks;
		for( _ITy it = init; check( it ); iterate( it ) )
		{
			tasks.emplace_back( [it, fn]() { fn( it ); } );
		}
		enqueue( tasks ).get();
	}

	///  must have deterministic check and iterate functions!
	template< typename _ITy >
	static void parallel_for_i_ballanced( _ITy const& init, std::function<bool( _ITy )> check, std::function<void( _ITy& )> iterate, std::function<void( _ITy )> fn )
	{
		std::size_t num = 0;
		for( auto it = init; check( it ); iterate( it ) )
			num++;
		auto sz = num / _instance->pool.size();
		std::vector<_ITy> iterators;
		std::size_t i = 0;
		for( _ITy it = init; check( it ); iterate( it ) )
		{
			if( 0 == i++ % sz )
				iterators.emplace_back( it );
			else if( i == num )
				iterators.emplace_back( it );
		}
		std::vector<std::function<void()>> tasks;

		{
			auto it = iterators.begin();
			if( it != iterators.end() ) 
			{
				_ITy start = *it;
				while( 1 )
				{
					it++;
					if( it != iterators.end() )
					{
						_ITy end = *it;
						tasks.emplace_back( [start, end, iterate, check, fn]() {
							for( _ITy itr = start; check( itr ) && itr != end; iterate( itr ) )
								fn( itr );
						} );
						start = end;
					}
					else
						break;
				}
			}
		}
		enqueue( tasks ).get();
	}

	~threadpool()
	{
		join = true;
		cv.notify_all();
	}
};


