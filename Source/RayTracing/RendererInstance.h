#pragma once
#include "Renderer/RHICommon.h"

#ifdef _WIN32
#define D3D12_RHI
#else
#define VULKAN_RHI
#endif

#if defined(D3D12_RHI)
#include "Renderer/Windows/D3D12RHI.h"
using RendererType = D3D12RHI;
#elif defined(VULKAN_RHI)
#include "Renderer/Common/VulkanRHI.h"
using RendererType = VulkanRHI;
#else
#include "Renderer/NullRHI.h"
using RendererType = NullRHI;
#endif

void InitializeRenderer(uint32 WindowWidth, uint32 WindowHeight);

RendererType* GetRenderer();

void ReleaseRenderer();
