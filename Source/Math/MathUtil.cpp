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

	FVector3 Random01Vector() {
		return FVector3{ Random01(), Random01(), Random01() };
	}

	FVector3 RandomVector(float min, float max) {
		return FVector3{ Random(min, max), Random(min, max), Random(min, max) };
	}

	FVector3 RandomUintVector() {
		FVector3 result = RandomVector(-1.0f, 1.0f);
		float lengthSq = result.LengthSquared();
		if (lengthSq < FLT_MIN) {
			result = FVector3{ 0,1,0 };
		}
		else if (lengthSq > 1.0f) {
			result.NormalizeSelf();
		}
		result.NormalizeSelf();
		return result;
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

	FVector2 RandomInDisk() {
		FVector2 result{ Random(-1.0f, 1.0f), Random(-1.0f, 1.0f) };
		if (result.LengthSquared() > 1.0f) {
			result.NormalizeSelf();
		}
		return result;
	}
}
