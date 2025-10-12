#pragma once
#include "Math/Geometry.h"
#include "RayTracing/Material.h"

struct RayHitSurface {
	Math::FRayHit Geometry;
	const MaterialBase* Material{ nullptr };
};

class RayTracingObjectBase {
public:
	virtual ~RayTracingObjectBase();
	virtual bool TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const = 0;
	virtual bool TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const;
};

class RTSphere: public RayTracingObjectBase {
public:
	RTSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial);
	virtual bool TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const override;
protected:
	Math::FSphere GeometrySphere;
	MaterialPtr SurfaceMaterial;
};

class RTMovableSphere: public RTSphere {
public:
	RTMovableSphere(const Math::FSphere& InSphere, MaterialPtr&& InMatrial, const Math::FVector3& InMoveTarget);
	virtual bool TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const override;
private:
	Math::FVector3 MoveTarget;
	Math::FVector3 MoveDir;
	float MoveDistance;
};