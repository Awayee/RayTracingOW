#include "RayTracing/Texture.h"

Math::FVector4 Texture::SampleVector4(Math::FVector2 Texcoord, const Math::FVector3& Point) const {
	const Math::Color8 Color8 = SampleColor8(Texcoord, Point);
	return Math::FVector4{
		Math::UnpackUNorm(Color8.R),
		Math::UnpackUNorm(Color8.G),
		Math::UnpackUNorm(Color8.B),
		Math::UnpackUNorm(Color8.A) };
}

SolidColor::SolidColor(Math::Color8 InAlbedo): Albedo(InAlbedo) {
}

Math::Color8 SolidColor::SampleColor8(Math::FVector2 Texcoord, const Math::FVector3& Point) const {
	return Albedo;
}

CheckerTexture::CheckerTexture(Math::Color8 InColorEven, Math::Color8 InColorOdd, float InScale): ColorEven(InColorEven), ColorOdd(InColorOdd), InvScale(1.0f / InScale) {
}

Math::Color8 CheckerTexture::SampleColor8(Math::FVector2 Texcoord, const Math::FVector3& Point) const {
	const int32 X = (int32)Math::Floor(Point.X * InvScale);
	const int32 Y = (int32)Math::Floor(Point.Y * InvScale);
	const int32 Z = (int32)Math::Floor(Point.Z * InvScale);
	const bool bEven = ((X + Y + Z) % 2) == 0;
	return bEven ? ColorEven : ColorOdd;
}
