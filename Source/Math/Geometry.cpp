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

	FAABB3 FAABB3::CenterExtent(const Math::FVector3& Center, const FVector3& Extent) {
		return FAABB3{Center-Extent, Center+Extent};
	}

	FVector3 FAABB3::Center() const	{
		return (Max + Min) * 0.5f;
	}

	FVector3 FAABB3::Extent() const {
		return (Max - Min) * 0.5f;
	}

	bool FAABB3::IsValid() const {
		return Math::FVector3::AllGreater(Max, Min);
	}

	int FAABB3::GetMaxAxis() const {
		const Math::FVector3 Ext = Extent();
		if(Ext.X > Ext.Y) {
			return Ext.X > Ext.Z ? 0 : 2;
		}
		else {
			return Ext.Y > Ext.Z ? 1 : 2;
		}
	}

	void FAABB3::Union(const FAABB3& Other) {
		if(IsValid()) {
			Min = Math::FVector3::Min(Min, Other.Min);
			Max = Math::FVector3::Max(Max, Other.Max);
		}
		else {
			Min = Other.Min;
			Max = Other.Max;
		}
	}

	bool FAABB3::TestRay(const FRay& InRay, float DistanceMin, float DistanceMax) const{
		for(int Axis=0; Axis<3; ++Axis) {
			const float IntervalMin = Min[Axis];
			const float IntervalMax = Max[Axis];
			const float DirectionInAxis = InRay.Direction[Axis];
			const float OriginInAxis = InRay.Origin[Axis];
			// X = O + t * D => t = (X - O) / D
			const float t0 = (IntervalMin - OriginInAxis) / DirectionInAxis;
			const float t1 = (IntervalMax - OriginInAxis) / DirectionInAxis;
			// Shrink ray interval
			if(t0 < t1) {
				DistanceMin = Math::Max(t0, DistanceMin);
				DistanceMax = Math::Min(t1, DistanceMax);
			}
			else {
				DistanceMin = Math::Max(t1, DistanceMin);
				DistanceMax = Math::Min(t0, DistanceMax);
			}
			// Range missed
			if(DistanceMin > DistanceMax) {
				return false;
			}
		}
		return true;
	}
}
