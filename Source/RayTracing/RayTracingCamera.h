#pragma once
#include "RayTracingScene.h"

struct RenderData {
	uint32 Width;
	uint32 Height;
	const Math::Color8* Data;
};

class RayTracingCamera {
public:
	RayTracingCamera(Math::USize size);
	~RayTracingCamera();
	void SetView(const Math::FVector3& eye, const Math::FVector3& at, const Math::FVector3& up);
	void SetFov(float fov);
	void SetFocus(float focusDistance, float defocusAngle);
	void Render(const RayTracingScene* scene);
	RenderData GetRenderData() const;
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
	uint32 m_RayPerPixel;
	Math::USize m_RenderSize;
	std::vector<Math::Color8> m_Pixels;

	// ray data
	Math::FVector3 m_PixelStart;
	Math::FVector3 m_DeltaU;
	Math::FVector3 m_DeltaV;
	Math::FVector3 m_DefocusDiskU;
	Math::FVector3 m_DefocusDiskV;

	void SetupRayData();
	void ComputeDirections(Math::FVector3& forward, Math::FVector3& right, Math::FVector3& up) const;
	Math::FRay GetRandomRay(uint32 i, uint32 j);
	Math::FVector4 ComputeRayResult(const Math::FRay& ray, const RayTracingScene* scene, uint32 recursiveDepth);
};