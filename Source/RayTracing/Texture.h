#pragma once
#include "Math/Color.h"
#include "Core/TUniquePtr.h"

class Texture{
public:
    virtual ~Texture() = default;
    virtual Math::Color8 SampleColor8(Math::FVector2 Texcoord, const Math::FVector3& Point) const = 0;
    virtual Math::FVector4 SampleVector4(Math::FVector2 Texcoord, const Math::FVector3& Point) const;
};

typedef TUniquePtr<Texture> TexturePtr;

class SolidColor: public Texture {
public:
    SolidColor(Math::Color8 InAlbedo);
    virtual Math::Color8 SampleColor8(Math::FVector2 Texcoord, const Math::FVector3& Point) const override;
private:
    Math::Color8 Albedo;
};

// Color grid
class CheckerTexture: public Texture {
public:
    CheckerTexture(Math::Color8 InColorEven, Math::Color8 InColorOdd, float InScale);
    virtual Math::Color8 SampleColor8(Math::FVector2 Texcoord, const Math::FVector3& Point) const override;
private:
    Math::Color8 ColorEven;
    Math::Color8 ColorOdd;
    float InvScale;
};