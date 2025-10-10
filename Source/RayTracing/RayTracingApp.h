#pragma once
#include "Core/TUniquePtr.h"
#include "RHI/RHIDefines.h"

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
	RHITextureHandle Texture;
};