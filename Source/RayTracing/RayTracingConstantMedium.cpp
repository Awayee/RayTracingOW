#include "RayTracing/RayTracingConstantMedium.h"

RTConstantMedium::RTConstantMedium(RTObjectPtr&& InObject, float InDensity, Math::Color8 Albedo):
RTConstantMedium(MoveTemp(InObject), InDensity, TexturePtr(new SolidColor(Albedo))) {
}

RTConstantMedium::RTConstantMedium(RTObjectPtr&& InObject, float InDensity, TexturePtr&& InTexture):
BoundaryObject(MoveTemp(InObject)){
	NegInvDensity = -1.0f / InDensity;
	Material.Reset(new IsotropicMaterial(MoveTemp(InTexture)));
}

Math::FAABB3 RTConstantMedium::GetAABB() const {
	return BoundaryObject->GetAABB();
}

bool RTConstantMedium::TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	RayHitSurface HitSuf0, HitSuf1;
	Math::FRayHit& Hit0 = HitSuf0.Geometry;
	Math::FRayHit& Hit1 = HitSuf1.Geometry;
	if(!BoundaryObject->TestRayWithTime(InRay, -FLOAT_MAX, FLOAT_MAX, HitSuf0)) {
		return false;
	}
	if(!BoundaryObject->TestRayWithTime(InRay, Hit0.Distance+0.0001f, FLOAT_MAX, HitSuf1)) {
		return false;
	}
	Hit0.Distance = Math::Max(Hit0.Distance, DistanceMin);
	Hit1.Distance = Math::Min(Hit1.Distance, DistanceMax);
	if(Hit0.Distance >= Hit1.Distance) {
		return false;
	}
	Hit0.Distance = Math::Max(Hit0.Distance, 0.0f);

	const float RayLength = InRay.Direction.Length();
	const float DistanceInsideBounddary = (Hit1.Distance - Hit0.Distance) * RayLength;
	const float RandomLog = Math::Log(Math::Random01());
	const float HitDistance = RandomLog * NegInvDensity;
	if(HitDistance > DistanceInsideBounddary) {
		return false;
	}

	OutHitSurface.Geometry.Distance = Hit0.Distance + HitDistance / RayLength;
	OutHitSurface.Geometry.Position = InRay.At(OutHitSurface.Geometry.Distance);
	OutHitSurface.Geometry.Normal = {1,0,0};
	OutHitSurface.Geometry.Texcoord = Math::RandomInDisk();
	OutHitSurface.Geometry.FrontFace = true;
	OutHitSurface.Material = Material.Get();
	return true;
}
