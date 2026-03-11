#pragma once
#include "Vector.h"
namespace Math {

	float Random01();

	float Random(float min, float max);

	int RandomInt(int Min, int Max);

	FVector3 Random01Vector();

	FVector3 RandomVector(float min, float max);

	FVector3 RandomUintVector();

	FVector3 RandomUniformOnSphere();

	FVector3 RandomOnHemisphere(const FVector3& normal);

	FVector3 RandomCosineDirection();

	FVector2 RandomInDisk();


	inline Math::FVector3 Vector3Reflect(const Math::FVector3& v, const Math::FVector3& n) {
		return v - 2 * v.Dot(n) * n;
	}

	inline Math::FVector3 Vector3Refract(const Math::FVector3& v, const Math::FVector3& n, float etaDivision) {
		float cosTheta = n.Dot(-v);
		Math::FVector3 outPerp = etaDivision * (v + cosTheta * n);
		Math::FVector3 outParallel = -Math::Sqrt(Math::Abs(1.0f - outPerp.LengthSquared())) * n;
		return outPerp + outParallel;
	}
}