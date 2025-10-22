#pragma once
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
		Color8(float r, float g, float b, float a) : R(PackUNorm(r)), G(PackUNorm(g)), B(PackUNorm(b)), A(PackUNorm(a)) {}
		explicit Color8(uint32 hex) : Hex(hex) {}
		explicit Color8(const FVector4& inVec) :Color8(inVec.X, inVec.Y, inVec.Z, inVec.W) {}
		explicit Color8(const FVector3& inVec) :Color8(inVec.X, inVec.Y, inVec.Z, 1.0f) {}
		bool operator==(Color8 Other) const {
			return Hex ==Other.Hex;
		}
	};
}