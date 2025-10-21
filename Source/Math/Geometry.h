#pragma once
#include "Vector.h"
namespace Math {
	struct FRay {
		FVector3 Origin;
		FVector3 Direction;
		FRay();
		FRay(const FVector3& InOrigin, const FVector3& InDirection);
		FVector3 At(float t) const;
	};

	struct FRayWithTime: FRay {
		float Time;
		FRayWithTime();
		FRayWithTime(const FVector3& InOrigin, const FVector3& InDirection, float InTime);
	};

	struct FRayHit {
		FVector3 Position;
		FVector3 Normal;
		FVector2 Texcoord;
		float Distance;
		bool FrontFace;
		FRayHit();
	};

	struct FSphere {
		FVector3 Center;
		float Radius;
		FSphere(const FVector3& center, float radius) : Center(center), Radius(Max(0.0f, radius)) {}
		float TestRay(const FRay& InRay) const;
		bool TestRay(const FRay& InRay, float DistanceMin, float DistanceMax, FRayHit& OutHit) const;
		static Math::FVector2 GetTexcoordByNormal(const Math::FVector3& Normal); // Normal is normlized
	};

	struct FAABB3
	{
		FVector3 Min;
		FVector3 Max;
		static FAABB3 CenterExtent(const Math::FVector3& Center, const FVector3& Extent);
		FVector3 Center() const;
		FVector3 Extent() const;
		bool IsValid() const;
		int GetMaxAxis() const;
		void Union(const FAABB3& Other);
		bool TestRay(const FRay& InRay, float DistanceMin, float DistanceMax) const;
	};

	struct FPlane {
		Math::FVector3 Normal;
		float D;
		FPlane(const Math::FVector3& InNormal, float InD);
		FPlane(const Math::FVector3& InQ, const Math::FVector3& InU, const Math::FVector3& InV);
		bool TestRay(const FRay& InRay, float DistanceMin, float DistanceMax, Math::FRayHit& OutHit) const;
	};


	class FQuad {
	public:
		FQuad() = default;
		FQuad(const FVector3& InQ, const FVector3& InU, const FVector3& InV);
		bool TestRay(const FRay& InRay, float DistanceMin, float DistanceMax, FRayHit& OutHit) const;
		FAABB3 GetAABB() const;
	private:
		//   V - 
		//  /  /
		// Q - U
		FVector3 Q;
		FVector3 U;
		FVector3 V;
		FPlane Plane;
	};
}