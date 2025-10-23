#include "RayTracing/RayTracingTransform.h"
#include "Math/Matrix.h"

RTTranslated::RTTranslated(RTObjectPtr&& InObject, const Math::FVector3& InTranslation) : Object(MoveTemp(InObject)), Translation(InTranslation) {
	AABB = Object->GetAABB().Translate(InTranslation);
}

bool RTTranslated::TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	const Math::FRayWithTime TempRay(InRay.Origin-Translation, InRay.Direction, InRay.Time);
	if(!Object->TestRayWithTime(TempRay, DistanceMin, DistanceMax, OutHitSurface)) {
		return false;
	}

	OutHitSurface.Geometry.Position += Translation;
	return true;
}

Math::FAABB3 RTTranslated::GetAABB() const {
	return AABB;
}

RTRotatedY::RTRotatedY(RTObjectPtr&& InObject, float Radian) : Object(MoveTemp(InObject)), RotationY(Radian){
	RotationMatrix = Math::FMatrix3x3::MakeRotateY(RotationY);
	InvRotationMatrix = Math::FMatrix3x3::MakeRotateY(-RotationY);
	AABB = Object->GetAABB().Rotate(RotationMatrix);
}

bool RTRotatedY::TestRayWithTime(const Math::FRayWithTime& InRay, float DistanceMin, float DistanceMax, RayHitSurface& OutHitSurface) const {
	const Math::FVector3 TempRayOrigin = InvRotationMatrix * InRay.Origin;
	const Math::FVector3 TempRayDirection = InvRotationMatrix * InRay.Direction;
	const Math::FRayWithTime TempRay{TempRayOrigin, TempRayDirection, InRay.Time};
	if(!Object->TestRayWithTime(TempRay, DistanceMin, DistanceMax, OutHitSurface)) {
		return false;
	}

	OutHitSurface.Geometry.Position = RotationMatrix * OutHitSurface.Geometry.Position;
	OutHitSurface.Geometry.Normal = RotationMatrix * OutHitSurface.Geometry.Normal;
	return true;
}

Math::FAABB3 RTRotatedY::GetAABB() const {
	return AABB;
}
