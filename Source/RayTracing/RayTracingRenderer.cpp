#include "RayTracing/RayTracingRenderer.h"
#include "RayTracing/ProbabilityDistributionFunction.h"
#include "RayTracing/RayTracingCamera.h"
#include "RayTracing/RayTracingScene.h"
#include "Core/Log.h"
#include "Core/ParallelFor.h"

static constexpr float  RAY_MAX_DISTANCE = 9999.9f;

RayTracingRenderer::RayTracingRenderer(RayTracingCamera* InCamera, RayTracingScene* InScene, uint32 InNumRaysPerPixel, uint32 InRecursiveDepth):
Camera(InCamera), Scene(InScene), NumRaysPerPixel(InNumRaysPerPixel), RecursiveDepth(InRecursiveDepth) {
}

void RayTracingRenderer::Render() {
	LOG_INFO("Scene is Rendering...");
	const uint32 NumRaysPerPiexelSqrt = (uint32)Math::Sqrt((float)NumRaysPerPixel);
	const float SampleScale = 1.0f / (float)(NumRaysPerPiexelSqrt * NumRaysPerPiexelSqrt);
	const float InverseNumRaysSqrt = 1.0f / (float)NumRaysPerPiexelSqrt;
	const Math::USize RenderSize = Camera->GetRenderSize();

	// parallel rendering
	size_t NumPixels = (size_t)(RenderSize.X * RenderSize.Y);
	Pixels.resize(NumPixels);
	ParallelFor(0ull, NumPixels, [this, SampleScale, RenderSize, NumRaysPerPiexelSqrt, InverseNumRaysSqrt](size_t Pixel) {
		uint32 RenderWidth = RenderSize.X;
		uint32 i = (uint32)Pixel % RenderWidth;
		uint32 j = (uint32)Pixel / RenderWidth;
		Math::FVector4 Color = Math::FVector4::ZERO;
		for (uint32 dy=0; dy<NumRaysPerPiexelSqrt; ++dy){
			for (uint32 dx=0; dx<NumRaysPerPiexelSqrt; ++dx){
				// Compute square stratified
				Math::FVector3 Offset{
					(dx + Math::Random01()) * InverseNumRaysSqrt - 0.5f,
					(dy + Math::Random01()) * InverseNumRaysSqrt - 0.5f,
					0.0f
				};
				// Get ray by stratified
				const Math::FRayWithTime Ray = Camera->GetRandomRayWithTimeOffset(i, j, Offset);
				Color += ComputeRayResultWithTime(Ray, RecursiveDepth);
			}
		}
		// for (uint32 Sample = 0; Sample < NumRaysPerPixel; ++Sample) {
		// 	//const Math::FRay Ray = Camera->GetRandomRay(i, j);
		// 	//Color += ComputeRayResult(Ray, RecursiveDepth);


		// 	const Math::FRayWithTime Ray = Camera->GetRandomRayWithTime(i, j);
		// 	Color += ComputeRayResultWithTime(Ray, RecursiveDepth);
		// }
		Color *= SampleScale;
		const Math::Color8 ColorUNorm{Color};
		Pixels[j * RenderWidth + i] = ColorUNorm;
	});
}

RenderResult RayTracingRenderer::GetRenderResult() const {
	const Math::USize RenderSize = Camera->GetRenderSize();
	return RenderResult{ RenderSize.X, RenderSize.Y, Pixels.data()};
}

Math::FVector4 RayTracingRenderer::ComputeRayResult(const Math::FRay& Ray, uint32 Depth) {
	if (0u == Depth) {
		return Math::FVector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	}
	RayHitSurface Hit;
	if (Scene->TestRay(Ray, 0.001f, RAY_MAX_DISTANCE, Hit)) {
		//const FVector3 direction = RandomOnHemisphere(hit.Normal);
		if (Hit.Material) {
			Math::FVector4 Color;
			Math::FRay OutRay;
			float PDFValue;
			if (Hit.Material->Scatter(Ray, Hit.Geometry, Color, OutRay, PDFValue)) {
				return Color * ComputeRayResult(OutRay, Depth - 1);
			}
		}
		return Math::FVector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	}
	else {
		return Scene->RayFallback(Ray);
	}
}

Math::FVector4 RayTracingRenderer::ComputeRayResultWithTime(const Math::FRayWithTime& Ray, uint32 Depth) {
	if (0u == Depth) {
		return Math::FVector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	}
	RayHitSurface Hit;
	if (!Scene->TestRayWithTime(Ray, 0.001f, RAY_MAX_DISTANCE, Hit)) {
		return Scene->RayFallback(Ray);
	}

	if(!Hit.Geometry.FrontFace){
		return Scene->RayFallback(Ray);
	}

	//const FVector3 direction = RandomOnHemisphere(hit.Normal);
	if (!Hit.Material) {
		// Default material color;
		return MaterialBase::Fallback;
	}

	const Math::FVector4 EmittedColor = Hit.Material->Emitted(Hit.Geometry);
	Math::FVector4 Attenuation;
	Math::FRayWithTime Scattered;
	float PDFValue;
	if (!Hit.Material->ScatterWithTime(Ray, Hit.Geometry, Attenuation, Scattered, PDFValue)) {
		return EmittedColor;
	}

	// TODO hard code light
	// Math::FVector3 OnLight = Math::FVector3{Math::Random(213.0f, 343.0f), 554.0f, Math::Random(227.0f, 332.0f)};
	// Math::FVector3 ToLight = OnLight - Hit.Geometry.Position;
	// float DistanceSq = ToLight.LengthSquared();
	// if(ToLight.Dot(Hit.Geometry.Normal) < 0.0f){
	// 	return EmittedColor;
	// }
	// ToLight.NormalizeSelf();
	// float LightArea = (343.0f - 213.0f) * (332.0f - 227.0f);
	// float LightCosine = Math::Abs(ToLight.Y);
	// if(LightCosine < 0.000001f){
	// 	return EmittedColor;
	// }
	// PDFValue = DistanceSq / (LightCosine * LightArea);

	// Scattering PDF
	FCosinePDF SurfacePDF{Hit.Geometry.Normal};
	Math::FVector3 ScatteredDir = SurfacePDF.GenerateDirection();
	Scattered = Math::FRayWithTime{Hit.Geometry.Position, ScatteredDir, Scattered.Time};
	PDFValue = SurfacePDF.GetPDFValue(ScatteredDir);

	float ScatteringPDF = Hit.Material->ScatteringPDF(Ray, Hit.Geometry, Scattered);

	Math::FVector4 ScatteredColor = Attenuation * ComputeRayResultWithTime(Scattered, Depth - 1);
	ScatteredColor = ScatteredColor * ScatteringPDF / PDFValue;
	return ScatteredColor + EmittedColor;
}
