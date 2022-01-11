#include <string.h>
#include <iostream>
#include <cassert> 
#include <chrono> 
#include <fstream> 
#include <thread> 
#include <vector> 
#include <array> 
#include <atomic> 
#include "concurrent_queue.h"
#include "keccak.h"

using namespace std;

#define CHUNK_SIZE      2048

std::atomic<bool> up_and_running {true};
std::atomic<size_t> cntr{ 0 };

class keccak_worker
{
    Keccak keccak224;
public:
    keccak_worker(): keccak224(Keccak::Keccak224){};
    void proc (array <char,CHUNK_SIZE > dt)
    {
        keccak224(dt.data(), dt.size());
    }
};

void usage()
{
    cout << "cnccrnt mode produser_threads_number [consumer_threas_number]\n";
    cout << "\tmode - sync|async";
    cout << "for async mode concumer threads number is mandatory"; 
    exit(0);
}

void sync_produce()
{
    char dt[CHUNK_SIZE];
    Keccak keccak224(Keccak::Keccak224);
    ifstream rnd("/dev/urandom");
    while(up_and_running.load(std::memory_order_relaxed))
    {
        rnd.read(dt, CHUNK_SIZE);
        keccak224( dt, CHUNK_SIZE);
        cntr.fetch_add(CHUNK_SIZE,std::memory_order_relaxed);         
    }
    rnd.close();
};

concurrent_queue < array <char, CHUNK_SIZE>, keccak_worker> q(&up_and_running);

void async_produce()
{
    array <char, CHUNK_SIZE>  dt;
    ifstream rnd("/dev/urandom");
    while(up_and_running.load(std::memory_order_relaxed))
    {
        rnd.read(dt.data(), CHUNK_SIZE);
        q.push(dt);
        cntr.fetch_add(CHUNK_SIZE,std::memory_order_relaxed);         
    }
    rnd.close();
};

void consume()
{
    q.wait_and_get();
}

int main(int argc, char** argv)
{
    int prods_num = 0;
    int cons_num = 0;
    void (*produce_ptr)();
    if (3 == argc && !strcmp(argv[1],"sync"))
    {
        prods_num = atoi(argv[2]);
        produce_ptr = sync_produce;
    }
    else if  (4 == argc && !strcmp(argv[1],"async"))
    {      
        prods_num = atoi(argv[2]);
        cons_num = atoi(argv[3]);
        produce_ptr = async_produce;
    }
    else usage();

    std::vector<std::thread> prods;
    std::vector<std::thread> cons;

    auto start_time = chrono::steady_clock::now();
    
    for (int i = 0; i != prods_num; ++i)
        prods.emplace_back(produce_ptr);

    if (produce_ptr == async_produce)
    {

        for (int i = 0; i != cons_num; ++i)
            cons.emplace_back(consume);
    }

    this_thread::sleep_for(3s);
    up_and_running.store(false, std::memory_order_relaxed);
    for (auto& i: prods)
        i.join();
    
    cout << "Bytes processed " << cntr.load(std::memory_order_relaxed) << "\n";
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    std::cout << elapsed_ns.count() << " ns " << "done" << endl;
    
    if (produce_ptr == async_produce)
        for (auto& i: cons)
            i.join();

    
}   
