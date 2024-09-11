#pragma once
#include "Math/MathUtil.h"
class MaterialBase {
public:
	virtual ~MaterialBase() = default;
	virtual bool Scatter(const Math::FRay& inRay, const Math::FRayHit& rayHit, Math::FVector4& outColor, Math::FRay& outRay) const = 0;
};

typedef std::unique_ptr<MaterialBase> MaterialPtr;

class LambertMaterial: public MaterialBase {
public:
	LambertMaterial(const Math::FVector4& albedo) : m_Albedo(albedo) {}
	LambertMaterial(const Math::FVector3& albedo) : m_Albedo(albedo.X, albedo.Y, albedo.Z, 1.0f) {}
	bool Scatter(const Math::FRay& inRay, const Math::FRayHit& rayHit, Math::FVector4& outColor, Math::FRay& outRay) const override;
private:
	Math::FVector4 m_Albedo;
};

class MetalMaterial: public MaterialBase {
public:
	MetalMaterial(const Math::FVector4& albedo, float fuzz=0.0f) : m_Albedo(albedo), m_Fuzz(fuzz){}
	MetalMaterial(const Math::FVector3& albedo, float fuzz=0.0f): m_Albedo(albedo.X, albedo.Y, albedo.Z, 1.0f), m_Fuzz(fuzz) {}
	bool Scatter(const Math::FRay& inRay, const Math::FRayHit& rayHit, Math::FVector4& outColor, Math::FRay& outRay) const override;
private:
	Math::FVector4 m_Albedo;
	float m_Fuzz;
};

class DielectricMaterial: public MaterialBase {
public:
	DielectricMaterial(float refractionIndex) : m_RefractionIndex(refractionIndex) {}
	bool Scatter(const Math::FRay& inRay, const Math::FRayHit& rayHit, Math::FVector4& outColor, Math::FRay& outRay) const override;
private:
	float m_RefractionIndex;
};