#include "RayTracingCamera.h"
#include "Math/MathUtil.h"
#include "Core/Log.h"
#include "Core/ParallelFor.h"
#include <random>

#define PARALLEL_RENDERING 1

using namespace Math;

RayTracingCamera::RayTracingCamera(Math::USize InRenderSize):
Eye(13.0f, 2.0f,3.0f),
At(0.0f, 0.0f, 0.0f),
Up(0.0f, 1.0f, 0.0f),
Fov(20 * Math::Deg2Rad),
FocusDistance(10.0f),
DefocusAngle(0.6f * Math::Deg2Rad),
RenderSize(InRenderSize)
{
}

RayTracingCamera::~RayTracingCamera() {};

void RayTracingCamera::SetView(const Math::FVector3& eye, const Math::FVector3& at, const Math::FVector3& up) {
	Eye = eye;
	At = at;
	Up = up;
}

void RayTracingCamera::SetFov(float fov) {
	Fov = fov;
}

void RayTracingCamera::SetFocus(float focusDistance, float defocusAngle) {
	FocusDistance = focusDistance;
	DefocusAngle = defocusAngle;
}

Math::USize RayTracingCamera::GetRenderSize() const {
	return RenderSize;
}

Math::FRay RayTracingCamera::GetRandomRay(uint32 i, uint32 j) const {
	FVector3 offset{ Random01() - 0.5f, Random01() - 0.5f, 0.0f }; // sample square
	FVector3 pixelSample = PixelStart + ((float)i + offset.X) * DeltaU + ((float)j + offset.Y) * DeltaV;
	FVector3 rayOrigin = Eye;
	if (DefocusAngle > 0.0f) {
		FVector2 p = RandomInDisk();
		rayOrigin += p.X * DefocusDiskU + p.Y * DefocusDiskV;
	}
	return FRay{ rayOrigin, pixelSample - rayOrigin };
}

Math::FRayWithTime RayTracingCamera::GetRandomRayWithTime(uint32 i, uint32 j) const {
	FVector3 offset{ Random01() - 0.5f, Random01() - 0.5f, 0.0f }; // sample square
	FVector3 pixelSample = PixelStart + ((float)i + offset.X) * DeltaU + ((float)j + offset.Y) * DeltaV;
	FVector3 rayOrigin = Eye;
	if (DefocusAngle > 0.0f) {
		FVector2 p = RandomInDisk();
		rayOrigin += p.X * DefocusDiskU + p.Y * DefocusDiskV;
	}
	const float Time = Random01();
	return FRayWithTime{ rayOrigin, pixelSample - rayOrigin, Time};
}

void RayTracingCamera::SetupRayData() {
	const float aspect = (float)RenderSize.X / (float)RenderSize.Y;
	const float cameraDistance = FocusDistance;
	const float tanHalfFov = Math::Tan(0.5f * Fov);
	const float viewportHeight = tanHalfFov * cameraDistance * 2.0f;
	const float viewportWidth = viewportHeight * aspect;

	// left-top to right-bottom
	FVector3 forward, right, up;
	ComputeDirections(forward, right, up);
	FVector3 viewportU = viewportWidth * right;
	FVector3 viewportV = -viewportHeight * up;
	DeltaU = viewportU / (float)RenderSize.X;
	DeltaV = viewportV / (float)RenderSize.Y;
	FVector3 viewportUpperLeft = Eye + forward * cameraDistance - viewportU * 0.5f - viewportV * 0.5f;
	PixelStart = viewportUpperLeft;

	// Calculate the camera defocus disk basis vectors.
	float defocus_radius = cameraDistance * Math::Tan(DefocusAngle * 0.5f);
	DefocusDiskU = defocus_radius * right;
	DefocusDiskV = defocus_radius * up;
}

void RayTracingCamera::ComputeDirections(Math::FVector3& forward, Math::FVector3& right, Math::FVector3& up) const {
	forward = (At - Eye).Normalize();
	right = Up.Cross(forward).Normalize();
	up = forward.Cross(right);
}
