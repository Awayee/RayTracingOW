#include "RayTracingScene.h"

#include <algorithm>

#include "Core/Log.h"

template<int Axis>
bool CompareAABBByAxis(const TUniquePtr<RayTracingHittable>& L, const TUniquePtr<RayTracingHittable>& R) {
	return L->GetAABB().Min[Axis] < R->GetAABB().Max[Axis];
}

RTVirtualNode::RTVirtualNode(const std::vector<RTVirtualNode>& InTree, uint32 InLeft, uint32 InRight): Tree(InTree), ChildLeft(InLeft), ChildRight(InRight) {
	if(InLeft!=INVALID_INDEX_U32) {
		AABB.Union(InTree[InLeft].GetAABB());
	}
	if(InRight != INVALID_INDEX_U32) {
		AABB.Union(InTree[InRight].GetAABB());
	}
}

bool RTVirtualNode::TestRay(const Math::FRayWithTime& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	if(AABB.TestRay(Ray, DistanceMin, DistanceMax)) {
		bool bLeft = ChildLeft!=INVALID_INDEX_U32 && Tree[ChildLeft].TestRay(Ray, DistanceMin, DistanceMax, OutHitSurface);
		bool bRight = ChildRight!= INVALID_INDEX_U32 && Tree[ChildRight].TestRay(Ray, DistanceMin, DistanceMax, OutHitSurface);
		return bLeft || bRight;
	}
	return false;
}

Math::FAABB3 RTVirtualNode::GetAABB() const {
	return AABB;
}

RayTracingScene::RayTracingScene() {
	Background.Reset(new SolidColor(Math::Color8{0.7f, 0.8f, 1.0f, 1.0f}));
}

void RayTracingScene::SetBackground(TexturePtr&& Texture) {
	Background = MoveTemp(Texture);
}

void RayTracingScene::AddSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial) {
	AddObject(RTObjectPtr(new RTSphere(InSphere, MoveTemp(InMaterial))));
}

void RayTracingScene::AddMovableSphere(const Math::FSphere& InSphere, MaterialPtr&& InMaterial, const Math::FVector3& MoveTarget) {
	AddObject(RTObjectPtr(new RTMovableSphere(InSphere, MoveTemp(InMaterial), MoveTarget)));
}

void RayTracingScene::AddObject(RTObjectPtr&& InObject) {
	Objects.push_back(MoveTemp(InObject));
}

void RayTracingScene::AddLight(TUniquePtr<RayTracingHittable>&& InLight) {
	Lights.push_back(MoveTemp(InLight));
}

void RayTracingScene::BuildHierarchy() {
	RecursivelyBuildNode(0, (uint32)Objects.size(), 0);
}

bool RayTracingScene::TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHit) const {
	//bool hitAnything = false;
	//float closestDistance = DistanceMax;
	//for (const TUniquePtr<RayTracingHittable>& obj : Objects) {
	//	if (obj->TestRay(InRay, DistanceMin, closestDistance, OutHit)) {
	//		closestDistance = OutHit.Geometry.Distance;
	//		hitAnything = true;
	//	}
	//}
	//return hitAnything;
	return RecursivelyTestRayWithTime(0, InRay, DistanceMin, DistanceMax, OutHit);
}

const ObjectArray& RayTracingScene::GetLights() const {
	return Lights;
}

Math::FVector4 RayTracingScene::RayFallback(const Math::FRay& InRay) {
	//// Return sky color
	//const float Alpha = InRay.Direction.Normalize().Y * 0.5f + 0.5f;
	//static const Math::FVector3 Color0{ 1.0f, 1.0f, 1.0f };
	//static const Math::FVector3 Color1{ 0.5f, 0.7f, 1.0f };
	//const Math::FVector3 Color = Alpha * Color1 + (1.0f - Alpha) * Color0;
	//return Math::FVector4{ Color, 1.0f };
	const Math::FVector3 Dir = InRay.Direction.Normalize();
	const Math::FVector2 UV {Dir.X * 0.5f + 0.5f, Dir.Y * 0.5f + 0.5f};
	return Background->SampleVector4(UV, InRay.Origin);
}

RayTracingScene::BVHNode::BVHNode() :
LeftNode(INVALID_INDEX_U32),
RightNode(INVALID_INDEX_U32),
LeftObject(INVALID_INDEX_U32),
RightObject(INVALID_INDEX_U32){}

uint32 RayTracingScene::RecursivelyBuildNode(uint32 ObjectStart, uint32 ObjectEnd, uint32 Depth) {
	const uint32 NumObjects = ObjectEnd - ObjectStart;
	if(0 == NumObjects) {
		return INVALID_INDEX_U32;
	}
	const uint32 Idx = (uint32)Nodes.size();
	BVHNode& Node = Nodes.emplace_back();

	if(1 == NumObjects) {
		Node.LeftObject = Node.RightObject = ObjectStart;
		Node.AABB = Objects[Node.LeftObject]->GetAABB();
	}
	else if(2 == NumObjects) {
		Node.LeftObject = ObjectStart;
		Node.RightObject = ObjectStart + 1;
		Node.AABB.Union(Objects[Node.LeftObject]->GetAABB());
		Node.AABB.Union(Objects[Node.RightObject]->GetAABB());
	}
	else {
		++Depth;
		for(uint32 i=ObjectStart; i<ObjectEnd; ++i) {
			Node.AABB.Union(Objects[i]->GetAABB());
		}
		int Axis = Node.AABB.GetMaxAxis();
		std::sort(Objects.begin()+ObjectStart, Objects.begin()+ObjectEnd, [Axis](const TUniquePtr<RayTracingHittable>& L, const TUniquePtr<RayTracingHittable>& R) {
			return L->GetAABB().Min[Axis] < R->GetAABB().Min[Axis];
		});
		const uint32 Mid = (ObjectStart + ObjectEnd) / 2;
		// memory modified
		Nodes[Idx].LeftNode = RecursivelyBuildNode(ObjectStart, Mid, Depth);
		Nodes[Idx].RightNode = RecursivelyBuildNode(Mid, ObjectEnd, Depth);
	}
	return Idx;
}

bool RayTracingScene::RecursivelyTestRayWithTime(uint32 NodeIdx, const Math::FRayWithTime& Ray, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	if(!(NodeIdx < Nodes.size())) {
		return false;
	}
	const BVHNode& Node = Nodes[NodeIdx];
	if(!Node.AABB.TestRay(Ray, DistanceMin, DistanceMax)) {
		return false;
	}

	bool bHit = false;
	if(Node.LeftObject != INVALID_INDEX_U32) {
		bHit |= Objects[Node.LeftObject]->TestRay(Ray, DistanceMin, DistanceMax, OutHitSurface);
	}
	if(Node.RightObject != INVALID_INDEX_U32 && Node.RightObject != Node.LeftObject) {
		DistanceMax = bHit ? OutHitSurface.Geometry.Distance : DistanceMax;
		bHit |= Objects[Node.RightObject]->TestRay(Ray, DistanceMin, DistanceMax, OutHitSurface);
	}
	if(Node.LeftNode != INVALID_INDEX_U32) {
		bHit |= RecursivelyTestRayWithTime(Node.LeftNode, Ray, DistanceMin, DistanceMax, OutHitSurface);
	}
	if(Node.RightNode != INVALID_INDEX_U32 && Node.RightNode != Node.LeftNode) {
		DistanceMax = bHit ? OutHitSurface.Geometry.Distance : DistanceMax;
		bHit |= RecursivelyTestRayWithTime(Node.RightNode, Ray, DistanceMin, DistanceMax, OutHitSurface);
	}
	return bHit;
}
