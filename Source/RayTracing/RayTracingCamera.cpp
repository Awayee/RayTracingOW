#include "RayTracingCamera.h"
#include "Core/Log.h"
#include <random>
#include <ppl.h>

using namespace Math;

static constexpr float  RAY_MAX_DISTANCE = 9999.9f;
static constexpr uint32 RAY_PER_PIXEL = 16;
static constexpr uint32 RAY_RECURSIVE_DEPTH = 16;

RayTracingCamera::RayTracingCamera(Math::USize size):m_RayPerPixel(RAY_PER_PIXEL), m_RenderSize(size){
	uint32 numPixels = m_RenderSize.X * m_RenderSize.Y;
	m_Pixels.resize(numPixels);
	SetupRayData();
}

RayTracingCamera::~RayTracingCamera() {
}

void RayTracingCamera::SetView(const Math::FVector3& eye, const Math::FVector3& at, const Math::FVector3& up) {
	m_Eye = eye;
	m_At = at;
	m_Up = up;
	SetupRayData();
}

void RayTracingCamera::SetFov(float fov) {
	m_Fov = fov;
	SetupRayData();
}

void RayTracingCamera::SetFocus(float focusDistance, float defocusAngle) {
	m_FocusDistance = focusDistance;
	m_DefocusAngle = defocusAngle;
	SetupRayData();
}

void RayTracingCamera::Render(const RayTracingScene* scene) {
	LOG_INFO("Scene is Rendering...");
	const float sampleScale = 1.0f / (float)m_RayPerPixel;

	// parallel rendering
	size_t numPixel = (size_t)(m_RenderSize.X * m_RenderSize.Y);
	Concurrency::parallel_for(0ull, numPixel, [this, sampleScale, scene](size_t pixel) {
		uint32 renderWidth = m_RenderSize.X;
		uint32 i = (uint32)pixel % renderWidth;
		uint32 j = (uint32)pixel / renderWidth;
		FVector4 color = FVector4::ZERO;
		if (m_RayPerPixel == 1) {
			FVector3 pixelSample = m_PixelStart + (float)i * m_DeltaU + (float)j * m_DeltaV;
			const FRay ray{ m_Eye, pixelSample };
			color = ComputeRayResult(ray, scene, RAY_RECURSIVE_DEPTH);
		}
		else {
			for (uint32 sample = 0; sample < m_RayPerPixel; ++sample) {
				const FRay ray = GetRandomRay(i, j);
				color += ComputeRayResult(ray, scene, RAY_RECURSIVE_DEPTH);
			}
			color *= sampleScale;
		}
		m_Pixels[j * renderWidth + i] = Color8{ color };
	});

	//for (uint32 j = 0; j < m_Height; ++j) {
	//	for (uint32 i = 0; i < m_Width; ++i) {
	//		// multi sample
	//		FVector4 color = FVector4::ZERO;
	//		if(m_RayPerPixel == 1) {
	//			FVector3 pixelSample = m_PixelStart + (float)i * m_DeltaU + (float)j * m_DeltaV;
	//			const FRay ray{ m_Eye, pixelSample };
	//			color = ComputeRayResult(ray, scene, RAY_RECURSIVE_DEPTH);
	//		}
	//		else {
	//			for (uint32 sample = 0; sample < m_RayPerPixel; ++sample) {
	//				const FRay ray = GetRandomRay(i, j);
	//				color += ComputeRayResult(ray, scene, RAY_RECURSIVE_DEPTH);
	//			}
	//			color *= sampleScale;
	//		}
	//		m_Pixels[j * m_Width + i] = Color8{ color };
	//	}
	//}
}

RenderData RayTracingCamera::GetRenderData() const {
	return RenderData{ m_RenderSize.X, m_RenderSize.Y, m_Pixels.data()};
}

void RayTracingCamera::SetupRayData() {
	const float aspect = (float)m_RenderSize.X / (float)m_RenderSize.Y;
	const float cameraDistance = m_FocusDistance;
	const float tanHalfFov = Math::Tan(0.5f * m_Fov);
	const float viewportHeight = tanHalfFov * cameraDistance * 2.0f;
	const float viewportWidth = viewportHeight * aspect;

	// left-top to right-bottom
	FVector3 forward, right, up;
	ComputeDirections(forward, right, up);
	FVector3 viewportU = viewportWidth * right;
	FVector3 viewportV = -viewportHeight * up;
	m_DeltaU = viewportU / (float)m_RenderSize.X;
	m_DeltaV = viewportV / (float)m_RenderSize.Y;
	FVector3 viewportUpperLeft = m_Eye + forward * cameraDistance - viewportU * 0.5f - viewportV * 0.5f;
	m_PixelStart = viewportUpperLeft;

	// Calculate the camera defocus disk basis vectors.
	float defocus_radius = cameraDistance * Math::Tan(m_DefocusAngle * 0.5f);
	m_DefocusDiskU = defocus_radius * right;
	m_DefocusDiskV = defocus_radius * up;
}

void RayTracingCamera::ComputeDirections(Math::FVector3& forward, Math::FVector3& right, Math::FVector3& up) const {
	forward = (m_At - m_Eye).Normalize();
	right = m_Up.Cross(forward).Normalize();
	up = forward.Cross(right);
}

Math::FRay RayTracingCamera::GetRandomRay(uint32 i, uint32 j) {
	FVector3 offset {Random01() - 0.5f, Random01() - 0.5f, 0.0f}; // sample square
	FVector3 pixelSample = m_PixelStart + ((float)i + offset.X) * m_DeltaU + ((float)j + offset.Y) * m_DeltaV;
	FVector3 rayOrigin = m_Eye;
	if(m_DefocusAngle > 0.0f) {
		FVector2 p = RandomInDisk();
		rayOrigin += p.X * m_DefocusDiskU + p.Y * m_DefocusDiskV;
	}
	return FRay{ rayOrigin, pixelSample - rayOrigin };
}

Math::FVector4 RayTracingCamera::ComputeRayResult(const Math::FRay& ray, const RayTracingScene* scene, uint32 recursiveDepth) {
	if(0u == recursiveDepth) {
		return Math::FVector4{ 0.0f, 0.0f, 0.0f, 1.0f };
	}
	RayHitSurface hit;
	if (scene->RayHit(ray, 0.001f, RAY_MAX_DISTANCE, hit)) {
		//const FVector3 direction = RandomOnHemisphere(hit.Normal);
		if(hit.Material) {
			Math::FVector4 color;
			Math::FRay newRay;
			if (hit.Material->Scatter(ray, hit.Geometry, color, newRay)) {
				return color * ComputeRayResult(newRay, scene, recursiveDepth-1);
			}
		}
		return FVector4{ 0.0f, 0.0f, 0.0f, 1.0f };
	}
	// Return sky color
	const float alpha = Math::Clamp(ray.Direction.Y * 0.5f + 0.5f, 0.0f, 1.0f);
	static const Math::FVector3 color0{ 1.0f, 1.0f, 1.0f };
	static const Math::FVector3 color1{ 0.5f, 0.7f, 1.0f };
	const Math::FVector3 color = alpha * color1 + (1.0f - alpha) * color0;

	return Math::FVector4{ color, 1.0f };
}
