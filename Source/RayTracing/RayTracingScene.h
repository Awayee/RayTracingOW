#pragma once
#include <vector>
#include "RayTracing/RayTracingObject.h"
#include "Core/TUniquePtr.h"

class RayTracingScene {
public:
	RayTracingScene() = default;
	~RayTracingScene() = default;

	void AddSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial);
	void AddMovableSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial, const Math::FVector3& MoveTarget);
	bool TestRay(const Math::FRay& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHit) const;
	bool TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHit) const;
private:
	std::vector<TUniquePtr<RayTracingObjectBase>> Objects;
	MaterialPtr MakeDefaultMaterial();
};