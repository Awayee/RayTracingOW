#include "RayTracingCamera.h"
#include "Math/MathUtil.h"
#include "Core/Log.h"
#include "Core/ParallelFor.h"
#include <random>

#define PARALLEL_RENDERING 1

using namespace Math;

RayTracingCamera::RayTracingCamera(Math::USize InRenderSize): RenderSize(InRenderSize){
	SetupRayData();
}

RayTracingCamera::~RayTracingCamera() {};

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

Math::USize RayTracingCamera::GetRenderSize() const {
	return RenderSize;
}

Math::FRay RayTracingCamera::GetRandomRay(uint32 i, uint32 j) const {
	FVector3 offset{ Random01() - 0.5f, Random01() - 0.5f, 0.0f }; // sample square
	FVector3 pixelSample = m_PixelStart + ((float)i + offset.X) * m_DeltaU + ((float)j + offset.Y) * m_DeltaV;
	FVector3 rayOrigin = m_Eye;
	if (m_DefocusAngle > 0.0f) {
		FVector2 p = RandomInDisk();
		rayOrigin += p.X * m_DefocusDiskU + p.Y * m_DefocusDiskV;
	}
	return FRay{ rayOrigin, pixelSample - rayOrigin };
}

Math::FRayWithTime RayTracingCamera::GetRandomRayWithTime(uint32 i, uint32 j) const {
	FVector3 offset{ Random01() - 0.5f, Random01() - 0.5f, 0.0f }; // sample square
	FVector3 pixelSample = m_PixelStart + ((float)i + offset.X) * m_DeltaU + ((float)j + offset.Y) * m_DeltaV;
	FVector3 rayOrigin = m_Eye;
	if (m_DefocusAngle > 0.0f) {
		FVector2 p = RandomInDisk();
		rayOrigin += p.X * m_DefocusDiskU + p.Y * m_DefocusDiskV;
	}
	const float Time = Random01();
	return FRayWithTime{ rayOrigin, pixelSample - rayOrigin, Time};
}

void RayTracingCamera::SetupRayData() {
	const float aspect = (float)RenderSize.X / (float)RenderSize.Y;
	const float cameraDistance = m_FocusDistance;
	const float tanHalfFov = Math::Tan(0.5f * m_Fov);
	const float viewportHeight = tanHalfFov * cameraDistance * 2.0f;
	const float viewportWidth = viewportHeight * aspect;

	// left-top to right-bottom
	FVector3 forward, right, up;
	ComputeDirections(forward, right, up);
	FVector3 viewportU = viewportWidth * right;
	FVector3 viewportV = -viewportHeight * up;
	m_DeltaU = viewportU / (float)RenderSize.X;
	m_DeltaV = viewportV / (float)RenderSize.Y;
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
