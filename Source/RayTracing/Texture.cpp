#include "RayTracing/Texture.h"
#include "Resource/ImageLoader.h"

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

ImageTexture::ImageTexture(const char* RelativeFilePath) {
	ImageLoader Loader(RelativeFilePath);
	if(Loader.IsValid()) {
		Width = (uint32)Loader.GetWidth();
		Height = (uint32)Loader.GetHeight();
		BytesPerPixel = (uint32)Loader.GetBytesPerPixel();
		Loader.GetData(Pixels);
	}
	else {
		Width = Height = BytesPerPixel = 0u;
	}
}

Math::Color8 ImageTexture::SampleColor8(Math::FVector2 Texcoord, const Math::FVector3& Point) const {
	if(Pixels.empty()) {
		return Math::Color8{0xffffffff};
	}
	const float u = Math::Clamp(Texcoord.X, 0.0f, 1.0f);
	const float v = 1.0f - Math::Clamp(Texcoord.Y, 0.0f, 1.0f);
	uint32 x = (uint32)(u * (float)Width);
	uint32 y = (uint32)(v * (float)Height);
	x = Math::Clamp(x, 0u, Width-1);
	y = Math::Clamp(y, 0u, Height-1);
	const uint32 Index = (y * Width + x) * BytesPerPixel;
	const uint8* Pixel = &Pixels[Index];

	Math::Color8 Color;
	const uint32 NumBytes = Math::Min(BytesPerPixel, 4u);
	for(uint32 i=0; i<NumBytes; ++i) {
		Color.Components[i] = Pixel[i];
	}
	return Color;
}
