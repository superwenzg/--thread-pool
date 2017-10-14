#ifndef TP_2_H
#define TP_2_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace d2
{
    class threadpool
    {
    public:
        threadpool( size_t nthreads = 0 );
        ~threadpool();
        
        template< class F, class... Args >
        auto entasks( F&& f, Args&&... args )
        -> std::future< typename std::result_of<F(Args...)>::type >;
    private:
        std::vector< std::thread > threads;
        std::queue< std::function<void()> > fucs;
        
        std::condition_variable conVariable;
        std::mutex qMutex;
        bool stopSignal;
    };
}

d2::threadpool::threadpool( size_t nthreads )
: stopSignal( false )
{
    if( !nthreads ) nthreads = std::thread::hardware_concurrency();
    
    for( int i = 0;i < nthreads; i++ )
    {
        threads.emplace_back([this](){
            for(;;)
            {
                std::function< void() > task;
                {
                    std::unique_lock< std::mutex > ulk( this -> qMutex);
                    conVariable.wait( ulk,[this]{
                        return ( stopSignal || !this -> fucs.empty() );
                    });
                    
                    if( stopSignal && fucs.empty() ) return;
                    task = std::move( fucs.front() );
                    fucs.pop();
                }
                task();
            }
        });
    }
}

template< class F, class... Args >
auto d2::threadpool::entasks(F&& f, Args&&...args )
    ->std::future< typename std::result_of< F(Args...) >::type >
{
    using retType = typename std::result_of< F(Args...) >::type;
    
    auto task = std::make_shared< std::packaged_task< retType() > >(
        std::bind( std::forward<F>(f), std::forward<Args>(args)... ) );
    
    std::future< retType > res = task -> get_future();
    {
        std::lock_guard< std::mutex > lg( qMutex );
        if( stopSignal )
            throw "thread pools had already stop";
        
        fucs.push([task](){
            (*task)(); });
    }
    conVariable.notify_one();
    
    return res;
}

d2::threadpool::~threadpool()
{
    {
        std::lock_guard< std::mutex > lg( qMutex );
        stopSignal = true;
    }
    conVariable.notify_all();

    for( auto  &t : threads )
        t.join();
}
#endif
