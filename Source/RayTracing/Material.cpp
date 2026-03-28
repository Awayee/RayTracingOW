#include "Material.h"
#include "Math/OrthonormalBasis.h"
#include "Math/MathUtil.h"
#include "Core/Log.h"
#include "RayTracing/RayTracingObject.h"
#include "RayTracing/ProbabilityDistributionFunction.h"

static float Reflectance(float cosine, float refractionIndex) {
    // Use Schlick's approximation for reflectance.
    float r0 = (1 - refractionIndex) / (1 + refractionIndex);
    r0 = r0 * r0;
    return r0 + (1 - r0) * Math::Pow((1.0f - cosine), 5.0f);
}

bool MaterialBase::Scatter(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, FScatterRecord& OutRec) const {
    return false;
}

float MaterialBase::ScatteringPDF(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, const Math::FRayWithTime& Scattered) const {
    return 0.0f;
}

Math::FVector4 MaterialBase::Emitted(const Math::FRayHit& RayHit) const {
    return { 0.0f, 0.0f, 0.0f, 0.0f };
}

const Math::FVector4 MaterialBase::Fallback = Math::FVector4{1.0f, 0.0f, 1.0f, 1.0f};

LambertMaterial::LambertMaterial(Math::Color8 InAlbedo) {
    Texture.Reset(new SolidColor(InAlbedo));
}

LambertMaterial::LambertMaterial(TexturePtr&& InTexture): Texture(MoveTemp(InTexture)) {
}

bool LambertMaterial::Scatter(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, FScatterRecord& OutRec) const {
    OutRec.Attenuation = Texture->SampleVector4(RayHit.Texcoord, RayHit.Position);
    OutRec.PDF = TUniquePtr<FPDFBase>(new FCosinePDF(RayHit.Normal));
    return true;
}

float LambertMaterial::ScatteringPDF(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, const Math::FRayWithTime& Scattered) const {
     const float CosTheta = RayHit.Normal.Dot(Scattered.Direction);
     return CosTheta < 0.0f ? 0.0f : CosTheta / Math::PI;
    /*return 1.0f / (2.0f * Math::PI);*/
}

bool MetalMaterial::Scatter(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, FScatterRecord& OutRec) const {
    Math::FVector3 Reflected = Math::Vector3Reflect(InRay.Direction, RayHit.Normal).Normalize();
    if(Fuzz > 0.0f) {
        Reflected += Math::RandomUnitVector() * Fuzz;
    }

    OutRec.Attenuation = Albedo;
    OutRec.PDF.Reset();
    OutRec.Scattered = Math::FRayWithTime{ RayHit.Position, Reflected, InRay.Time };
    return true;
}

bool DielectricMaterial::Scatter(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, FScatterRecord& OutRec) const {
    OutRec.Attenuation= Math::FVector4{ 1.0f, 1.0f, 1.0f, 1.0f };
    float RI = RayHit.FrontFace ? (1.0f / RefractionIndex) : RefractionIndex;
    Math::FVector3 RayDirection = InRay.Direction.Normalize();
    // Determine refract or reflect by theta: if the theta' is greater tran 90, reflect.
    bool IsReflection = false;
    // With Schlick's approximation
    // const float CosTheta = -RayDirection.Dot(RayHit.Normal);
    const float CosTheta = -RayDirection.Dot(RayHit.Normal);
    const float ReflectanceValue = Reflectance(CosTheta, RI);
    if (ReflectanceValue > Math::Random01()) {
        IsReflection = true;
    }
    if (!IsReflection) {
        float SinTheta = Math::Sqrt(1.0f - CosTheta * CosTheta);
        float SininThetaRefracted = RI * SinTheta;
        IsReflection = SininThetaRefracted > 1.0f;
    }
    Math::FVector3 OutDirection;
    if (IsReflection) {
        OutDirection = Math::Vector3Reflect(RayDirection, RayHit.Normal);
    }
    else {
        OutDirection = Math::Vector3Refract(RayDirection, RayHit.Normal, RI);
    }
    OutRec.Scattered = { RayHit.Position, OutDirection, InRay.Time };
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

bool IsotropicMaterial::Scatter(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, FScatterRecord& OutRec) const {
    OutRec.Attenuation = Texture->SampleVector4(RayHit.Texcoord, RayHit.Position);
    OutRec.PDF = TUniquePtr<FPDFBase>(new FSpherePDF());
    return true;
}

float IsotropicMaterial::ScatteringPDF(const Math::FRayWithTime& InRay, const Math::FRayHit& RayHit, const Math::FRayWithTime& Scattered) const {
    return 1.0f / (4.0f * Math::PI);
}

Math::FVector4 IsotropicMaterial::Emitted(const Math::FRayHit &RayHit) const {
    if(RayHit.FrontFace){
        return Texture->SampleVector4(RayHit.Texcoord, RayHit.Position);
    }
    return MaterialBase::Emitted(RayHit);
}
