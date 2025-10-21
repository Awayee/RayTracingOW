#include "RayTracing/RayTracingObject.h"
#include "Core/Defines.h"

RayTracingObjectBase::~RayTracingObjectBase()=default;

bool RayTracingObjectBase::TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	return TestRay((const Math::FRay&)InRay, DistanceMin, DistanceMax, OutHitSurface);
}

RTSphere::RTSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial):
GeometrySphere(InSphere),SurfaceMaterial(MoveTemp(InMaterial)) {
	AABB = Math::FAABB3::CenterExtent(InSphere.Center, Math::FVector3{InSphere.Radius});
}

bool RTSphere::TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	if(GeometrySphere.TestRay(Ray, DistanceMin, DistanceMax, OutHitSurface.Geometry)) {
		OutHitSurface.Material = SurfaceMaterial.Get();
		return true;
	}
	return false;
}

Math::FAABB3 RTSphere::GetAABB() const {
	return AABB;
}

RTQuad::RTQuad(const Math::FQuad& InQuad, MaterialPtr&& InMaterial): RayTracingObjectBase(), GeometryQuad(InQuad), SurfaceMaterial(MoveTemp(InMaterial)) {
	// Compute the bounding box of all four vertices.
	AABB = InQuad.GetAABB();
}

bool RTQuad::TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	if(GeometryQuad.TestRay(Ray, DistanceMin, DistanceMax, OutHitSurface.Geometry)) {
		OutHitSurface.Material = SurfaceMaterial.Get();
		return true;
	}
	return false;
}

Math::FAABB3 RTQuad::GetAABB() const {
	return AABB;
}

RTMovableSphere::RTMovableSphere(const Math::FSphere& InSphere, MaterialPtr&& InMatrial, const Math::FVector3& InMoveTarget):
RTSphere(InSphere, MoveTemp(InMatrial)), MoveTarget(InMoveTarget){
	const Math::FVector3 MoveVector = MoveTarget - GeometrySphere.Center;
	MoveDistance = MoveVector.Length();
	MoveDir = MoveVector / MoveDistance;

	Math::FAABB3 TargetAABB = Math::FAABB3::CenterExtent(MoveTarget, Math::FVector3{InSphere.Radius});
	AABB.Union(TargetAABB);
}

bool RTMovableSphere::TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	const float Time = Math::Clamp(InRay.Time, 0.0f, 1.0f);
	const Math::FVector3 MovedCenter = GeometrySphere.Center + MoveDir * MoveDistance * Time;
	const Math::FSphere MovedSphere{MovedCenter, GeometrySphere.Radius};
	if(MovedSphere.TestRay((const Math::FRay&)InRay, DistanceMin, DistanceMax, OutHitSurface.Geometry)) {
		OutHitSurface.Material = SurfaceMaterial.Get();
		return true;
	}
	return false;
}
