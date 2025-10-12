#include "RayTracing/RayTracingRenderer.h"
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
	const float SampleScale = 1.0f / (float)NumRaysPerPixel;
	const Math::USize RenderSize = Camera->GetRenderSize();

	// parallel rendering
	size_t NumPixels = (size_t)(RenderSize.X * RenderSize.Y);
	Pixels.resize(NumPixels);
	ParallelFor(0ull, NumPixels, [this, SampleScale, RenderSize](size_t Pixel) {
		uint32 RenderWidth = RenderSize.X;
		uint32 i = (uint32)Pixel % RenderWidth;
		uint32 j = (uint32)Pixel / RenderWidth;
		Math::FVector4 Color = Math::FVector4::ZERO;
		for (uint32 Sample = 0; Sample < NumRaysPerPixel; ++Sample) {
			//const Math::FRay Ray = Camera->GetRandomRay(i, j);
			//Color += ComputeRayResult(Ray, RecursiveDepth);
			const Math::FRayWithTime Ray = Camera->GetRandomRayWithTime(i, j);
			Color += ComputeRayResultWithTime(Ray, RecursiveDepth);
		}
		Color *= SampleScale;
		const Math::Color8 ColorUNorm{Color};
		Pixels[j * RenderWidth + i] = ColorUNorm;
	});
}

RenderResult RayTracingRenderer::GetRenderResult() const {
	const Math::USize RenderSize = Camera->GetRenderSize();
	return RenderResult{ RenderSize.X, RenderSize.Y, Pixels.data()};
}

Math::FVector4 RayTracingRenderer::ComputeRayResult(const Math::FRay& Ray, uint32 RecursiveDepth) {
	if (0u == RecursiveDepth) {
		return Math::FVector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	}
	RayHitSurface hit;
	if (Scene->TestRay(Ray, 0.001f, RAY_MAX_DISTANCE, hit)) {
		//const FVector3 direction = RandomOnHemisphere(hit.Normal);
		if (hit.Material) {
			Math::FVector4 color;
			Math::FRay newRay;
			if (hit.Material->Scatter(Ray, hit.Geometry, color, newRay)) {
				return color * ComputeRayResult(newRay, RecursiveDepth - 1);
			}
		}
		return Math::FVector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	}
	else {
		return RayFallback(Ray);
	}
}

Math::FVector4 RayTracingRenderer::ComputeRayResultWithTime(const Math::FRayWithTime& Ray, uint32 RecursiveDepth) {
	if (0u == RecursiveDepth) {
		return Math::FVector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	}
	RayHitSurface Hit;
	if (Scene->TestRayWithTime(Ray, 0.001f, RAY_MAX_DISTANCE, Hit)) {
		//const FVector3 direction = RandomOnHemisphere(hit.Normal);
		if (Hit.Material) {
			Math::FVector4 Color;
			Math::FRayWithTime NewRay;
			if (Hit.Material->ScatterWithTime(Ray, Hit.Geometry, Color, NewRay)) {
				return Color * ComputeRayResult(NewRay, RecursiveDepth - 1);
			}
		}
		return Math::FVector4{ 1.0f, 1.0f, 1.0f, 1.0f };
	}
	else {
		return RayFallback(Ray);
	}
}

Math::FVector4 RayTracingRenderer::RayFallback(const Math::FRay& Ray) {
	// Return sky color
	const float Alpha = Math::Clamp(Ray.Direction.Y * 0.5f + 0.5f, 0.0f, 1.0f);
	static const Math::FVector3 Color0{ 1.0f, 1.0f, 1.0f };
	static const Math::FVector3 Color1{ 0.5f, 0.7f, 1.0f };
	const Math::FVector3 Color = Alpha * Color1 + (1.0f - Alpha) * Color0;
	return Math::FVector4{ Color, 1.0f };
}
