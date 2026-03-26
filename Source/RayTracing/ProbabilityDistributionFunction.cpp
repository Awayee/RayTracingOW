#include "Math/MathUtil.h"
#include "ProbabilityDistributionFunction.h"
#include "RayTracing/RayTracingObject.h"

FPDFBase::~FPDFBase(){}

float FSpherePDF::GetPDFValue(const Math::FVector3& InDirection) {
    return 1.0f / (4.0f * Math::PI);
}

Math::FVector3 FSpherePDF::GenerateDirection(){
    return Math::RandomUnitVector();    
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

FMixturePDF::FMixturePDF(FPDFBase* InP0, FPDFBase* InP1): P0(InP0), P1(InP1) {
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

FHittableListPDF::FHittableListPDF(const std::vector<TUniquePtr<RayTracingHittable>>& InObjects, const Math::FVector3& InOrigin): Objects(InObjects), Origin(InOrigin) {
}

float FHittableListPDF::GetPDFValue(const Math::FVector3& InDirection) {
    float Sum = 0.0f;
    for(const RTObjectPtr& Object: Objects) {
        Sum += Object->GetPDFValue(Origin, InDirection);
    }
    const float Weight = 1.0f / (float)Objects.size();
    return Sum * Weight;
}

Math::FVector3 FHittableListPDF::GenerateDirection() {
    const int32 RandomIndex = Math::RandomInt(0, (int32)Objects.size() - 1);
    return Objects[RandomIndex]->Random(Origin);
}
