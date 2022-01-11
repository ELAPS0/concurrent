#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
//потокобезопасная обертка над std::queue
//недотаток в том, что блокировка общая для чтения и записи. 
//особенность в том, что потоки-получатели (consumers) не копируют данные из очереди, а обрабатывают их по месту.
//шаблон параметризируется типом данных, передаваемом через очередь (Data) и классом-обработичком данных 
//references
//https://codetrips.com/2020/07/26/modern-c-writing-a-thread-safe-queue/
//https://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
template<typename Data, typename Worker>
class concurrent_queue
{
private:
    std::mutex mtx;
    std::queue<Data> q;
    std::condition_variable cv;
    //ссылка на atomic флаг, указывающий необходимость продолжения цикла обработки данных, поступающих в очередь
    std::atomic <bool>*  up_and_running;
public:
    
    concurrent_queue(std::atomic <bool>* flg):up_and_running(flg){};
    concurrent_queue(const concurrent_queue<Data, Worker>& ) = delete;
    concurrent_queue<Data, Worker>& operator= (const concurrent_queue<Data, Worker>& ) = delete;
    //перемещение не реализовано

    void push(Data const& data)
    {
        {
            std::lock_guard <std::mutex> lck (mtx);
            q.push(data);
        }
        cv.notify_one();
    }
    
    //в потобезопасной реализации, предназначенной для работы с несколькими consumer'ами
    // подобный метод не может быть публичным и вызываться отдельно от других методов    
    //bool empty() const
    //{
    //    std::lock_guard <std::mute> lck (mtx);
    //    return the_queue.empty();
    //}
   
    //NYI переместить w, а не скопировать 
    void wait_and_get()
    {
        Worker w;    
        while(up_and_running->load(std::memory_order_relaxed))
        {
            std::unique_lock <std::mutex> lck(mtx);
            cv.wait(lck);
           //NYI w.proc(q.front());
            q.pop();
        }
    }
};

