#pragma once
#include "Vector.h"

namespace Math{
	struct FOrthNormalBasis{
		FVector3 U;
		FVector3 V;
		FVector3 W;
		FOrthNormalBasis(const Math::FVector3& Normal) {
			W = Normal.Normalize();
			Math::FVector3 Temp;
			if(Math::Abs(W.X) > 0.99f){
				Temp = Math::FVector3{0.0f, 1.0f, 0.0f};
			}
			else{
				Temp = Math::FVector3{1.0f, 0.0f, 0.0f};
			}
			V = W.Cross(Temp).Normalize();
			U = W.Cross(V);
		}
		Math::FVector3 Transform(const Math::FVector3& Displace) const {
			return U * Displace.X + V * Displace.Y + W * Displace.Z;
		}
	};
}