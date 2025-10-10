#pragma once
#include "RHI/RHIDefines.h"

#define D3D12_RHI

#if defined(D3D12_RHI)
#include "RHI/D3D12RHI.h"
using RendererType = D3D12RHI;
#elif defined(VULKAN_RHI)
#include "RHI/VulkanRHI.h"
using RendererType = VulkanRHI;
#else
#include "RHI/NullRHI.h"
using RendererType = NullRHI;
#endif

void InitializeRenderer(AppInstanceHandle AppInstance, uint32 WindowWidth, uint32 WindowHeight);

RendererType* GetRenderer();

void ReleaseRenderer();
