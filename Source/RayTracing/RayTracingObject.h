#pragma once
#include "Math/Geometry.h"
#include "RayTracing/Material.h"

struct RayHitSurface {
	Math::FRayHit Geometry;
	const MaterialBase* Material{ nullptr };
};

class RayTracingHittable {
public:
	virtual ~RayTracingHittable();
	virtual bool TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const;// TODO discard
	virtual bool TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const;
	virtual Math::FAABB3 GetAABB() const = 0;
	virtual float GetPDFValue(const Math::FVector3& Origin, const Math::FVector3& Direction) const;
	virtual Math::FVector3 Random(const Math::FVector3& Origin) const;
};

typedef TUniquePtr<RayTracingHittable> RTObjectPtr;

class RTSphere: public RayTracingHittable {
public:
	RTSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial);
	virtual bool TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const override;
	virtual Math::FAABB3 GetAABB() const override;
protected:
	Math::FSphere GeometrySphere;
	MaterialPtr SurfaceMaterial;
	Math::FAABB3 AABB;
};

class RTQuad: public RayTracingHittable {
public:
	RTQuad(const Math::FQuad& InQuad, MaterialPtr&& InMaterial);
	virtual bool TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const override;
	virtual Math::FAABB3 GetAABB() const override;
	virtual float GetPDFValue(const Math::FVector3& Origin, const Math::FVector3& Direction) const override;
	virtual Math::FVector3 Random(const Math::FVector3& Origin) const override;
private:
	Math::FQuad GeometryQuad;
	Math::FAABB3 AABB;
	float Area;
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

class RTBox: public RayTracingHittable {
public:
	RTBox(const Math::FVector3& A, const Math::FVector3& B, MaterialPtr&& InMaterial);
	virtual bool TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const override;
	virtual Math::FAABB3 GetAABB() const override;
private:
	Math::FBox Box;
	Math::FAABB3 AABB;
	MaterialPtr Material;
};