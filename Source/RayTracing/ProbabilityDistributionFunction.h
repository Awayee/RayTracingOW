#pragma once
#include "Math/Vector.h"
#include "Math/OrthonormalBasis.h"
#include <vector>

#include "Core/TUniquePtr.h"

class RayTracingHittable;

class FPDFBase{
public:
    virtual ~FPDFBase();
    virtual float GetPDFValue(const Math::FVector3& InDirection) = 0;
    virtual Math::FVector3 GenerateDirection() = 0;
};

class FSpherePDF: public FPDFBase{
public:
    FSpherePDF(){}
    virtual float GetPDFValue(const Math::FVector3& InDirection) override;
    virtual Math::FVector3 GenerateDirection() override;
};

class FCosinePDF: public FPDFBase{
public:
    FCosinePDF(const Math::FVector3& InSurfaceNormal);
    virtual float GetPDFValue(const Math::FVector3& InDirection) override;
    virtual Math::FVector3 GenerateDirection() override;
private:
    Math::FOrthNormalBasis UVW;
};

// Sampling directions towards hitable.
class FHittablePDF: public FPDFBase{
public:
    FHittablePDF(const RayTracingHittable* InHittable, const Math::FVector3& InOrigin);
    virtual float GetPDFValue(const Math::FVector3& InDirection) override;
    virtual Math::FVector3 GenerateDirection() override;
private:
    const RayTracingHittable* Hittable;
    Math::FVector3 Origin;
};

// Mixture pdf
class FMixturePDF: public FPDFBase {
public:
    FMixturePDF(FPDFBase* InP0, FPDFBase* InP1);
    virtual float GetPDFValue(const Math::FVector3& InDirection) override;
    virtual Math::FVector3 GenerateDirection() override;
private:
    FPDFBase* P0;
    FPDFBase* P1;
};


class FHittableListPDF: public FPDFBase {
public:
    FHittableListPDF(const std::vector<TUniquePtr<RayTracingHittable>>& InObjects, const Math::FVector3& InOrigins);
    virtual float GetPDFValue(const Math::FVector3& InDirection) override;
    virtual Math::FVector3 GenerateDirection() override;
private:
    const std::vector<TUniquePtr<RayTracingHittable>>& Objects;
    Math::FVector3 Origin;
};