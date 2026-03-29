#pragma once

#include "Game/GameRaycastResult3D.hpp"
#include "Engine/Renderer/Camera.hpp"
#include <vector>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct Vertex_PCU;

class Entity;

enum CameraMode
{
	CAMERA_FIRST_PERSON,
	CAMERA_THIRD_PERSON,
	CAMERA_SPECTATOR,
	CAMERA_SPECTATOR_XY,
	CAMERA_INDEPENDANT,

	CAMERA_COUNT
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class GameCamera
{
public:

	GameCamera();
	~GameCamera();

	void Update();
	void DebugRenders();
	void Render();

	Vec3 GetForwardVector() const;
	Vec3 GetPosition() const;
	
	void KeyboardControls(float deltaSeconds);

	void MovementControls(float deltaSeconds);	
	void FollowEntity(Entity* entityToFollow);

private:
	void		AddVertsForHitSurface(std::vector<Vertex_PCU>& verts);
	void		SwitchCameraModes();
	std::string CameraModeDebug();

public:
	Camera				m_camera;
	CameraMode			m_cameraMode = CAMERA_FIRST_PERSON;

	Entity*				m_followingEntity = nullptr;
};