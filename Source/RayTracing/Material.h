#pragma once
#include "Core/TUniquePtr.h"
#include "Math/Geometry.h"
#include "RayTracing/Texture.h"

class MaterialBase {
public:
	virtual ~MaterialBase() = default;
	virtual bool Scatter(const Math::FRay& Ray, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay, float& OutPDF) const;
	virtual bool ScatterWithTime(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRayWithTime& OutRay, float& OutPDF) const;
	virtual float ScatteringPDF(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, const Math::FRayWithTime& Scattered) const;
	virtual Math::FVector4 Emitted(const Math::FRayHit& RayHit) const;
	static const Math::FVector4 Fallback;
};

typedef TUniquePtr<MaterialBase> MaterialPtr;

class LambertMaterial: public MaterialBase {
public:
	explicit LambertMaterial(Math::Color8 InAlbedo);
	explicit LambertMaterial(TexturePtr&& InTexture);
	bool Scatter(const Math::FRay& Ray, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay, float& OutPDF) const override;
	virtual float ScatteringPDF(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, const Math::FRayWithTime& Scattered) const override;
private:
	TexturePtr Texture;

};

class MetalMaterial: public MaterialBase {
public:
	MetalMaterial(const Math::FVector4& albedo, float fuzz=0.0f) : Albedo(albedo), Fuzz(fuzz){}
	MetalMaterial(const Math::FVector3& albedo, float fuzz=0.0f): Albedo(albedo.X, albedo.Y, albedo.Z, 1.0f), Fuzz(fuzz) {}
	bool Scatter(const Math::FRay& Ray, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay, float& OutPDF) const override;
private:
	Math::FVector4 Albedo;
	float Fuzz;
};

class DielectricMaterial: public MaterialBase {
public:
	DielectricMaterial(float refractionIndex) : RefractionIndex(refractionIndex) {}
	bool Scatter(const Math::FRay& Ray, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay, float& OutPDF) const override;
private:
	float RefractionIndex;
};

class DiffuseLightMaterial: public MaterialBase {
public:
	DiffuseLightMaterial(TexturePtr&& InTexture, float InScale);
	explicit DiffuseLightMaterial(Math::Color8 InColor, float InScale);
	Math::FVector4 Emitted(const Math::FRayHit& RayHit) const override;
private:
	TexturePtr Texture;
	float Scale;
};

class IsotropicMaterial: public MaterialBase {
public:
	explicit IsotropicMaterial(TexturePtr&& InTexture);
	bool Scatter(const Math::FRay& Ray, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay, float& OutPDF) const override;
	virtual Math::FVector4 Emitted(const Math::FRayHit& RayHit) const override;
private:
	TexturePtr Texture;
};