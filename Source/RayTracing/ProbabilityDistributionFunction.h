#pragma once
#include "Math/Vector.h"
#include "Math/OrthonormalBasis.h"

class FProbabilityDistributionFuncionBase{
public:
    virtual ~FProbabilityDistributionFuncionBase();
    virtual float GetPDFValue(const Math::FVector3& InDirection) = 0;
    virtual Math::FVector3 GenerateDirection() = 0;
};

class FSpherePDF: public FProbabilityDistributionFuncionBase{
public:
    FSpherePDF(){};
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