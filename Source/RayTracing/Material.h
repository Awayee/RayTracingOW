#pragma once
#include "Core/TUniquePtr.h"
#include "Math/Geometry.h"
#include "RayTracing/Texture.h"

class MaterialBase {
public:
	virtual ~MaterialBase() = default;
	virtual bool Scatter(const Math::FRay& Ray, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay) const = 0;
	virtual bool ScatterWithTime(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRayWithTime& OutRay) const;
};

typedef TUniquePtr<MaterialBase> MaterialPtr;

class LambertMaterial: public MaterialBase {
public:
	explicit LambertMaterial(const Math::FVector4& InAlbedo);
	explicit LambertMaterial(TexturePtr&& InTexture);
	LambertMaterial(const Math::FVector3& InAlbedo);
	bool Scatter(const Math::FRay& Ray, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay) const override;
private:
	TexturePtr Texture;

};

class MetalMaterial: public MaterialBase {
public:
	MetalMaterial(const Math::FVector4& albedo, float fuzz=0.0f) : Albedo(albedo), Fuzz(fuzz){}
	MetalMaterial(const Math::FVector3& albedo, float fuzz=0.0f): Albedo(albedo.X, albedo.Y, albedo.Z, 1.0f), Fuzz(fuzz) {}
	bool Scatter(const Math::FRay& Ray, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay) const override;
private:
	Math::FVector4 Albedo;
	float Fuzz;
};

class DielectricMaterial: public MaterialBase {
public:
	DielectricMaterial(float refractionIndex) : RefractionIndex(refractionIndex) {}
	bool Scatter(const Math::FRay& Ray, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay) const override;
private:
	float RefractionIndex;
};