#pragma once
#include "Game/GameRaycastResult3D.hpp"

#include "Engine/Math/Mat44.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/Vertex_PCU.hpp"

#include <vector>
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------	
enum PhysicsMode
{
	PHYSICS_WALKING,
	PHYSICS_FLYING,
	PHYSICS_NOCLIP,

	PHYSICS_COUNT
};
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Entity
{
	friend class Game;
	friend class GameCamera;
	friend class World;

protected:
						Entity();
	explicit			Entity(Vec3 const& position, EulerAngles const& orientation, float maxMoveSpeed = 0.f, float height = 1.f, float width = 1.f, float eyeHeight = 1.f);

	
	virtual				~Entity();

	virtual void		Update();
	virtual void		PhysicsUpdate(float deltaSeconds);
	virtual void		CollisionDetection(float deltaSeconds);
	virtual void		Render()		= 0;
	virtual void		DebugRender()	= 0;

	inline	void		SetPhysicsMode(PhysicsMode mode) { m_physicsMode = mode; }
	inline	void		SwitchPhysicsMode() { m_physicsMode = static_cast<PhysicsMode>((m_physicsMode + 1) % PHYSICS_COUNT); }

	void				MoveInDirection(Vec3 const& moveDirection, bool didJump);
	inline void			AddForce(Vec3 const& force) {m_currentAcceleration += force;}
	inline void			AddImpulse(Vec3 const& impulse){m_currentVelocity += impulse;}

	void				PrecomputeCornerPoints();

	void				TransformCornerPointsToWorld();

	GameRaycastResult3D	RaycastVsWorld(Vec3 const& startPosition, Vec3 const& direction, float length = RAY_MAX_LENGTH);
	bool				GroundCheck();

public:
	inline Vec3			GetPosition() const {return m_position;}	
	inline EulerAngles	GetRotation() const {return m_rotation;}
	Vec3				GetForward() const;
	Vec3				GetLeft() const;		
	Vec3				GetUp() const;		
	Mat44				GetPositionOnlyTransfrom() const;
	Mat44				GetTransfrom() const;
	inline PhysicsMode	GetPhysicsMode() const	{return m_physicsMode;}
	inline Vec3			GetVelocity() const		{return m_currentVelocity;}
	inline float		GetHeight() const		{return m_height;}
	inline float		GetEyeHeight() const	{return m_eyeHeight;}
	inline float		GetWidth() const		{return m_width;}

	inline Vec3			GetEyePosition() const	{return m_position + Vec3::UP * m_eyeHeight;}

	inline void			SetRotation(EulerAngles const& rotation) {m_rotation = rotation;};
	inline void			SetPosition(Vec3 const& position) {m_position = position;};

protected:
	
	enum ColliderPoints
	{
		CP_BACK_RIGHT_BOTTOM,     
		CP_BACK_RIGHT_TOP,        
		CP_BACK_LEFT_TOP,         
		CP_BACK_LEFT_BOTTOM,      

		CP_FORWARD_LEFT_BOTTOM,   
		CP_FORWARD_RIGHT_BOTTOM,  
		CP_FORWARD_RIGHT_TOP,     
		CP_FORWARD_LEFT_TOP,      

		CP_MID_BACK_RIGHT,       
		CP_MID_BACK_LEFT,        
		CP_MID_FORWARD_LEFT,     
		CP_MID_FORWARD_RIGHT,    

		CP_COUNT
	};

	Vec3				m_position;
	EulerAngles			m_rotation;
	AABB3				m_collider;
	Vec3				m_pointsOnCollider_local[CP_COUNT] = {};
	Vec3				m_pointsOnCollider_world[CP_COUNT] = {};
	Vec3				m_currentAcceleration	=	Vec3();
	Vec3				m_currentVelocity		=	Vec3();

	float				m_horizontalGroundAcceleration	= 64.0f;
	float				m_horizontalAirAcceleration		= 4.0f;

	float				m_horizontalGroundDragCoefficient	= 8.0f;
	float				m_horizontalAirDragCoefficient		= 4.0f;

	float				m_maxHorizontalSpeed				= 10.0f;
	float				m_maxVerticalSpeed					= 56.0f;
	float				m_maxHorizontalSpeedFlying			= 200.0f;

	float				m_gravityAcceleration				= 9.8f;
	float				m_jumpImpulse						= 6.0f;

	bool				m_isGrounded	= true;

	float				m_height		= 0.f;
	float				m_width			= 0.f;
	float				m_eyeHeight		= 0.f;
	float				m_maxMoveSpeed	= 0.f;
	PhysicsMode			m_physicsMode	= PHYSICS_WALKING;

	// #ToDo: May be a buffer later?
	std::vector<Vertex_PCU>		m_debugVerts;
	std::vector<unsigned int>	m_debugInds;

};