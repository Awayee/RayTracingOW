#pragma once
#include "RayTracing/RayTracingObject.h"
#include "RayTracing/Material.h"

class RTConstantMedium : public RayTracingObjectBase {
public:
	RTConstantMedium(RTObjectPtr&& InObject, float InDensity, Math::Color8 Albedo);
	RTConstantMedium(RTObjectPtr&& InObject, float InDensity, TexturePtr&& InTexture);
	Math::FAABB3 GetAABB() const override;
	bool TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const override;
private:
	RTObjectPtr BoundaryObject;
	TUniquePtr<IsotropicMaterial> Material;
	float NegInvDensity;
};