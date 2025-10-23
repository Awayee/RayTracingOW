#include "RayTracing/RayTracingObject.h"
#include "Core/Defines.h"

RayTracingObjectBase::~RayTracingObjectBase()=default;

bool RayTracingObjectBase::TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	return false;
}

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

RTBox::RTBox(const Math::FVector3& A, const Math::FVector3& B, MaterialPtr&& InMaterial): Material(MoveTemp(InMaterial)) {
	// Construct the two opposite vertices with the minimum and maximum coordinates.
	Math::FVector3 Min = Math::FVector3::Min(A, B);
	Math::FVector3 Max = Math::FVector3::Max(A, B);

	Math::FVector3 DX = Math::FVector3{ Max.X - Min.X, 0.0f, 0.0f };
	Math::FVector3 DY = Math::FVector3{ 0.0f, Max.Y - Min.Y, 0.0f };
	Math::FVector3 DZ = Math::FVector3{ 0.0f, 0.0f, Max.Z - Min.Z };

	Quads[0].Reset(new RTQuad({ {Min.X, Min.Y, Max.Z}, DX, DY }, {})); // front
	Quads[1].Reset(new RTQuad({ {Max.X, Min.Y, Max.Z},-DZ, DY }, {})); // right
	Quads[2].Reset(new RTQuad({ {Max.X, Min.Y, Min.Z},-DX, DY }, {})); // back
	Quads[3].Reset(new RTQuad({ {Min.X, Min.Y, Min.Z}, DZ, DY }, {})); // left
	Quads[4].Reset(new RTQuad({ {Min.X, Max.Y, Max.Z}, DX,-DZ }, {})); // top
	Quads[5].Reset(new RTQuad({ {Min.X, Min.Y, Min.Z}, DX, DZ }, {})); // bottom
	AABB = Math::FAABB3{Min, Max};
}

bool RTBox::TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	bool bHit = false;
	for(const auto& Quad: Quads) {
		if(Quad->TestRayWithTime(InRay, DistanceMin, DistanceMax, OutHitSurface)) {
			bHit = true;
			DistanceMax = OutHitSurface.Geometry.Distance;
			OutHitSurface.Material = Material.Get();
		}
	}
	return bHit;
}

Math::FAABB3 RTBox::GetAABB() const {
	return AABB;
}
