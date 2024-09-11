#include "Material.h"

static float Reflectance(float cosine, float refractionIndex) {
	// Use Schlick's approximation for reflectance.
	float r0 = (1 - refractionIndex) / (1 + refractionIndex);
	r0 = r0 * r0;
	return r0 + (1 - r0) * Math::Pow((1 - cosine), 5.0f);
}

bool LambertMaterial::Scatter(const Math::FRay& inRay, const Math::FRayHit& rayHit, Math::FVector4& outColor, Math::FRay& outRay) const {
	Math::FVector3 scatterDirection = rayHit.Normal + Math::RandomUintVector();
	if (scatterDirection.IsNearlyZero())
		scatterDirection = rayHit.Normal;
	outRay = { rayHit.Position, scatterDirection };
	outColor = m_Albedo;
	return true;
}

bool MetalMaterial::Scatter(const Math::FRay& inRay, const Math::FRayHit& rayHit, Math::FVector4& outColor, Math::FRay& outRay) const {
	Math::FVector3 reflected = Math::Vector3Reflect(inRay.Direction, rayHit.Normal);
	reflected.NormalizeSelf();
	if(m_Fuzz > 0.0f){
		reflected += Math::RandomUintVector() * m_Fuzz;
	}
	outRay = { rayHit.Position, reflected };
	outColor = m_Albedo;
	return reflected.Dot(rayHit.Normal) > 0.0f;
}

bool DielectricMaterial::Scatter(const Math::FRay& inRay, const Math::FRayHit& rayHit, Math::FVector4& outColor, Math::FRay& outRay) const {
	outColor = Math::FVector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	float reflectionIndex = rayHit.FrontFace ? (1.0f / m_RefractionIndex) : m_RefractionIndex;
	Math::FVector3 rayDirection = inRay.Direction.Normalize();
	// Determine refract or reflect by theta: if the theta' is greater tran 90, reflect.
	bool isReflection = false;
	// With Schlick's approximation
	float cosTheta = -rayDirection.Dot(rayHit.Normal);
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
		outDirection = Math::Vector3Reflect(rayDirection, rayHit.Normal);
	}
	else {
		outDirection = Math::Vector3Refract(rayDirection, rayHit.Normal, reflectionIndex);
	}
	outRay = { rayHit.Position, outDirection };
	return true;
}
