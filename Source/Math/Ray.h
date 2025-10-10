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
}