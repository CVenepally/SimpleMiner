#pragma once
#include "Game/Entity.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Inventory;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Player : public Entity
{
	friend class GameCamera;
	friend class Game;
	friend class World;

private:
	
					Player();
	explicit		Player(Vec3 const& position, EulerAngles const& orientation, float maxMoveSpeed = 1.f, float height = 1.f, float width = 1.f, float eyeHeight = 0.8f);
	virtual			~Player();

	virtual void	Update()		override;
	virtual void	PhysicsUpdate(float deltaSeconds) override;
	virtual void	Render()		override;
	virtual void	DebugRender()	override;

	void			DebugRenderInventory();
	void			RenderInventory();

	void			RenderHotbar();

	void			PlaceBlock();
	void			Dig();
	void			SwitchBlockType();

	void			NextHotbarSlot();
	void			PrevHotbarSlot();
	void			SetHotbarSlot(int slot);
	void			HitTestInvetoryUI(Vec2 const& mousePos);
	void			MakeInventoryUISlots();

	void			HandleInventoryLeftClick();
	void			HandleInventoryRightClick();

private:

	unsigned char		m_blockTypeToPlace		= 0;
	GameRaycastResult3D	m_rayResult				= GameRaycastResult3D();
	Inventory*			m_inventory				= nullptr;
	bool				m_isInventoryMenuOpen	= false;
	AABB2				m_inventoryUISlots[TOTAL_SLOTS_HOTBAR + TOTAL_STORAGE_SLOTS + TOTAL_SLOTS_OFFHAND];
};