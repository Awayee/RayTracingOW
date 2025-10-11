#pragma once
#include "Core/Defines.h"
#include "Math/Vector.h"
#include "Math/Geometry.h"
#include "Math/Color.h"
#include <vector>

class RayTracingCamera;
class RayTracingScene;

struct RenderResult {
	uint32 Width;
	uint32 Height;
	const Math::Color8* Data;
};

class RayTracingRenderer {
public:
	RayTracingRenderer(RayTracingCamera* InCamera, RayTracingScene* InScene);
	void Render();
	RenderResult GetRenderResult() const;
private:
	RayTracingCamera* Camera;
	RayTracingScene* Scene;

	// render
	uint32 RayPerPixel;
	std::vector<Math::Color8> Pixels;

	Math::FVector4 ComputeRayResult(const Math::FRay& Ray, uint32 RecursiveDepth);
	Math::FVector4 RayFallback(const Math::FRay& Ray);
};