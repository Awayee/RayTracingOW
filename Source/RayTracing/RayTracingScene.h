#pragma once
#include <vector>
#include "Math/MathUtil.h"
#include "Material.h"

struct RayHitSurface{
	Math::FRayHit Geometry;
	MaterialBase* Material{ nullptr };
};

class RayTracingScene {
public:
	RayTracingScene() = default;
	~RayTracingScene() = default;
	void AddSphere(const Math::FSphere& sphere);
	void AddSphere(const Math::FSphere& sphere, MaterialPtr&& material);
	bool RayHit(const Math::FRay& ray, float rayMin, float rayMax, RayHitSurface& outHit) const;
private:
	struct SceneObject {
		Math::FSphere Sphere;
		MaterialPtr Material;
	};
	std::vector<SceneObject> m_Objects;
	MaterialPtr MakeDefaultMaterial();
};