#include "RayTracingScene.h"

void RayTracingScene::AddSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial) {
	Objects.emplace_back(new RTSphere(InSphere, MoveTemp(InMaterial)));
}

void RayTracingScene::AddMovableSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial, const Math::FVector3& MoveTarget) {
	Objects.emplace_back(new RTMovableSphere(InSphere, MoveTemp(InMaterial), MoveTarget));
}

bool RayTracingScene::TestRay(const Math::FRay& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHit) const {
	bool hitAnything = false;
	float closestDistance = DistanceMax;
	for(const TUniquePtr<RayTracingObjectBase>& obj: Objects) {
		if(obj->TestRay(InRay, DistanceMin, closestDistance, OutHit)) {
			closestDistance = OutHit.Geometry.Distance;
			hitAnything = true;
		}
	}
	return hitAnything;
}

bool RayTracingScene::TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHit) const {
	bool hitAnything = false;
	float closestDistance = DistanceMax;
	for (const TUniquePtr<RayTracingObjectBase>& obj : Objects) {
		if (obj->TestRayWithTime(InRay, DistanceMin, closestDistance, OutHit)) {
			closestDistance = OutHit.Geometry.Distance;
			hitAnything = true;
		}
	}
	return hitAnything;
}

MaterialPtr RayTracingScene::MakeDefaultMaterial() {
	return MaterialPtr(new LambertMaterial({ 0.1f, 0.1f, 0.1f, 1.0f }));
}

