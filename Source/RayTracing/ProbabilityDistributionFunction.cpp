#include "Math/MathUtil.h"
#include "ProbabilityDistributionFunction.h"

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