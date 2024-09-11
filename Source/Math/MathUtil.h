#pragma once
#include <random>
#include "Vector.h"
namespace Math {
	struct Color8 {
	public:
		union {
			struct { uint8 R, G, B, A; };
			uint8 Components[4];
			uint32 Hex;
		};
		Color8() :Hex(0) {}
		Color8(uint8 r, uint8 g, uint8 b, uint8 a) :R(r), G(g), B(b), A(a) {}
		Color8(uint32 hex) : Hex(hex) {}
		Color8(float r, float g, float b, float a) : R(PackUNorm(r)), G(PackUNorm(g)), B(PackUNorm(b)), A(PackUNorm(a)) {}
		Color8(const FVector4& inVec) :Color8(inVec.X, inVec.Y, inVec.Z, inVec.W) {}
		Color8(const FVector3& inVec) :Color8(inVec.X, inVec.Y, inVec.Z, 1.0f) {}
	};

	struct FRay {
		FVector3 Origin;
		FVector3 Direction;
		FVector3 At(float t) const;
	};

	struct FRayHit {
		FVector3 Position;
		FVector3 Normal;
		float Distance;
		bool FrontFace;
	};

	struct FSphere {
		FVector3 Center;
		float Radius;
		FSphere(const FVector3& center, float radius) : Center(center), Radius(Max(0.0f, radius)) {}
		float RayHit(const FRay& ray) const;
		bool RayHit(const FRay& ray, float rayMin, float rayMax, FRayHit& outHit) const;
	};

	inline float Random01() {
		static std::uniform_real_distribution<float> distribution(0.0, 1.0);
		static std::mt19937 generator;
		return distribution(generator);
	}

	inline float Random(float min, float max) {
		return min + (max - min) * Random01();
	}

	inline FVector3 Random01Vector() {
		return FVector3{ Random01(), Random01(), Random01() };
	}

	inline FVector3 RandomVector(float min, float max) {
		return FVector3{ Random(min, max), Random(min, max), Random(min, max) };
	}

	inline FVector3 RandomUintVector() {
		FVector3 result = RandomVector(-1.0f, 1.0f);
		float lengthSq = result.LengthSquared();
		if(lengthSq < FLT_MIN) {
			result = FVector3{ 0,1,0 };
		}
		else if(lengthSq > 1.0f) {
			result.NormalizeSelf();
		}
		result.NormalizeSelf();
		return result;
	}

	inline FVector3 RandomOnHemisphere(const FVector3& normal) {
		// 1. random generate a point until locate in unit sphere
		FVector3 result = RandomUintVector();
		// 2. map the point from sphere to hemisphere
		if (result.Dot(normal) < 0.0f) {
			return -result;
		}
		return result;
	}

	inline FVector2 RandomInDisk() {
		FVector2 result{ Random(-1.0f, 1.0f), Random(-1.0f, 1.0f) };
		if(result.LengthSquared() > 1.0f) {
			result.NormalizeSelf();
		}
		return result;
	}


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