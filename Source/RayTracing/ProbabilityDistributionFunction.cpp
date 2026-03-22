#include "Math/MathUtil.h"
#include "ProbabilityDistributionFunction.h"
#include "RayTracing/RayTracingObject.h"

FProbabilityDistributionFuncionBase::~FProbabilityDistributionFuncionBase(){}

float FSpherePDF::GetPDFValue(const Math::FVector3& InDirection) {
    return 1.0f / (4.0f * Math::PI);
}

Math::FVector3 FSpherePDF::GenerateDirection(){
    return Math::RandomUintVector();    
}

FCosinePDF::FCosinePDF(const Math::FVector3& InSurfaceNormal): UVW(InSurfaceNormal){
}

float FCosinePDF::GetPDFValue(const Math::FVector3& InDirection){
    float CosTheta = InDirection.Dot(UVW.W);
    return Math::Max(0.0f, CosTheta / Math::PI);
}

Math::FVector3 FCosinePDF::GenerateDirection(){
    return UVW.Transform(Math::RandomCosineDirection().Normalize());
}

FHittablePDF::FHittablePDF(const RayTracingHittable* InHittable, const Math::FVector3& InOrigin) : Hittable(InHittable), Origin(InOrigin){
}

float FHittablePDF::GetPDFValue(const Math::FVector3& InDirection) {
    return Hittable->GetPDFValue(Origin, InDirection);
}

Math::FVector3 FHittablePDF::GenerateDirection() {
    return Hittable->Random(Origin);
}

FMixturePDF::FMixturePDF(FProbabilityDistributionFuncionBase* InP0, FProbabilityDistributionFuncionBase* InP1): P0(InP0), P1(InP1) {
}

float FMixturePDF::GetPDFValue(const Math::FVector3& InDirection) {
    return P0->GetPDFValue(InDirection) * 0.5f + P1->GetPDFValue(InDirection) * 0.5f;
}

Math::FVector3 FMixturePDF::GenerateDirection() {
    const float RandomVal = Math::Random01();
    if(RandomVal < 0.5f) {
        return P0->GenerateDirection();
    }
    else {
        return P1->GenerateDirection();
    }
}
