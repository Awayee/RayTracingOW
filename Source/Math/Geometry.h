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
	};
}