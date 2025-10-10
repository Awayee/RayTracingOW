#include "Core/Defines.h"
#include <functional>

#ifdef _WIN32
#include <ppl.h>
inline void ParallelFor(uint64 Num, uint64 NumLoops, std::function<void(uint64)>&& Func){
    Concurrency::parallel_for(Num, NumLoops, Func);
}

#else
inline void ParallelFor(uint64 Num, uint64 NumLoops, std::function<void(uint64)>&& Func){
    for(uint64 i=0; i<NumLoops; ++i){
        Func(i);
    }
}
#endif