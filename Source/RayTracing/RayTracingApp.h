#pragma once

#include "RHI/RHICommon.h"
#include "Core/TUniquePtr.h"

class RayTracingScene;
class RayTracingCamera;
class RayTracingApp {
public:
	RayTracingApp();
	~RayTracingApp();
	void Run();
private:
	TUniquePtr<RayTracingScene> Scene;
	TUniquePtr<RayTracingCamera> Camera;
	RHITextureHandle RHITexture;
};