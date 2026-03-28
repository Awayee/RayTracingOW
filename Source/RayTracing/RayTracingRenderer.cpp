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
					((float)dx + Math::Random01()) * InverseNumRaysSqrt - 0.5f,
					((float)dy + Math::Random01()) * InverseNumRaysSqrt - 0.5f,
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
		
		// Replace NaN components with zero.
		for(int32 Comp=0; Comp <3; ++Comp){
			if(Color[Comp] != Color[Comp]){
				Color[Comp] = 0.0f;
			}
		}
		
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
		return Scene->RayFallback(Ray);
	}
	RayHitSurface Hit;
	if (!Scene->TestRayWithTime(Ray, 0.001f, RAY_MAX_DISTANCE, Hit)) {
		return Scene->RayFallback(Ray);
	}

	//if(!Hit.Geometry.FrontFace){
	//	return Scene->RayFallback(Ray);
	//}

	//const FVector3 direction = RandomOnHemisphere(hit.Normal);
	if (!Hit.Material) {
		// Default material color;
		return MaterialBase::Fallback;
	}

	// Emit
	const Math::FVector4 EmittedColor = Hit.Material->Emitted(Hit.Geometry);

	FScatterRecord ScatterRecord;
	if(!Hit.Material->Scatter(Ray, Hit.Geometry, ScatterRecord)) {
		return EmittedColor;
	}

	if(!ScatterRecord.bContinue){
		return EmittedColor + ScatterRecord.Attenuation;
	}

	// Do not scattered.
	if(!ScatterRecord.PDF.Get()) {
		return ScatterRecord.Attenuation * ComputeRayResultWithTime(ScatterRecord.Scattered, Depth - 1);
	}

	Math::FRayWithTime Scattered;
	float PDFValue;

	// Sample lights
	const ObjectArray& Lights = Scene->GetLights();

	if(!Lights.empty()) {
		FHittableListPDF PDF{ Lights, Hit.Geometry.Position };
		FMixturePDF MixturePDF{ ScatterRecord.PDF.Get(), &PDF};
		// Scattered ray
		const Math::FVector3 ScatteredDirection = MixturePDF.GenerateDirection();
		Scattered = Math::FRayWithTime{ Hit.Geometry.Position, ScatteredDirection, Ray.Time };
		PDFValue = MixturePDF.GetPDFValue(ScatteredDirection);
	}
	else {
		const Math::FVector3 ScatteredDirection = ScatterRecord.PDF->GenerateDirection();
		Scattered = Math::FRayWithTime{ Hit.Geometry.Position, ScatteredDirection, Ray.Time };
		PDFValue = ScatterRecord.PDF->GetPDFValue(ScatteredDirection);
	}

	const float ScatteringPDF = Hit.Material->ScatteringPDF(Ray, Hit.Geometry, Scattered);

	// Recursively ray sampling
	const Math::FVector4 SampledColor = ComputeRayResultWithTime(Scattered, Depth - 1);
	const Math::FVector4 ScatteredColor = ScatterRecord.Attenuation * ScatteringPDF * SampledColor / PDFValue;
	return ScatteredColor + EmittedColor;
}
