#pragma once
#include "Math/Vector.h"
#include "Math/Geometry.h"

class RayTracingCamera {
public:
	RayTracingCamera(Math::USize InRenderSize);
	~RayTracingCamera();
	void SetView(const Math::FVector3& eye, const Math::FVector3& at, const Math::FVector3& up);
	void SetFov(float fov);
	void SetFocusDistance(float InDistance);
	void SetDefocusAngle(float InAngle);
	Math::USize GetRenderSize() const;
	Math::FRayWithTime GetRandomRayWithTime(uint32 i, uint32 j) const;
	Math::FRayWithTime GetRandomRayWithTimeOffset(uint32 i, uint32 j, const Math::FVector3& Offset) const;
	void SetupRayData();
private:
	// view
	Math::FVector3 Eye{0.0f, 0.0f, 0.0f};
	Math::FVector3 At{0.0f, 0.0f, -1.0f};
	Math::FVector3 Up{0.0f, 1.0f, 0.0f};
	float Fov{1.0f}; // radian

	// defocus
	float FocusDistance {3.4f};
	float DefocusAngle {0.174f};// radian

	// render
	Math::USize RenderSize;

	// ray data
	Math::FVector3 PixelStart;
	Math::FVector3 DeltaU;
	Math::FVector3 DeltaV;
	Math::FVector3 DefocusDiskU;
	Math::FVector3 DefocusDiskV;
	void ComputeDirections(Math::FVector3& forward, Math::FVector3& right, Math::FVector3& up) const;
};