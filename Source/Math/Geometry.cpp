#include "Geometry.h"
namespace Math {
	FRay::FRay(){}

	FRay::FRay(const FVector3& InOrigin, const FVector3& InDirection): Origin(InOrigin), Direction(InDirection) {
	}

	FVector3 FRay::At(float t) const {
		return Origin + t * Direction;
	}

	FRayWithTime::FRayWithTime(): Time(0.0f){
	}

	FRayWithTime::FRayWithTime(const FVector3& InOrigin, const FVector3& InDirection, float InTime): FRay(InOrigin, InDirection), Time(InTime){
	}

	FRayHit::FRayHit() : Distance(0.0f), FrontFace(true) {}
	float FSphere::TestRay(const FRay& InRay) const {
		//const FVector3 oc = Center - InRay.Origin;
		//const float a = InRay.Direction.Dot(InRay.Direction);
		//const float b = -2.0f * InRay.Direction.Dot(oc);
		//const float c = oc.Dot(oc) - Radius * Radius;
		//const float discriminant = b * b - 4 * a * c;
		//if(discriminant < 0.0f) {
		//	return -1.0f;
		//}
		//return (-b - Sqrt(discriminant)) / (a * 2.0f);
		const FVector3 OC = Center - InRay.Origin;
		const float a = InRay.Direction.LengthSquared();
		const float h = InRay.Direction.Dot(OC);
		const float c = OC.LengthSquared() - Radius * Radius;
		const float Discriminant = h * h - a * c;
		if (Discriminant < 0.0f) {
			return -1.0f;
		}
		return h - Sqrt(Discriminant) / a;
	}

	bool FSphere::TestRay(const FRay& InRay, float DistanceMin, float DistanceMax, FRayHit& OutHit) const {
		const FVector3 OC = Center - InRay.Origin;
		const float a = InRay.Direction.LengthSquared();
		const float h = InRay.Direction.Dot(OC);
		const float c = OC.LengthSquared() - Radius * Radius;
		const float Discriminant = h * h - a * c;
		if (Discriminant < 0.0f) {
			return false;
		}
		const float SqrtD = std::sqrt(Discriminant);
		float t = (h - SqrtD) / a;
		if (t < DistanceMin || t > DistanceMax) {
			t = (h + SqrtD) / a;
			if (t < DistanceMin || t > DistanceMax) {
				return false;
			}
		}
		OutHit.Distance = t;
		OutHit.Position = InRay.At(t);
		const FVector3 OutWardNormal = (OutHit.Position - Center) / Radius;
		OutHit.FrontFace = OutWardNormal.Dot(InRay.Direction) < 0.0f;
		OutHit.Normal = OutHit.FrontFace ? OutWardNormal : -OutWardNormal;
		return true;
	}
}