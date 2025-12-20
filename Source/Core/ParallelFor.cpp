#include "ParallelFor.h"
#ifdef _WIN32
#include <ppl.h>
void ParallelFor(uint64 Num, uint64 NumLoops, std::function<void(uint64)>&& Func){
    Concurrency::parallel_for(Num, NumLoops, Func);
}
#elif defined(_OPENMP)
void ParallelFor(uint64 Num, uint64 NumLoops, std::function<void(uint64)>&& Func){
    Concurrency::parallel_for(Num, NumLoops, Func);
}
#else
#include <thread>
#include <mutex>
void ParallelFor(uint64 Num, uint64 NumLoops, std::function<void(uint64)>&& Func){
    uint64 NumThreads = std::thread::hardware_concurrency();
    uint64 ChunkSize = NumLoops / NumThreads;
    std::vector<std::thread> threads;
    for (uint64 ThreadIndex = 0; ThreadIndex < NumThreads; ++ThreadIndex) {
        uint64 ChunkStart = Num + ThreadIndex * ChunkSize;
        uint64 ChunkEnd = (ThreadIndex == NumThreads - 1) ? Num+NumLoops : ChunkStart + ChunkSize;
        threads.emplace_back([ChunkStart, ChunkEnd, &Func]() {
            for (int i = ChunkStart; i < ChunkEnd; ++i) {
                Func(i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}
#endif