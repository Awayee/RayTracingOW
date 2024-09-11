#pragma once
#include <Windows.h>
#include <memory>

class Texture2D;
class PSOCommon;
class RayTracingScene;
class RayTracingCamera;

class RayTracingApp {
public:
	RayTracingApp(HINSTANCE hInstance);
	~RayTracingApp();
	void Run();
private:
	std::unique_ptr<Texture2D> m_Texture;
	std::unique_ptr<PSOCommon> m_PSO;
	std::unique_ptr<RayTracingScene> m_Scene;
	std::unique_ptr<RayTracingCamera> m_Camera;
	void RenderTexture();
};