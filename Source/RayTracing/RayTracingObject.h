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
	virtual Math::FAABB3 GetAABB() const = 0;
};

typedef TUniquePtr<RayTracingObjectBase> RTObjectPtr;

class RTSphere: public RayTracingObjectBase {
public:
	RTSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial);
	virtual bool TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const override;
	virtual Math::FAABB3 GetAABB() const override;
protected:
	Math::FSphere GeometrySphere;
	MaterialPtr SurfaceMaterial;
	Math::FAABB3 AABB;
};

class RTQuad: public RayTracingObjectBase {
public:
	RTQuad(const Math::FQuad& InQuad, MaterialPtr&& InMaterial);
	virtual bool TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const override;
	virtual Math::FAABB3 GetAABB() const override;
private:
	Math::FQuad GeometryQuad;
	Math::FAABB3 AABB;
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