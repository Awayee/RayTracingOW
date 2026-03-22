#include "RayTracing/RayTracingObject.h"
#include "Core/Defines.h"

RayTracingHittable::~RayTracingHittable()=default;

bool RayTracingHittable::TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	return false;
}

bool RayTracingHittable::TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	return TestRay((const Math::FRay&)InRay, DistanceMin, DistanceMax, OutHitSurface);
}

float RayTracingHittable::GetPDFValue(const Math::FVector3& Origin, const Math::FVector3& Direction) const {
	return 0.0f;
}

Math::FVector3 RayTracingHittable::Random(const Math::FVector3& Origin) const {
	return Math::FVector3{ 1.0f, 0.0f, 0.0f };
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

RTQuad::RTQuad(const Math::FQuad& InQuad, MaterialPtr&& InMaterial): RayTracingHittable(), GeometryQuad(InQuad), SurfaceMaterial(MoveTemp(InMaterial)) {
	// Compute the bounding box of all four vertices.
	AABB = InQuad.GetAABB();
	Area = InQuad.GetArea();
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

float RTQuad::GetPDFValue(const Math::FVector3& Origin, const Math::FVector3& Direction) const {
	RayHitSurface Hit;
	if (!TestRayWithTime(Math::FRayWithTime{ Origin, Direction, 0.0f}, 0.0001f, 99999.0f, Hit)) {
		return 0.0f;
	}
	const float DistanceSq = Hit.Geometry.Distance * Hit.Geometry.Distance * Direction.LengthSquared();
	const float Cosine = Math::Abs(Direction.Dot(Hit.Geometry.Normal) / Direction.Length());
	return DistanceSq / (Cosine * Area);
}

Math::FVector3 RTQuad::Random(const Math::FVector3& Origin) const {
	const Math::FVector3 P = GeometryQuad.GetOrigin() + (Math::Random01() * GeometryQuad.GetU()) + (Math::Random01() * GeometryQuad.GetV());
	return (P - Origin).Normalize();
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

RTBox::RTBox(const Math::FVector3& A, const Math::FVector3& B, MaterialPtr&& InMaterial): Box(A, B), Material(MoveTemp(InMaterial)){
	// Construct the two opposite vertices with the minimum and maximum coordinates.
	Math::FVector3 Min = Math::FVector3::Min(A, B);
	Math::FVector3 Max = Math::FVector3::Max(A, B);
	AABB = Math::FAABB3{Min, Max};
}

bool RTBox::TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	if(Box.TestRay(InRay, DistanceMin, DistanceMax, OutHitSurface.Geometry)) {
		OutHitSurface.Material = Material.Get();
		return true;
	}
	return false;
}

Math::FAABB3 RTBox::GetAABB() const {
	return AABB;
}
