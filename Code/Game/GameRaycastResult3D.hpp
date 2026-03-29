#pragma once
#include "Engine/Math/RaycastUtils.hpp"
#include "Game/BlockIterator.hpp"

struct GameRaycastResult3D : public RaycastResult3D
{
public:
	GameRaycastResult3D() = default;

	explicit GameRaycastResult3D(bool impactResult, Vec3 const& rayStartPos, Vec3 const& rayFwdNormal, float rayLength);
	explicit GameRaycastResult3D(bool impactResult, float impactDistance, Vec3 const& impactPos, Vec3 const& impactNormal, Vec3 const& rayStartPos, Vec3 const& rayFwdNormal, float rayLength);

	BlockIterator m_blockIterator;

};