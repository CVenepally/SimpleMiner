#include "Game/GameCamera.hpp"
#include "Game/Game.hpp" 
#include "Game/World.hpp" 
#include "Game/Chunk.hpp" 
#include "Game/Player.hpp" 
#include "Game/GameCommon.hpp"
#include "Game/BlockDefinition.hpp"
#include "Engine/Core/Vertex_PCU.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DebugRender.hpp"
#include "Entity.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
GameCamera::GameCamera()
{
	m_camera.m_viewportBounds.m_mins = Vec2::ZERO;
	m_camera.m_viewportBounds.m_maxs = Vec2(static_cast<float>(g_theWindow->GetClientDimensions().x), static_cast<float>(g_theWindow->GetClientDimensions().y));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
GameCamera::~GameCamera()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GameCamera::Update()
{
	float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();
	KeyboardControls(deltaSeconds);
	DebugRenders();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GameCamera::DebugRenders()
{
	std::string cameraMode = CameraModeDebug();

	AABB2 textBox = AABB2(IntVec2(0, 0), g_theWindow->GetClientDimensions());
	float fontSize = 12.f;

	DebugAddScreenText(cameraMode, textBox, fontSize, Vec2(1.f, 0.95f), 0.f, Rgba8::CYAN);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string GameCamera::CameraModeDebug()
{
	switch(m_cameraMode)
	{
		case CAMERA_THIRD_PERSON:	return "[C] CameraMode: Third Person";
		case CAMERA_FIRST_PERSON:	return "[C] CameraMode: First Person";
		case CAMERA_SPECTATOR:		return "[C] CameraMode: Spectator";
		case CAMERA_SPECTATOR_XY:	return "[C] CameraMode: SpectatorXY";
		case CAMERA_INDEPENDANT:	return "[C] CameraMode: Independent";
		default : return "ERROR";
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GameCamera::Render()
{
	if(!m_followingEntity)
	{
		return;
	}

	if(m_cameraMode != CameraMode::CAMERA_FIRST_PERSON && m_followingEntity)
	{
		m_followingEntity->Render();
	}

	Player* player = dynamic_cast<Player*>(m_followingEntity);
	
	if(player && player->m_rayResult.m_didImpact)
	{
		std::vector<Vertex_PCU> verts;
		AddVertsForHitSurface(verts);
		g_theRenderer->BeginRenderEvent("Wireframe Quad Render");
		g_theRenderer->SetModelConstants();
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawWireframeVertexArray(verts);
		g_theRenderer->EndRenderEvent("Wireframe Quad Render");
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Vec3 GameCamera::GetForwardVector() const
{
	Mat44 transform = m_camera.GetOrientation().GetAsMatrix_IFwd_JLeft_KUp();
	return transform.GetIBasis3D();
}

//------------------------------------------------------------------------------------------------------------------
Vec3 GameCamera::GetPosition() const
{
	return m_camera.m_position;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GameCamera::KeyboardControls(float deltaSeconds)
{
	MovementControls(deltaSeconds);

	if(g_inputSystem->WasKeyJustPressed('C'))
	{
		SwitchCameraModes();
	}

	if(g_inputSystem->WasKeyJustPressed('V'))
	{
		m_followingEntity->SwitchPhysicsMode();
	}

	Player* player = dynamic_cast<Player*>(m_followingEntity);

	if(!player)
	{
		return;
	}
// 	if(g_inputSystem->IsKeyDown(KEYCODE_LEFT_MOUSE) && g_enableDiggingAndPlacing)
// 	{
// 		player->Dig();
// 	}
	if(g_inputSystem->WasKeyJustPressed(KEYCODE_LEFT_MOUSE))
	{
		if(player->m_isInventoryMenuOpen)
		{
			player->HandleInventoryLeftClick();
		}
		else
		{
			player->Dig();
		}
	}

	if(g_inputSystem->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE))
	{
		if(player->m_isInventoryMenuOpen)
		{
			player->HandleInventoryRightClick();
		}
		else
		{
			player->PlaceBlock();
		}
	}

	if(g_inputSystem->GetWheelDelta() > 0.f)
	{
		player->PrevHotbarSlot();
	
	}

	if(g_inputSystem->GetWheelDelta() < 0.f)
	{
		player->NextHotbarSlot();
	}

	if(g_inputSystem->WasKeyJustPressed('1')) { player->SetHotbarSlot(0); }
	if(g_inputSystem->WasKeyJustPressed('2')) { player->SetHotbarSlot(1); }
	if(g_inputSystem->WasKeyJustPressed('3')) { player->SetHotbarSlot(2); }
	if(g_inputSystem->WasKeyJustPressed('4')) { player->SetHotbarSlot(3); }
	if(g_inputSystem->WasKeyJustPressed('5')) { player->SetHotbarSlot(4); }
	if(g_inputSystem->WasKeyJustPressed('6')) { player->SetHotbarSlot(5); }
	if(g_inputSystem->WasKeyJustPressed('7')) { player->SetHotbarSlot(6); }
	if(g_inputSystem->WasKeyJustPressed('8')) { player->SetHotbarSlot(7); }
	if(g_inputSystem->WasKeyJustPressed('9')) { player->SetHotbarSlot(8); }


	if(g_inputSystem->WasKeyJustPressed(KEYCODE_TAB))
	{
		player->m_isInventoryMenuOpen = !player->m_isInventoryMenuOpen;
	}

	if(player->m_isInventoryMenuOpen)
	{
		g_inputSystem->SetCursorMode(CursorMode::POINTER);
	//	player->HitTestInvetoryUI(g_theWindow->GetNormalizedMouseUV());
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GameCamera::MovementControls(float deltaSeconds)
{
	float mouseSensitivity = 0.075f;
	
	float moveSpeed = g_gameConfigBlackboard.GetValue("playerMoveSpeed", 2.f);

	if(g_inputSystem->IsKeyDown(KEYCODE_LSHIFT))
	{
		moveSpeed *= 100.f;
	}

	if(m_cameraMode != CAMERA_SPECTATOR && m_cameraMode != CAMERA_SPECTATOR_XY)
	{
		EulerAngles entityRotation = m_followingEntity->GetRotation();
		entityRotation.m_yawDegrees -= g_inputSystem->GetCursorClientDelta().x * mouseSensitivity;
		entityRotation.m_pitchDegrees += g_inputSystem->GetCursorClientDelta().y * mouseSensitivity;

		entityRotation.m_pitchDegrees = GetClamped(entityRotation.m_pitchDegrees, -80.f, 85.f);

		m_followingEntity->SetRotation(entityRotation);

		Vec3 fwd_2DPlane	= m_followingEntity->GetForward().GetXY().GetNormalized();
		Vec3 left_2DPlane	= m_followingEntity->GetLeft().GetXY().GetNormalized();
		bool wasInputPressed = false;
		Vec3 intendedDirection = Vec3::ZERO;

		if(g_inputSystem->IsKeyDown('W'))
		{
			intendedDirection += fwd_2DPlane;
			wasInputPressed = true;
		}

		if(g_inputSystem->IsKeyDown('S'))
		{
			intendedDirection -= fwd_2DPlane;
			wasInputPressed = true;
		}

		if(g_inputSystem->IsKeyDown('A'))
		{
			intendedDirection += left_2DPlane;
			wasInputPressed = true;
		}

		if(g_inputSystem->IsKeyDown('D'))
		{
			intendedDirection -= left_2DPlane;
			wasInputPressed = true;
		}

		bool didJump = false;
		if(g_inputSystem->WasKeyJustPressed(' '))
		{
			didJump = true;   
		}

		if(m_followingEntity->GetPhysicsMode() != PHYSICS_WALKING)
		{
			if(g_inputSystem->IsKeyDown('E'))
			{
				intendedDirection += Vec3::UP;
				wasInputPressed = true;
			}

			if(g_inputSystem->IsKeyDown('Q'))
			{
				intendedDirection += Vec3::DOWN;
				wasInputPressed = true;
			}
		}

		if(wasInputPressed)
		{
			Vec3 dir = intendedDirection.GetNormalized();
			m_followingEntity->MoveInDirection(dir * moveSpeed, didJump);
		}
		else if(didJump)
		{
			m_followingEntity->MoveInDirection(Vec3::ZERO, true);
		}		
		
		if(m_cameraMode != CAMERA_INDEPENDANT)
		{
			m_camera.m_orientation	= m_followingEntity->GetRotation();
			m_camera.m_position		= m_followingEntity->GetEyePosition();

			if(m_cameraMode == CAMERA_THIRD_PERSON)
			{
				m_camera.m_position -= m_followingEntity->GetForward() * 4.f;
			}
		}
	}
	else
	{
		m_camera.m_orientation.m_yawDegrees -= g_inputSystem->GetCursorClientDelta().x * mouseSensitivity;
		m_camera.m_orientation.m_pitchDegrees += g_inputSystem->GetCursorClientDelta().y * mouseSensitivity;

		m_camera.m_orientation.m_pitchDegrees = GetClamped(m_camera.m_orientation.m_pitchDegrees, -80.f, 85.f);


		Vec3 fwdVector;
		Vec3 leftVector;
		Vec3 upVector;

		m_camera.m_orientation.GetAsVectors_IFwd_JLeft_KUp(fwdVector, leftVector, upVector);

		if(m_cameraMode == CAMERA_SPECTATOR_XY)
		{
			fwdVector = Vec3(fwdVector.x, fwdVector.y, 0.f).GetNormalized();
			leftVector = Vec3(leftVector.x, leftVector.y, 0.f).GetNormalized();
		}

		if(g_inputSystem->IsKeyDown('W'))
		{
			m_camera.m_position += fwdVector * moveSpeed * deltaSeconds;
		}

		if(g_inputSystem->IsKeyDown('S'))
		{
			m_camera.m_position -= fwdVector * moveSpeed * deltaSeconds;
		}

		if(g_inputSystem->IsKeyDown('A'))
		{
			m_camera.m_position += leftVector * moveSpeed * deltaSeconds;
		}

		if(g_inputSystem->IsKeyDown('D'))
		{
			m_camera.m_position -= leftVector * moveSpeed * deltaSeconds;
		}

		if(g_inputSystem->IsKeyDown('E'))
		{
			m_camera.m_position.z += moveSpeed * deltaSeconds;
		}

		if(g_inputSystem->IsKeyDown('Q'))
		{
			m_camera.m_position.z -= moveSpeed * deltaSeconds;
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GameCamera::FollowEntity(Entity* entityToFollow)
{
	m_followingEntity = entityToFollow;
	
	m_camera.m_position		= m_followingEntity->GetPosition() + m_followingEntity->GetUp() * m_followingEntity->GetEyeHeight();
	m_camera.m_orientation	= m_followingEntity->GetRotation();	
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GameCamera::AddVertsForHitSurface(std::vector<Vertex_PCU>& verts)
{
	Player* player = dynamic_cast<Player*>(m_followingEntity);

	Vec3			hitBlockNormal	= player->m_rayResult.m_impactNormal;
	BlockIterator	iter			= player->m_rayResult.m_blockIterator;
	uint32_t		blockIndex		= iter.m_blockIndex;
	IntVec3			hitBlockCoord	= GetGlobalCoords(iter.m_chunk->GetChunkCoordinates(), blockIndex);
	Vec3			hitNormal		= player->m_rayResult.m_impactNormal;
	AABB3			hitBlockBounds	= AABB3(Vec3(hitBlockCoord), Vec3(hitBlockCoord + IntVec3::ONE));

	std::vector<Vec3> eightCornerPoints;

	hitBlockBounds.GetCornerPoints(eightCornerPoints);

	Vec3 pointOffset = Vec3(0.01f) * hitBlockNormal;

	if(hitNormal.x > 0.f)
	{
		AddVertsForWireframeQuad3D(verts, eightCornerPoints[AABB3_FORWARD_RIGHT_BOTTOM] + pointOffset, eightCornerPoints[AABB3_FORWARD_LEFT_BOTTOM] + pointOffset, eightCornerPoints[AABB3_FORWARD_LEFT_TOP] + pointOffset,
							eightCornerPoints[AABB3_FORWARD_RIGHT_TOP] + pointOffset, Rgba8::WHITE);
	}

	if(hitNormal.x < 0.f)
	{
		AddVertsForWireframeQuad3D(verts, eightCornerPoints[AABB3_BACK_LEFT_BOTTOM] + pointOffset, eightCornerPoints[AABB3_BACK_RIGHT_BOTTOM] + pointOffset, eightCornerPoints[AABB3_BACK_RIGHT_TOP] + pointOffset, 
			eightCornerPoints[AABB3_BACK_LEFT_TOP] + pointOffset, Rgba8::WHITE);
	}
	if(hitNormal.y > 0.f)
	{
		AddVertsForWireframeQuad3D(verts, eightCornerPoints[AABB3_FORWARD_LEFT_BOTTOM] + pointOffset, eightCornerPoints[AABB3_BACK_LEFT_BOTTOM] + pointOffset, 
			eightCornerPoints[AABB3_BACK_LEFT_TOP] + pointOffset, eightCornerPoints[AABB3_FORWARD_LEFT_TOP] + pointOffset, Rgba8::WHITE);
	}

	if(hitNormal.y < 0.f)
	{
		AddVertsForWireframeQuad3D(verts, eightCornerPoints[AABB3_BACK_RIGHT_BOTTOM] + pointOffset, eightCornerPoints[AABB3_FORWARD_RIGHT_BOTTOM] + pointOffset, eightCornerPoints[AABB3_FORWARD_RIGHT_TOP] + pointOffset, 
			eightCornerPoints[AABB3_BACK_RIGHT_TOP] + pointOffset, Rgba8::WHITE);
	}
	if(hitNormal.z > 0.f)
	{
		AddVertsForWireframeQuad3D(verts, eightCornerPoints[AABB3_BACK_RIGHT_TOP] + pointOffset, eightCornerPoints[AABB3_FORWARD_RIGHT_TOP] + pointOffset, 
			eightCornerPoints[AABB3_FORWARD_LEFT_TOP] + pointOffset, eightCornerPoints[AABB3_BACK_LEFT_TOP] + pointOffset, Rgba8::WHITE);
	}
	if(hitNormal.z < 0.f)
	{
		AddVertsForWireframeQuad3D(verts, eightCornerPoints[AABB3_FORWARD_RIGHT_BOTTOM] + pointOffset, eightCornerPoints[AABB3_BACK_RIGHT_BOTTOM] + pointOffset, 
			eightCornerPoints[AABB3_BACK_LEFT_BOTTOM] + pointOffset, eightCornerPoints[AABB3_FORWARD_LEFT_BOTTOM] + pointOffset, Rgba8::WHITE);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GameCamera::SwitchCameraModes()
{
	m_cameraMode = static_cast<CameraMode>((m_cameraMode + 1) % (CAMERA_COUNT));
}

