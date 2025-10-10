#pragma once
#include <Windows.h>
#include <memory>
#include "RHI/RHIDefines.h"

class RayTracingScene;
class RayTracingCamera;
class RayTracingApp {
public:
	RayTracingApp(HINSTANCE hInstance);
	~RayTracingApp();
	void Run();
private:
	std::unique_ptr<RayTracingScene> m_Scene;
	std::unique_ptr<RayTracingCamera> m_Camera;
	RHITextureHandle Texture;
};