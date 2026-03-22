#pragma once
#include "Core/Defines.h"
#include "Math/Vector.h"
#include "Math/Geometry.h"
#include "Math/Color.h"
#include <vector>

class RayTracingCamera;
class RayTracingScene;
class RayTracingHittable;

struct RenderResult {
	uint32 Width;
	uint32 Height;
	const Math::Color8* Data;
};

class RayTracingRenderer {
public:
	RayTracingRenderer(RayTracingCamera* InCamera, RayTracingScene* InScene, uint32 InNumRaysPerPixel, uint32 InRecursiveDepth);
	void Render();
	RenderResult GetRenderResult() const;
private:
	RayTracingCamera* Camera;
	RayTracingScene* Scene;

	// Performance params
	const uint32 NumRaysPerPixel;
	const uint32 RecursiveDepth;

	// render Result
	std::vector<Math::Color8> Pixels;

	Math::FVector4 ComputeRayResult(const Math::FRay& Ray, uint32 Depth);
	Math::FVector4 ComputeRayResultWithTime(const Math::FRayWithTime& Ray, uint32 Depth);
};