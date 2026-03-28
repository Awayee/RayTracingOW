#pragma once
#include "Math/Matrix.h"
#include "RayTracing/RayTracingObject.h"

class RTTranslated: public RayTracingHittable {
public:
	RTTranslated(RTObjectPtr&& InObject, const Math::FVector3& InTranslation);
	virtual bool TestRay(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const override;
	virtual Math::FAABB3 GetAABB() const override;
private:
	RTObjectPtr Object;
	Math::FAABB3 AABB;
	Math::FVector3 Translation;
};

class RTRotatedY: public RayTracingHittable {
public:
	RTRotatedY(RTObjectPtr&& InObject, float Radian);
	virtual bool TestRay(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const override;
	virtual Math::FAABB3 GetAABB() const override;
private:
	RTObjectPtr Object;
	Math::FAABB3 AABB;
	Math::FMatrix3x3 RotationMatrix;
	Math::FMatrix3x3 InvRotationMatrix;
	float RotationY;
};