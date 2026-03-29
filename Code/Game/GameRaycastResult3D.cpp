#include "Game/GameRaycastResult3D.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Game.hpp"
#include "Game/World.hpp"
#include "Game/Chunk.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
GameRaycastResult3D::GameRaycastResult3D(bool impactResult, Vec3 const& rayStartPos, Vec3 const& rayFwdNormal, float rayLength)
	: RaycastResult3D(impactResult, rayStartPos, rayFwdNormal, rayLength)
	, m_blockIterator(BlockIterator())
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
GameRaycastResult3D::GameRaycastResult3D(bool impactResult, float impactDistance, Vec3 const& impactPos, 
										Vec3 const& impactNormal, Vec3 const& rayStartPos, Vec3 const& rayFwdNormal, float rayLength)
	: RaycastResult3D(impactResult, impactDistance, impactPos, impactNormal, rayStartPos, rayFwdNormal, rayLength)
{
	IntVec3 hitPos		= IntVec3(impactPos);
	IntVec2 chunkCoord	= GetChunkCoords(hitPos);
	Chunk* chunk		= Game::GetWorld()->GetChunk(chunkCoord);

	IntVec3 blockLocalCoord = GlobalCoordsToLocalCoords(hitPos);
	uint32_t blockIndex		= LocalCoordsToIndex(blockLocalCoord);

	m_blockIterator = BlockIterator(chunk, blockIndex);
}
