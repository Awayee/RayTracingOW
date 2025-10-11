#pragma once
#include "RHI/RHICommon.h"

#if defined(_WIN32) && !FORCE_VULKAN
#define D3D12_RHI
#else
#define VULKAN_RHI
#endif

#if defined(D3D12_RHI)
#include "RHI/Windows/D3D12RHI.h"
using RHIType = D3D12RHI;
#elif defined(VULKAN_RHI)
#include "RHI/Common/VulkanRHI.h"
using RHIType = VulkanRHI;
#else
#include "RHI/Common/NullRHI.h"
using RHIType = NullRHI;
#endif

void InitializeRHI(uint32 WindowWidth, uint32 WindowHeight);

RHIType* GetRHI();

void ReleaseRHI();
