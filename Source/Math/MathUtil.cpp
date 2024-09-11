#include "MathUtil.h"
#include <cfloat>

namespace Math {

	FVector3 FRay::At(float t) const {
		return Origin + t * Direction;
	}

	float FSphere::RayHit(const FRay& ray) const {
		//const FVector3 oc = Center - ray.Origin;
		//const float a = ray.Direction.Dot(ray.Direction);
		//const float b = -2.0f * ray.Direction.Dot(oc);
		//const float c = oc.Dot(oc) - Radius * Radius;
		//const float discriminant = b * b - 4 * a * c;
		//if(discriminant < 0.0f) {
		//	return -1.0f;
		//}
		//return (-b - Sqrt(discriminant)) / (a * 2.0f);
		const FVector3 oc = Center - ray.Origin;
		const float a = ray.Direction.LengthSquared();
		const float h = ray.Direction.Dot(oc);
		const float c = oc.LengthSquared() - Radius * Radius;
		const float discriminant = h * h - a * c;
		if(discriminant < 0.0f) {
			return -1.0f;
		}
		return h - Sqrt(discriminant) / a;
	}

	bool FSphere::RayHit(const FRay& ray, float rayMin, float rayMax, FRayHit& outHit) const {
		const FVector3 oc = Center - ray.Origin;
		const float a = ray.Direction.LengthSquared();
		const float h = ray.Direction.Dot(oc);
		const float c = oc.LengthSquared() - Radius * Radius;
		const float discriminant = h * h - a * c;
		if(discriminant < 0.0f) {
			return false;
		}
		const float sqrtD = std::sqrt(discriminant);
		float t = (h - sqrtD) / a;
		if(t < rayMin || t > rayMax) {
			t = (h + sqrtD) / a;
			if(t < rayMin || t > rayMax) {
				return false;
			}
		}
		outHit.Distance = t;
		outHit.Position = ray.At(t);
		const FVector3 outwardNormal = (outHit.Position - Center) / Radius;
		outHit.FrontFace = outwardNormal.Dot(ray.Direction) < 0.0f;
		outHit.Normal = outHit.FrontFace ? outwardNormal : -outwardNormal;
		return true;
	}
}
