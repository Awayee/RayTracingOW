#include "RayTracing/RayTracingObject.h"
#include "Core/Defines.h"
#include "Math/OrthonormalBasis.h"

RayTracingHittable::~RayTracingHittable()=default;

bool RayTracingHittable::TestRay(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const{
	return false;
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

bool RTSphere::TestRay(const Math::FRayWithTime& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	if(GeometrySphere.TestRay(Ray, DistanceMin, DistanceMax, OutHitSurface.Geometry)) {
		OutHitSurface.Material = SurfaceMaterial.Get();
		return true;
	}
	return false;
}

Math::FAABB3 RTSphere::GetAABB() const {
	return AABB;
}

inline Math::FVector3 RandomToSphere(float Radius, float DistanceSq) {
	float R1 = Math::Random01();
	float R2 = Math::Random01();
	float CosTheta = 1.0f + R2 * (Math::Sqrt(1 - Radius * Radius / DistanceSq) - 1);
	float SinTheta = Math::Sqrt(1.0f - CosTheta * CosTheta);
	float Phi = 2 * Math::PI * R1;
	return Math::FVector3{
		Math::Cos(Phi) * SinTheta,
		Math::Sin(Phi) * SinTheta,
		CosTheta
	};
}
float RTSphere::GetPDFValue(const Math::FVector3& Origin, const Math::FVector3& Direction) const {
	// Only for stationary spheres.
	Math::FRayHit Hit;
	if (!GeometrySphere.TestRay(Math::FRay{ Origin, Direction }, 0.0001f, 99999.0f, Hit)) {
		return 0.0f;
	}
	const float Radius = GeometrySphere.Radius;
	const float DistanceSq = (GeometrySphere.Center - Origin).LengthSquared();
	const float CosThetaMax = Math::Sqrt(1.0f - Radius * Radius / DistanceSq);
	const float SolidAngle = 2.0f * Math::PI * (1.0f - CosThetaMax);
	return 1.0f / SolidAngle;
}

Math::FVector3 RTSphere::Random(const Math::FVector3& Origin) const {
	Math::FVector3 Direction = GeometrySphere.Center - Origin;
	float DistanceSq = Direction.LengthSquared();
	Math::FOrthNormalBasis UVW{ Direction };
	return UVW.Transform(RandomToSphere(GeometrySphere.Radius, DistanceSq));
}

RTQuad::RTQuad(const Math::FQuad& InQuad, MaterialPtr&& InMaterial): RayTracingHittable(), GeometryQuad(InQuad), SurfaceMaterial(MoveTemp(InMaterial)) {
	// Compute the bounding box of all four vertices.
	AABB = InQuad.GetAABB();
	Area = InQuad.GetArea();
}

bool RTQuad::TestRay(const Math::FRayWithTime& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
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
	Math::FRayHit Hit;
	if (!GeometryQuad.TestRay(Math::FRay{Origin, Direction}, 0.0001f, 99999.0f, Hit)){
		return 0.0f;
	}
	const float DistanceSq = Hit.Distance * Hit.Distance * Direction.LengthSquared();
	const float Cosine = Math::Abs(Direction.Dot(Hit.Normal) / Direction.Length());
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

bool RTMovableSphere::TestRay(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
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

bool RTBox::TestRay(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	if(Box.TestRay(InRay, DistanceMin, DistanceMax, OutHitSurface.Geometry)) {
		OutHitSurface.Material = Material.Get();
		return true;
	}
	return false;
}

Math::FAABB3 RTBox::GetAABB() const {
	return AABB;
}

RTConstantMedium::RTConstantMedium(RTObjectPtr&& InObject, float InDensity, Math::Color8 Albedo) :
	RTConstantMedium(MoveTemp(InObject), InDensity, TexturePtr(new SolidColor(Albedo))) {
}

RTConstantMedium::RTConstantMedium(RTObjectPtr&& InObject, float InDensity, TexturePtr&& InTexture) :
	BoundaryObject(MoveTemp(InObject)) {
	NegInvDensity = -1.0f / InDensity;
	Material.Reset(new IsotropicMaterial(MoveTemp(InTexture)));
}

Math::FAABB3 RTConstantMedium::GetAABB() const {
	return BoundaryObject->GetAABB();
}

bool RTConstantMedium::TestRay(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	RayHitSurface HitSuf0, HitSuf1;
	Math::FRayHit& Hit0 = HitSuf0.Geometry;
	Math::FRayHit& Hit1 = HitSuf1.Geometry;
	if (!BoundaryObject->TestRay(InRay, -FLOAT_MAX, FLOAT_MAX, HitSuf0)) {
		return false;
	}
	if (!BoundaryObject->TestRay(InRay, Hit0.Distance + 0.0001f, FLOAT_MAX, HitSuf1)) {
		return false;
	}
	Hit0.Distance = Math::Max(Hit0.Distance, DistanceMin);
	Hit1.Distance = Math::Min(Hit1.Distance, DistanceMax);
	if (Hit0.Distance >= Hit1.Distance) {
		return false;
	}
	Hit0.Distance = Math::Max(Hit0.Distance, 0.0f);

	const float RayLength = InRay.Direction.Length();
	const float DistanceInsideBounddary = (Hit1.Distance - Hit0.Distance) * RayLength;
	const float RandomLog = Math::Log(Math::Random01());
	const float HitDistance = RandomLog * NegInvDensity;
	if (HitDistance > DistanceInsideBounddary) {
		return false;
	}

	OutHitSurface.Geometry.Distance = Hit0.Distance + HitDistance / RayLength;
	OutHitSurface.Geometry.Position = InRay.At(OutHitSurface.Geometry.Distance);
	OutHitSurface.Geometry.Normal = { 1,0,0 };
	OutHitSurface.Geometry.Texcoord = Math::RandomInDisk();
	OutHitSurface.Geometry.FrontFace = true;
	OutHitSurface.Material = Material.Get();
	return true;
}