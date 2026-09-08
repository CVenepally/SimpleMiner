#include "Game/Entity.hpp"
#include "Game/Chunk.hpp"
#include "Game/Game.hpp"
#include "Game/World.hpp"
#include "Game/BlockIterator.hpp"

#include "Game/GameCommon.hpp"

#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/DebugRender.hpp"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Entity::Entity()
{
	m_position		= Vec3();
	m_rotation		= EulerAngles();
	m_maxMoveSpeed	= 0.f;
	
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Entity::Entity(Vec3 const& position, EulerAngles const& orientation, float moveSpeed, float height, float width, float eyeHeight)
	: m_position(position)
	, m_rotation(orientation)
	, m_maxMoveSpeed(moveSpeed)
	, m_height(height)
	, m_width(width)
	, m_eyeHeight(eyeHeight)
{
	float halfWidth = m_width / 2.f;
	Vec3 boxMins = Vec3(-halfWidth, - halfWidth, 0.f);
	Vec3 boxMaxs = boxMins + Vec3(width, width, height);
	m_collider = AABB3(boxMins, boxMaxs);

	AddVertsForAABB3D(m_debugVerts, m_collider, Rgba8::BLUE);

	PrecomputeCornerPoints();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Entity::~Entity()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Entity::Update()
{
	if(Game::ShowEntityDebugs())
	{
		for(int i = 0; i < 12; i++)
		{
			DebugAddWorldSphere(m_pointsOnCollider_world[i], 0.007f, 0.f, Rgba8::WHITE, Rgba8::WHITE, DebugRenderMode::ALWAYS);
		}
	}	
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Entity::PhysicsUpdate(float deltaSeconds)
{
	Vec3 horizontal = Vec3(m_currentVelocity.x, m_currentVelocity.y, 0.f);

	float dragCoefficient = m_isGrounded ? m_horizontalGroundDragCoefficient : m_horizontalAirDragCoefficient;
	Vec3 drag = -horizontal * dragCoefficient;

	AddForce(drag);

	if(m_physicsMode == PHYSICS_WALKING && m_isGrounded == false)
	{
		AddForce(Vec3::DOWN * m_gravityAcceleration);
	}

	m_currentVelocity += m_currentAcceleration * deltaSeconds;

	Vec3 horizontalVelocity = Vec3(m_currentVelocity.x, m_currentVelocity.y, 0.f);
	float speed = horizontalVelocity.GetLength();

	if(m_physicsMode == PHYSICS_WALKING)
	{
		if(speed > m_maxHorizontalSpeed)
		{
			Vec3 clamped = horizontalVelocity.GetNormalized() * m_maxHorizontalSpeed;
			m_currentVelocity.x = clamped.x;
			m_currentVelocity.y = clamped.y;
		}

	}
	else
	{
		if(speed > m_maxHorizontalSpeedFlying)
		{
			Vec3 clamped = horizontalVelocity.GetNormalized() * m_maxHorizontalSpeedFlying;
			m_currentVelocity.x = clamped.x;
			m_currentVelocity.y = clamped.y;
		}

	}


	Vec3 verticalVelocity = Vec3(0.f, 0.f, m_currentVelocity.z);
	speed = verticalVelocity.GetLength();

	if(speed > m_maxVerticalSpeed)
	{
		Vec3 clamped = verticalVelocity.GetNormalized() * m_maxVerticalSpeed;
		m_currentVelocity.z = clamped.z;
	}

	CollisionDetection(deltaSeconds);

	m_isGrounded = GroundCheck();
	

	TransformCornerPointsToWorld();

	m_currentAcceleration = Vec3::ZERO;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Entity::CollisionDetection(float deltaSeconds)
{
	Vec3 deltaPosition = m_currentVelocity * deltaSeconds;

	if(deltaPosition.GetLengthSquared() < (0.001f) * (0.001f))
	{
		return;
	}
	if(m_physicsMode != PHYSICS_NOCLIP)
	{
		for(Vec3 startPoint : m_pointsOnCollider_world)
		{
			Vec3 rayDirection = deltaPosition.GetNormalized();
			float length = deltaPosition.GetLength() + g_colliderPointNudgeFactor;
			GameRaycastResult3D result = RaycastVsWorld(startPoint, rayDirection, length);

			if(Game::ShowEntityDebugs())
			{
				DebugAddWorldArrow(startPoint, rayDirection, length, 0.01f, 0.f, Rgba8(0, 100, 0));
			}
			
			if(!result.m_didImpact)
			{
				continue;
			}

			BlockIterator& blockIter = result.m_blockIterator;
			uint32_t blockIndex = blockIter.m_blockIndex;
			Block& block = blockIter.m_chunk->GetBlock(blockIndex);

			if(block.IsBlockWater())
			{
				continue;
			}

			Vec3 normal = result.m_impactNormal;

			if(DotProduct3D(rayDirection, normal) >= 0)
			{
				continue;
			}


			if(fabsf(normal.x) > 0.0001f)
			{
				deltaPosition.x = 0.f;
				m_currentVelocity.x = 0.f;
			}

			if(fabsf(normal.y) > 0.0001f)
			{
				deltaPosition.y = 0.f;
				m_currentVelocity.y = 0.f;
			}

			if(fabsf(normal.z) > 0.0001f)
			{
				deltaPosition.z = 0.f;
				m_currentVelocity.z = 0.f;
			}

		}
	}
	
	m_position += deltaPosition;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
GameRaycastResult3D Entity::RaycastVsWorld(Vec3 const& startPosition, Vec3 const& direction, float length)
{
	GameRaycastResult3D rayResult = GameRaycastResult3D();

	rayResult.m_didImpact = false;
	rayResult.m_impactDistance = length;
	rayResult.m_impactNormal = Vec3::ZERO;
	rayResult.m_rayStartPos = startPosition;
	rayResult.m_rayForwardNormal = direction;
	rayResult.m_rayMaxLength = length;
	rayResult.m_blockIterator = BlockIterator();

	IntVec3 startBlockCoord = GetGlobalCoords(rayResult.m_rayStartPos);

	Vec3 rayFwd = rayResult.m_rayForwardNormal;

	int xStepDirection = rayFwd.x < 0.f ? -1 : 1;
	int yStepDirection = rayFwd.y < 0.f ? -1 : 1;
	int zStepDirection = rayFwd.z < 0.f ? -1 : 1;

	float fwdDistancePerX = 1.f / abs(rayFwd.x);
	float xPosAtFirstXCrossing = static_cast<float>(startBlockCoord.x + (xStepDirection + 1) / 2);
	float xDistToFirstXCrossing = xPosAtFirstXCrossing - rayResult.m_rayStartPos.x;
	float fwdDistAtNextXCrossing = fabsf(xDistToFirstXCrossing) * fwdDistancePerX;

	float fwdDistancePerY = 1.f / abs(rayFwd.y);
	float yPosAtFirstYCrossing = static_cast<float>(startBlockCoord.y + (yStepDirection + 1) / 2);
	float yDistToFirstYCrossing = yPosAtFirstYCrossing - rayResult.m_rayStartPos.y;
	float fwdDistAtNextYCrossing = fabsf(yDistToFirstYCrossing) * fwdDistancePerY;

	float fwdDistancePerZ = 1.f / abs(rayFwd.z);
	float zPosAtFirstZCrossing = static_cast<float>(startBlockCoord.z + (zStepDirection + 1) / 2);
	float zDistToFirstZCrossing = zPosAtFirstZCrossing - rayResult.m_rayStartPos.z;
	float fwdDistAtNextZCrossing = fabsf(zDistToFirstZCrossing) * fwdDistancePerZ;

	while(true)
	{
		// X
		if((fwdDistAtNextXCrossing < fwdDistAtNextYCrossing) &&
		   (fwdDistAtNextXCrossing < fwdDistAtNextZCrossing))
		{
			if(fwdDistAtNextXCrossing > rayResult.m_rayMaxLength)
			{

				rayResult.m_didImpact = false;
				rayResult.m_impactDistance = length;
				rayResult.m_impactNormal = Vec3::ZERO;
				rayResult.m_rayStartPos = startPosition;
				rayResult.m_rayForwardNormal = direction;
				rayResult.m_rayMaxLength = length;
				rayResult.m_blockIterator = BlockIterator();

				return rayResult;
			}

			startBlockCoord.x += xStepDirection;

			IntVec2 chunkCoord = GetChunkCoords(startBlockCoord);
			Chunk* chunk = Game::GetWorld()->GetChunk(chunkCoord);

			if(chunk)
			{
				int blockIndex = GlobalCoordsToIndex(startBlockCoord);
				if(blockIndex >= 0 && blockIndex < BLOCKS_PER_CHUNK && !chunk->IsBlockAir(blockIndex))
				{
					rayResult.m_blockIterator = BlockIterator(chunk, blockIndex);
					rayResult.m_didImpact = true;
					rayResult.m_impactDistance = fwdDistAtNextXCrossing;
					rayResult.m_impactPos = rayResult.m_rayStartPos + (rayResult.m_impactDistance * rayResult.m_rayForwardNormal);
					rayResult.m_impactNormal = Vec3(-xStepDirection, 0, 0);
					return rayResult;
				}
			}

			fwdDistAtNextXCrossing += fwdDistancePerX;
		}

		else if(fwdDistAtNextYCrossing < fwdDistAtNextZCrossing)
		{
			if(fwdDistAtNextYCrossing > rayResult.m_rayMaxLength)
			{
				rayResult.m_didImpact = false;
				rayResult.m_impactDistance = length;
				rayResult.m_impactNormal = Vec3::ZERO;
				rayResult.m_rayStartPos = startPosition;
				rayResult.m_rayForwardNormal = direction;
				rayResult.m_rayMaxLength = length;
				rayResult.m_blockIterator = BlockIterator();

				return rayResult;
			}

			startBlockCoord.y += yStepDirection;

			IntVec2 chunkCoord = GetChunkCoords(startBlockCoord);
			Chunk* chunk = Game::GetWorld()->GetChunk(chunkCoord);

			if(chunk)
			{
				int blockIndex = GlobalCoordsToIndex(startBlockCoord);
				if(blockIndex >= 0 && blockIndex < BLOCKS_PER_CHUNK && !chunk->IsBlockAir(blockIndex))
				{
					rayResult.m_blockIterator = BlockIterator(chunk, blockIndex);
					rayResult.m_didImpact = true;
					rayResult.m_impactDistance = fwdDistAtNextYCrossing;
					rayResult.m_impactPos = rayResult.m_rayStartPos + (rayResult.m_impactDistance * rayResult.m_rayForwardNormal);
					rayResult.m_impactNormal = Vec3(0, -yStepDirection, 0);
					return rayResult;
				}
			}

			fwdDistAtNextYCrossing += fwdDistancePerY;
		}

		else
		{
			if(fwdDistAtNextZCrossing > rayResult.m_rayMaxLength)
			{
				rayResult.m_didImpact = false;
				rayResult.m_impactDistance = length;
				rayResult.m_impactNormal = Vec3::ZERO;
				rayResult.m_rayStartPos = startPosition;
				rayResult.m_rayForwardNormal = direction;
				rayResult.m_rayMaxLength = length;
				rayResult.m_blockIterator = BlockIterator();

				return rayResult;
			}

			startBlockCoord.z += zStepDirection;

			IntVec2 chunkCoord = GetChunkCoords(startBlockCoord);
			Chunk* chunk = Game::GetWorld()->GetChunk(chunkCoord);

			if(chunk)
			{
				int blockIndex = GlobalCoordsToIndex(startBlockCoord);
				if(blockIndex >= 0 && blockIndex < BLOCKS_PER_CHUNK && !chunk->IsBlockAir(blockIndex))
				{
					rayResult.m_blockIterator = BlockIterator(chunk, blockIndex);
					rayResult.m_didImpact = true;
					rayResult.m_impactDistance = fwdDistAtNextZCrossing;
					rayResult.m_impactPos = rayResult.m_rayStartPos + (rayResult.m_impactDistance * rayResult.m_rayForwardNormal);
					rayResult.m_impactNormal = Vec3(0, 0, -zStepDirection);
					return rayResult;
				}
			}

			fwdDistAtNextZCrossing += fwdDistancePerZ;
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Entity::GroundCheck()
{
	if(m_physicsMode != PHYSICS_WALKING)
	{
		return false;
	}
	
	Vec3& bottomRightBack = m_pointsOnCollider_world[CP_BACK_RIGHT_BOTTOM];
	Vec3& bottomLeftBack = m_pointsOnCollider_world[CP_BACK_LEFT_BOTTOM];
	Vec3& bottomRightFront = m_pointsOnCollider_world[CP_FORWARD_RIGHT_BOTTOM];
	Vec3& bottomLeftFront = m_pointsOnCollider_world[CP_FORWARD_LEFT_BOTTOM];

	Vec3 bottomPoints[4] = {bottomRightBack, bottomRightFront, bottomLeftBack, bottomLeftFront};

	for(int point = 0; point < 4; ++point)
	{
		GameRaycastResult3D rayResult = RaycastVsWorld(bottomPoints[point], Vec3::DOWN, 2 * g_colliderPointNudgeFactor);

		BlockIterator& blockIter = rayResult.m_blockIterator;
		uint32_t blockIndex = blockIter.m_blockIndex;
		Block& block = blockIter.m_chunk->GetBlock(blockIndex);

		if(!blockIter.m_chunk || block.IsBlockWater())
		{
			continue;
		}

		if(Game::ShowEntityDebugs())
		{
			DebugAddWorldLine(bottomPoints[point], Vec3::DOWN, 2 * g_colliderPointNudgeFactor, 0.01f, 0.f, Rgba8::CYAN);
		}

		if(rayResult.m_didImpact == true)
		{
			return true;
		}
	}

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Entity::PrecomputeCornerPoints()
{
	std::vector<Vec3> eightCornerPoints;
	m_collider.GetCornerPoints(eightCornerPoints);

	for(size_t point = 0; point < eightCornerPoints.size(); ++point)
	{
		m_pointsOnCollider_local[point] = eightCornerPoints[point];
	}

	Vec3 const& backRightBottom = eightCornerPoints[AABB3_BACK_RIGHT_BOTTOM];
	Vec3 const& backRightTop = eightCornerPoints[AABB3_BACK_RIGHT_TOP];

	Vec3 const& backLeftBottom = eightCornerPoints[AABB3_BACK_LEFT_BOTTOM];
	Vec3 const& backLeftTop = eightCornerPoints[AABB3_BACK_LEFT_TOP];

	Vec3 const& forwardLeftBottom = eightCornerPoints[AABB3_FORWARD_LEFT_BOTTOM];
	Vec3 const& forwardLeftTop = eightCornerPoints[AABB3_FORWARD_LEFT_TOP];

	Vec3 const& forwardRightBottom = eightCornerPoints[AABB3_FORWARD_RIGHT_BOTTOM];
	Vec3 const& forwardRightTop = eightCornerPoints[AABB3_FORWARD_RIGHT_TOP];

	Vec3 midBackRight = 0.5f * (backRightBottom + backRightTop);
	Vec3 midBackLeft = 0.5f * (backLeftBottom + backLeftTop);
	Vec3 midForwardLeft = 0.5f * (forwardLeftBottom + forwardLeftTop);
	Vec3 midForwardRight = 0.5f * (forwardRightBottom + forwardRightTop);

	m_pointsOnCollider_local[CP_MID_BACK_RIGHT] = midBackRight;
	m_pointsOnCollider_local[CP_MID_BACK_LEFT] = midBackLeft;
	m_pointsOnCollider_local[CP_MID_FORWARD_LEFT] = midForwardLeft;
	m_pointsOnCollider_local[CP_MID_FORWARD_RIGHT] = midForwardRight;

	Vec3 center = m_collider.GetCenter();

	for(int i = 0; i < 12; i++)
	{
		Vec3& p = m_pointsOnCollider_local[i];

		Vec3 dir = p - center;

		dir.Normalize();

		p = p - dir * g_colliderPointNudgeFactor;
	}

	TransformCornerPointsToWorld();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Entity::TransformCornerPointsToWorld()
{
	Mat44 transform = GetPositionOnlyTransfrom();
	for(int i = 0; i < 12; i++)
	{
		m_pointsOnCollider_world[i] = transform.TransformPosition3D(m_pointsOnCollider_local[i]);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 Entity::GetForward() const
{
	return m_rotation.GetAsMatrix_IFwd_JLeft_KUp().GetIBasis3D();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 Entity::GetLeft() const
{
	return m_rotation.GetAsMatrix_IFwd_JLeft_KUp().GetJBasis3D();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 Entity::GetUp() const
{
	return m_rotation.GetAsMatrix_IFwd_JLeft_KUp().GetKBasis3D();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Mat44 Entity::GetPositionOnlyTransfrom() const
{
//	Mat44 transform = m_rotation.GetAsMatrix_IFwd_JLeft_KUp();
	Mat44 transform = Mat44();
	transform.SetTranslation3D(m_position);
	return transform;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Mat44 Entity::GetTransfrom() const
{
	Mat44 transform = m_rotation.GetAsMatrix_IFwd_JLeft_KUp();
	transform.SetTranslation3D(m_position);
	return transform;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Entity::MoveInDirection(Vec3 const& moveDirection, bool didJump)
{
	float acceleration = m_isGrounded ? m_horizontalGroundAcceleration : m_horizontalAirAcceleration;
	
	AddForce(moveDirection * acceleration);

	if(didJump && m_isGrounded)
	{
		AddImpulse(Vec3::UP * m_jumpImpulse);
	}
}


