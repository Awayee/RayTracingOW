#include "Math/Perlin.h"

namespace Math {
	inline float TrilinearInterpolate(Math::FVector3* c, const Math::FVector3& Offset) {
		float Accum = 0.0f;
		for(uint8 Num=0; Num<8; ++Num) {
			Math::FVector3 Weight{
			(Num & 0b100) ? (Offset.X - 1.0f) : Offset.X,
			(Num & 0b010) ? (Offset.Y - 1.0f) : Offset.Y,
			(Num & 0b001) ? (Offset.Z - 1.0f) : Offset.Z
			};
			Accum += c[Num].Dot( Weight) * 
			((Num & 0b100) ? Offset.X : (1.0f - Offset.X)) *
			((Num & 0b010) ? Offset.Y : (1.0f - Offset.Y)) *
			((Num & 0b001) ? Offset.Z : (1.0f - Offset.Z));
		}
		return Accum;
	}

	Perlin::Perlin() {
		//for(int i=0; i<POINT_COUNT; ++i) {
		//	RandFloat[i] = Random01();
		//}
		for(int i=0; i<POINT_COUNT; ++i) {
			RandVectors[i] = RandomUnitVector();
		}
		PerlinGeneratePerm(PermX, POINT_COUNT);
		PerlinGeneratePerm(PermY, POINT_COUNT);
		PerlinGeneratePerm(PermZ, POINT_COUNT);
	}

	//float Perlin::Noise(const Math::FVector3& P) const {
	//	int i = int(4 * P.X) & 255;
	//	int j = int(4 * P.Y) & 255;
	//	int k = int(4 * P.Z) & 255;

	//	return RandFloat[PermX[i] ^ PermY[j] ^ PermZ[k]];
	//}

	float Perlin::SmoothNoise(const Math::FVector3& P) const {
		Math::FVector3 Offset {
			P.X - Math::Floor(P.X),
			P.Y - Math::Floor(P.Y),
			P.Z - Math::Floor(P.Z)
		};

		Offset = Offset * Offset * ( Math::FVector3{3.0f} - 2.0f * Offset);

		int i = (int)Math::Floor(P.X);
		int j = (int)Math::Floor(P.Y);
		int k = (int)Math::Floor(P.Z);

		Math::FVector3 c[8];
		for(int di=0; di<2; ++di) {
			for(int dj=0; dj < 2; ++dj) {
				for(int dk=0; dk <2; ++dk) {
					c[(di << 2) | (dj << 1) | dk] = RandVectors[
						PermX[(i + di) & 255] ^
						PermY[(j + dj) & 255] ^
						PermZ[(k + dk) & 255]
					];
				}
			}
		}
		return TrilinearInterpolate(c, Offset);
	}

	float Perlin::Turbulence(const Math::FVector3& P, int Depth) const {
		float Accum = 0.0f;
		Math::FVector3 TempP = P;
		float Weight = 1.0f;
		for(int i=0; i<Depth; ++i) {
			Accum += Weight * SmoothNoise(TempP);
			Weight *= 0.5f;
			TempP *= 2.0f;
		}
		return Math::Abs(Accum);
	}

	void Perlin::PerlinGeneratePerm(int* P, int N) {
		for(int i=0; i<N; ++i) {
			P[i] = i;
		}

		for(int i=N-1; i>0; --i) {
			const int Target = RandomInt(0, i);
			const int Temp = P[i];
			P[i] = P[Target];
			P[Target] = Temp;
		}
	}
}
