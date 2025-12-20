#pragma once
#include "Core/Defines.h"
#include <functional>

void ParallelFor(uint64 Num, uint64 NumLoops, std::function<void(uint64)>&& Func);
