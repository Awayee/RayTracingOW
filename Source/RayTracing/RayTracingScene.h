#pragma once
#include <vector>
#include "RayTracing/RayTracingObject.h"
#include "Core/TUniquePtr.h"

typedef std::vector<TUniquePtr<RayTracingObjectBase>> ObjectArray;

class RTVirtualNode: public RayTracingObjectBase {
public:
	RTVirtualNode(const std::vector<RTVirtualNode>& InTree, uint32 InLeft, uint32 InRight);
	RTVirtualNode(const RTVirtualNode&) = delete;
	RTVirtualNode& operator=(const RTVirtualNode&)=delete;	
	RTVirtualNode(RTVirtualNode&&) noexcept = default;
	RTVirtualNode& operator=(RTVirtualNode&&)noexcept = default;

	virtual bool TestRay(const Math::FRay& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const override;
	virtual Math::FAABB3 GetAABB() const override;
private:
	const std::vector<RTVirtualNode>& Tree;
	uint32 ChildLeft;
	uint32 ChildRight;
	Math::FAABB3 AABB;
};

class RayTracingScene {
public:
	RayTracingScene();
	~RayTracingScene() = default;
	void SetBackground(TexturePtr&& Texture);
	void AddSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial);
	void AddMovableSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial, const Math::FVector3& MoveTarget);
	void AddObject(TUniquePtr<RayTracingObjectBase>&& InObject);
	void BuildHierarchy();
	bool TestRay(const Math::FRay& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHit) const;
	bool TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHit) const;
	Math::FVector4 RayFallback(const Math::FRay& InRay);
private:
	struct BVHNode {
		uint32 LeftNode;
		uint32 RightNode;
		uint32 LeftObject;
		uint32 RightObject;
		Math::FAABB3 AABB;
		BVHNode();
		BVHNode(const BVHNode&)=default;
		BVHNode& operator=(const BVHNode&)=default;
		BVHNode(BVHNode&&)noexcept = default;
		BVHNode& operator=(BVHNode&&)noexcept=default;
	};

	TexturePtr Background;
	ObjectArray Objects;
	std::vector<BVHNode> Nodes;

	uint32 RecursivelyBuildNode(uint32 ObjectStart, uint32 ObjectEnd, uint32 Depth);

	bool RecursivelyTestRayWithTime(uint32 NodeIdx, const Math::FRayWithTime& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const;
};