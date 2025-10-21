#pragma once
#include "Math/Vector.h"
#include "Math/MathUtil.h"

namespace Math {
	class Perlin {
	public:
		Perlin();

		//float Noise(const Math::FVector3& P) const;

		// return [-1, 1]
		float SmoothNoise(const Math::FVector3& P)const;

		float Turbulence(const Math::FVector3& P, int Depth) const;

	private:
		static constexpr int POINT_COUNT = 256;
		//float RandFloat[POINT_COUNT];
		Math::FVector3 RandVectors[POINT_COUNT];
		int PermX[POINT_COUNT];
		int PermY[POINT_COUNT];
		int PermZ[POINT_COUNT];


		static void PerlinGeneratePerm(int* P, int N);
	};
}