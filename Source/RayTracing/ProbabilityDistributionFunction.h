#pragma once
#include "Math/Vector.h"
#include "Math/OrthonormalBasis.h"

class RayTracingHittable;

class FProbabilityDistributionFuncionBase{
public:
    virtual ~FProbabilityDistributionFuncionBase();
    virtual float GetPDFValue(const Math::FVector3& InDirection) = 0;
    virtual Math::FVector3 GenerateDirection() = 0;
};

class FSpherePDF: public FProbabilityDistributionFuncionBase{
public:
    FSpherePDF(){}
    virtual float GetPDFValue(const Math::FVector3& InDirection) override;
    virtual Math::FVector3 GenerateDirection() override;
};

class FCosinePDF: public FProbabilityDistributionFuncionBase{
public:
    FCosinePDF(const Math::FVector3& InSurfaceNormal);
    virtual float GetPDFValue(const Math::FVector3& InDirection) override;
    virtual Math::FVector3 GenerateDirection() override;
private:
    Math::FOrthNormalBasis UVW;
};

// Sampling directions towards hitable.
class FHittablePDF: public FProbabilityDistributionFuncionBase{
public:
    FHittablePDF(const RayTracingHittable* InHittable, const Math::FVector3& InOrigin);
    virtual float GetPDFValue(const Math::FVector3& InDirection) override;
    virtual Math::FVector3 GenerateDirection() override;
private:
    const RayTracingHittable* Hittable;
    Math::FVector3 Origin;
};

// Mixture pdf
class FMixturePDF: public FProbabilityDistributionFuncionBase {
public:
    FMixturePDF(FProbabilityDistributionFuncionBase* InP0, FProbabilityDistributionFuncionBase* InP1);
    virtual float GetPDFValue(const Math::FVector3& InDirection) override;
    virtual Math::FVector3 GenerateDirection() override;
private:
    FProbabilityDistributionFuncionBase* P0;
    FProbabilityDistributionFuncionBase* P1;
};