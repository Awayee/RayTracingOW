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
		const float SqrtD = Math::Sqrt(Discriminant);
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
		OutHit.Texcoord = GetTexcoordByNormal(OutWardNormal);
		return true;
	}

	Math::FVector2 FSphere::GetTexcoordByNormal(const Math::FVector3& Normal) {
		const float Theta = Math::ACos(-Normal.Y);
		const float Phi = Math::ATan2(-Normal.Z, Normal.X) + Math::PI;
		return Math::FVector2{
			Phi / (2.0f * Math::PI),
			Theta / Math::PI
		};
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
		return !Extent().IsNearlyZero();
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

	FAABB3 FAABB3::Translate(const Math::FVector3& Translation) const {
		return FAABB3{Min+Translation, Max+Translation};
	}

	FAABB3 FAABB3::Rotate(const Math::FMatrix3x3& Rotation) const {
		Math::FVector3 TempMin{FLOAT_MAX}, TempMax{-FLOAT_MAX};
		for(uint32 i=0; i<8; ++i) {
			Math::FVector3 Point{
				(i & (1<<2)) ? Max.X: Min.X,
				(i & (1<<1)) ? Max.Y: Min.Y,
				(i & 1 ) ? Max.Z : Min.Z};
			Point = Rotation * Point;
			TempMin = Math::FVector3::Min(TempMin, Point);
			TempMax = Math::FVector3::Max(TempMax, Point);
		}
		return Math::FAABB3{TempMin, TempMax};
	}

	bool FAABB3::TestRay(const FRay& InRay, float DistanceMin, float DistanceMax) const{
		// Reference: https://raytracing.github.io/books/RayTracingTheNextWeek.html#boundingvolumehierarchies/hierarchiesofboundingvolumes
		for(int Axis=0; Axis<3; ++Axis) {
			const float IntervalMin = Min[Axis];
			const float IntervalMax = Max[Axis];
			const float DirectionInAxis = InRay.Direction[Axis];
			const float OriginInAxis = InRay.Origin[Axis];
			// Parallel direction
			if (Math::IsNearlyZero(DirectionInAxis)) {
				if(OriginInAxis < IntervalMin || OriginInAxis > IntervalMax) {
					return false;
				}
			}
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

	FPlane::FPlane(const Math::FVector3& InNormal, float InD): Normal(InNormal), D(InD) {
	}

	FPlane::FPlane(const Math::FVector3& InQ, const Math::FVector3& InU, const Math::FVector3& InV) {
		Normal = InU.Cross(InV).Normalize();
		D = Normal.Dot(InQ);
	}

	bool FPlane::TestRay(const FRay& InRay, float DistanceMin, float DistanceMax, Math::FRayHit& OutHit) const {
		const float Denom = Normal.Dot(InRay.Direction);
		if(Math::IsNearlyZero(Denom)) {
			return false;
		}
		const float t = (D - InRay.Origin.Dot(Normal)) / Denom;
		if(t < DistanceMin || t > DistanceMax) {
			return false;
		}
		OutHit.Distance = t;
		OutHit.Position = InRay.At(t);
		OutHit.FrontFace = InRay.Direction.Dot(Normal) < 0.0f;
		OutHit.Normal = OutHit.FrontFace ? Normal : -Normal;
		return true;
	}

	FQuad::FQuad(const FVector3& InQ, const FVector3& InU, const FVector3& InV) :Q(InQ), U(InU), V(InV), Plane{ Q, U, V } {
	}

	bool FQuad::TestRay(const FRay& InRay, float DistanceMin, float DistanceMax, FRayHit& OutHit) const {
		// Reference: https://raytracing.github.io/books/RayTracingTheNextWeek.html#quadrilaterals/ray-planeintersection
		FRayHit TempHit;
		if(!Plane.TestRay(InRay, DistanceMin, DistanceMax, TempHit)) {
			return false;
		}
		// Whether the intersected point lies in the quad
		const Math::FVector3 NormalUnorm = U.Cross(V);
		const Math::FVector3 W = NormalUnorm / NormalUnorm.LengthSquared();
		const Math::FVector3 HitVec = TempHit.Position - Q;
		float Alpha = W.Dot(HitVec.Cross(V));
		float Beta  = W.Dot(U.Cross(HitVec));
		if(Alpha < 0.0f || Alpha > 1.0f || Beta < 0.0f || Beta > 1.0f) {
			return false;
		}
		OutHit = TempHit;
		OutHit.Texcoord = {Alpha, Beta};
		return true;
	}

	FAABB3 FQuad::GetAABB() const {
		Math::FVector3 Points[4];
		Points[0] = Q;
		Points[1] = Q+U;
		Points[2] = Q+V;
		Points[3] = Points[1]+V;
		Math::FVector3 Min{ FLOAT_MAX }, Max{ -FLOAT_MAX };
		for(const Math::FVector3& Point: Points) {
			Min = Math::FVector3::Min(Min, Point);
			Max = Math::FVector3::Max(Max, Point);
		}
		return Math::FAABB3{Min, Max};
	}

	float FQuad::GetArea() const {
		return U.Cross(V).Length();
	}

	const Math::FVector3& FQuad::GetOrigin() const {
		return Q;
	}

	const Math::FVector3& FQuad::GetU() const {
		return U;
	}

	const Math::FVector3& FQuad::GetV() const {
		return V;
	}

	FBox::FBox(const FVector3& A, const FVector3& B) {
		Math::FVector3 Min = Math::FVector3::Min(A, B);
		Math::FVector3 Max = Math::FVector3::Max(A, B);
		Math::FVector3 DX = Math::FVector3{ Max.X - Min.X, 0.0f, 0.0f };
		Math::FVector3 DY = Math::FVector3{ 0.0f, Max.Y - Min.Y, 0.0f };
		Math::FVector3 DZ = Math::FVector3{ 0.0f, 0.0f, Max.Z - Min.Z };
		Quads[0]={ {Min.X, Min.Y, Min.Z}, DY, DX }; // front
		Quads[1]={ {Max.X, Min.Y, Min.Z}, DY, DZ }; // right
		Quads[2]={ {Min.X, Min.Y, Max.Z}, DX, DY }; // back
		Quads[3]={ {Min.X, Min.Y, Min.Z}, DZ, DY }; // left
		Quads[4]={ {Min.X, Max.Y, Min.Z}, DZ, DX }; // top
		Quads[5]={ {Min.X, Min.Y, Min.Z}, DX, DZ }; // bottom
	}

	bool FBox::TestRay(const FRay& InRay, float DistanceMin, float DistanceMax, FRayHit& OutHit) const {
		bool bHit = false;
		for (const auto& Quad : Quads) {
			if (Quad.TestRay(InRay, DistanceMin, DistanceMax, OutHit)) {
				bHit = true;
				DistanceMax = OutHit.Distance;
			}
		}
		return bHit;
	}
}
