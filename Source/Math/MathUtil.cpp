#include "MathUtil.h"

#include <random>

namespace Math {

	float Random01() {
		static std::uniform_real_distribution<float> distribution(0.0, 1.0);
		static std::mt19937 generator;
		return distribution(generator);
	}

	float Random(float min, float max) {
		return min + (max - min) * Random01();
	}

	int RandomInt(int Min, int Max) {
		return (int)Random((float)Min, (float)(Max+1));
	}

	FVector3 Random01Vector() {
		return FVector3{ Random01(), Random01(), Random01() };
	}

	FVector3 RandomVector(float min, float max) {
		return FVector3{ Random(min, max), Random(min, max), Random(min, max) };
	}

	FVector3 RandomUintVector() {
		FVector3 result = RandomVector(-1.0f, 1.0f);
		if (result.IsNearlyZero()) {
			result = FVector3{ 0,1,0 };
		}
		else{
			result.NormalizeSelf();
		}
		return result;
	}

    FVector3 RandomUniformOnSphere() {
        float R1 = Random01();
		float R2 = Random01();
		float Phi = 2.0f * Math::PI * R1;
		float CosTheta = 1.0f - 2.0f * R2;
		float SinTheta = 2.0f * Math::Sqrt(R2 * (1.0f - R2));
		return FVector3{
			Math::Cos(Phi) * SinTheta,
			Math::Sin(Phi) * SinTheta,
			CosTheta,	
		};
    }

    FVector3 RandomOnHemisphere(const FVector3& normal) {
		// 1. random generate a point until locate in unit sphere
		FVector3 result = RandomUintVector();
		// 2. map the point from sphere to hemisphere
		if (result.Dot(normal) < 0.0f) {
			return -result;
		}
		return result;
	}

    FVector3 RandomCosineDirection() {
		float R1 = Random01();
		float R2 = Random01();
		float Phi = 2 * Math::PI * R1;
		float SqrtR2 = Math::Sqrt(R2);
		return FVector3{
			Math::Cos(Phi) * SqrtR2,
			Math::Sin(Phi) * SqrtR2,
			Math::Sqrt(1.0f - R2)
		};
    }

    FVector2 RandomInDisk() {
		FVector2 result{ Random(-1.0f, 1.0f), Random(-1.0f, 1.0f) };
		if (result.LengthSquared() > 1.0f) {
			result.NormalizeSelf();
		}
		return result;
	}
}
