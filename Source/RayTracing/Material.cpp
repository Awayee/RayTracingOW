#include "Material.h"
#include "Math/MathUtil.h"

static float Reflectance(float cosine, float refractionIndex) {
	// Use Schlick's approximation for reflectance.
	float r0 = (1 - refractionIndex) / (1 + refractionIndex);
	r0 = r0 * r0;
	return r0 + (1 - r0) * Math::Pow((1 - cosine), 5.0f);
}

bool MaterialBase::Scatter(const Math::FRay& Ray, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay) const {
	return false;
}

bool MaterialBase::ScatterWithTime(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRayWithTime& OutRay) const {
	if(Scatter((const Math::FRay&)InRay, RayHit, OutColor, (Math::FRay&)OutRay)) {
		OutRay.Time = InRay.Time;
		return true;
	}
	return false;	
}

Math::FVector4 MaterialBase::Emitted(const Math::FRayHit& RayHit) const {
	return { 0.0f, 0.0f, 0.0f, 0.0f };
}

LambertMaterial::LambertMaterial(Math::Color8 InAlbedo) {
	Texture.Reset(new SolidColor(InAlbedo));
}

LambertMaterial::LambertMaterial(TexturePtr&& InTexture): Texture(MoveTemp(InTexture)) {
}

bool LambertMaterial::Scatter(const Math::FRay& InRay, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay) const {
	Math::FVector3 scatterDirection = RayHit.Normal + Math::RandomUintVector();
	if (scatterDirection.IsNearlyZero())
		scatterDirection = RayHit.Normal;
	OutRay = { RayHit.Position, scatterDirection };
	OutColor = Texture->SampleVector4(RayHit.Texcoord, RayHit.Position);
	return true;
}

bool MetalMaterial::Scatter(const Math::FRay& InRay, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay) const {
	Math::FVector3 reflected = Math::Vector3Reflect(InRay.Direction, RayHit.Normal);
	reflected.NormalizeSelf();
	if(Fuzz > 0.0f){
		reflected += Math::RandomUintVector() * Fuzz;
	}
	OutRay = { RayHit.Position, reflected };
	OutColor = Albedo;
	return reflected.Dot(RayHit.Normal) > 0.0f;
}

bool DielectricMaterial::Scatter(const Math::FRay& InRay, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay) const {
	OutColor = Math::FVector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	float reflectionIndex = RayHit.FrontFace ? (1.0f / RefractionIndex) : RefractionIndex;
	Math::FVector3 rayDirection = InRay.Direction.Normalize();
	// Determine refract or reflect by theta: if the theta' is greater tran 90, reflect.
	bool isReflection = false;
	// With Schlick's approximation
	float cosTheta = -rayDirection.Dot(RayHit.Normal);
	float reflectance = Reflectance(cosTheta, reflectionIndex);
	if(reflectance > Math::Random01()) {
		isReflection = true;
	}
	if(!isReflection) {
		float sinTheta = Math::Sqrt(1.0f - cosTheta * cosTheta);
		float sinThetaRefracted = reflectionIndex * sinTheta;
		isReflection = sinThetaRefracted > 1.0f;
	}
	Math::FVector3 outDirection;
	if(isReflection) {
		outDirection = Math::Vector3Reflect(rayDirection, RayHit.Normal);
	}
	else {
		outDirection = Math::Vector3Refract(rayDirection, RayHit.Normal, reflectionIndex);
	}
	OutRay = { RayHit.Position, outDirection };
	return true;
}

DiffuseLightMaterial::DiffuseLightMaterial(TexturePtr&& InTexture, float InScale) : Texture(MoveTemp(InTexture)), Scale(InScale){
}

DiffuseLightMaterial::DiffuseLightMaterial(Math::Color8 InColor, float InScale):Texture(new SolidColor(InColor)),Scale(InScale){
}

Math::FVector4 DiffuseLightMaterial::Emitted(const Math::FRayHit& RayHit) const {
	return Scale * Texture->SampleVector4(RayHit.Texcoord, RayHit.Position);
}

IsotropicMaterial::IsotropicMaterial(TexturePtr&& InTexture) : Texture(MoveTemp(InTexture)){
}

bool IsotropicMaterial::Scatter(const Math::FRay& Ray, const Math::FRayHit& RayHit, Math::FVector4& OutColor, Math::FRay& OutRay) const {
	OutRay=Math::FRay(RayHit.Position, Math::RandomUintVector());
	OutColor = Texture->SampleVector4(RayHit.Texcoord, RayHit.Position);
	return true;
}
