#include "Ray.h"
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
}
