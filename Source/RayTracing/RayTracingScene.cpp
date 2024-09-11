#include "RayTracingScene.h"

void RayTracingScene::AddSphere(const Math::FSphere& sphere) {
	AddSphere(sphere, MakeDefaultMaterial());
}

void RayTracingScene::AddSphere(const Math::FSphere& sphere, MaterialPtr&& material) {
	m_Objects.push_back({ sphere, MoveTemp(material)});
}

bool RayTracingScene::RayHit(const Math::FRay& ray, float rayMin, float rayMax, RayHitSurface& outHit) const {
	bool hitAnything = false;
	float closestDistance = rayMax;
	for(const auto& obj: m_Objects) {
		if(obj.Sphere.RayHit(ray, rayMin, closestDistance, outHit.Geometry)) {
			closestDistance = outHit.Geometry.Distance;
			outHit.Material = obj.Material.get();
			hitAnything = true;
		}
	}
	return hitAnything;
}

MaterialPtr RayTracingScene::MakeDefaultMaterial() {
	return MaterialPtr(new LambertMaterial({ 0.1f, 0.1f, 0.1f, 1.0f }));
}

