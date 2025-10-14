#pragma once
#include "Math/Vector.h"
#include "Math/Geometry.h"

class RayTracingCamera {
public:
	RayTracingCamera(Math::USize InRenderSize);
	~RayTracingCamera();
	void SetView(const Math::FVector3& eye, const Math::FVector3& at, const Math::FVector3& up);
	void SetFov(float fov);
	void SetFocus(float focusDistance, float defocusAngle);
	Math::USize GetRenderSize() const;
	Math::FRay GetRandomRay(uint32 i, uint32 j) const;
	Math::FRayWithTime GetRandomRayWithTime(uint32 i, uint32 j) const;
	void SetupRayData();
private:
	// view
	Math::FVector3 m_Eye{0.0f, 0.0f, 0.0f};
	Math::FVector3 m_At{0.0f, 0.0f, -1.0f};
	Math::FVector3 m_Up{0.0f, 1.0f, 0.0f};
	float m_Fov{1.0f}; // radian

	// defocus
	float m_DefocusAngle {0.174f};// radian
	float m_FocusDistance {3.4f};

	// render
	Math::USize RenderSize;

	// ray data
	Math::FVector3 m_PixelStart;
	Math::FVector3 m_DeltaU;
	Math::FVector3 m_DeltaV;
	Math::FVector3 m_DefocusDiskU;
	Math::FVector3 m_DefocusDiskV;
	void ComputeDirections(Math::FVector3& forward, Math::FVector3& right, Math::FVector3& up) const;
};