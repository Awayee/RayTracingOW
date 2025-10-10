#pragma once
#include "RHI/RHIDefines.h"

#ifdef _WIN32
#define D3D12_RHI
#else
#define VULKAN_RHI
#endif

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

void InitializeRenderer(uint32 WindowWidth, uint32 WindowHeight);

RendererType* GetRenderer();

void ReleaseRenderer();
