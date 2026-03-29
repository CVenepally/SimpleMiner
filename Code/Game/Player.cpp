#include "Game/Player.hpp"
#include "Game/GameCommon.hpp"
#include "Game/Chunk.hpp"
#include "Game/World.hpp"
#include "Game/Game.hpp"
#include "Game/BlockIterator.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/Inventory.hpp"

#include "Engine/Core/DebugRender.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

extern SpriteSheet* g_blockSpriteSheet;
extern SpriteSheet* g_itemSpriteSheet;
extern BitmapFont* g_gameFont;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Player::Player()
	:Entity()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Player::Player(Vec3 const& position, EulerAngles const& orientation, float maxMoveSpeed, float height, float width, float eyeHeight)
	: Entity(position, orientation, maxMoveSpeed, height, width, eyeHeight)
{
	m_horizontalGroundAcceleration = g_playerHorizontalGroundAcceleration;
	m_horizontalAirAcceleration = g_playerHorizontalAirAcceleration;

	m_horizontalGroundDragCoefficient = g_playerHorizontalGroundDragCoefficient;
	m_horizontalAirDragCoefficient = g_playerHorizontalAirDragCoefficient;

	m_maxHorizontalSpeed = g_playerMaxHorizontalSpeed;
	m_maxVerticalSpeed = g_playerMaxVerticalSpeed;

	m_gravityAcceleration = g_playerGravityAcceleration;
	m_jumpImpulse = g_playerJumpImpulse;

	m_inventory = new Inventory();

	MakeInventoryUISlots();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Player::~Player()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::Update()
{
	Entity::Update();
	m_rayResult = RaycastVsWorld(GetEyePosition(), GetForward(), RAY_MAX_LENGTH);
	m_blockTypeToPlace = m_inventory->m_slots[m_inventory->m_activeSlotIndex].m_itemType;

// 	DebugAddWorldLine(GetEyePosition(), m_rayResult.m_rayForwardNormal, m_rayResult.m_impactDistance, 0.01f, 0.f, Rgba8::GREY, DebugRenderMode::X_RAY);
// 	DebugAddWorldArrow(m_rayResult.m_impactPos, m_rayResult.m_impactNormal, 1.f, 0.03f, 0.f, Rgba8::WHITE, DebugRenderMode::X_RAY);

	DebugRender();

	if(g_inputSystem->WasKeyJustPressed('I'))
	{
		m_inventory->PrintToConsole();
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::PhysicsUpdate(float deltaSeconds)
{
	Entity::PhysicsUpdate(deltaSeconds);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::Render()
{
	g_theRenderer->SetModelConstants(GetPositionOnlyTransfrom());
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::WIREFRAME_CULL_NONE);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(m_debugVerts);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::DebugRender()
{
	std::string physicsMode;

	switch(m_physicsMode)
	{
		case PHYSICS_WALKING: 
		{
			physicsMode = "[V] PhysicsMode: Walking";
			break;
		}
		case PHYSICS_FLYING:
		{
			physicsMode = "[V] PhysicsMode: Flying";
			break;
		}
		case PHYSICS_NOCLIP:
		{
			physicsMode = "[V] PhysicsMode: Noclip";
			break;
		}
	}
	AABB2 textBox = AABB2(IntVec2(0, 0), g_theWindow->GetClientDimensions());
	float fontSize = 12.f;

	DebugAddScreenText(physicsMode, textBox, fontSize, Vec2(1.f, 0.92f), 0.f, Rgba8::YELLOW);

	if(Game::ShowEntityDebugs())
	{
		std::string info =
			Stringf("Grounded: %s\nPos: (%.2f, %.2f, %.2f)\nVel: (%.2f, %.2f, %.2f)\nAccel: (%.2f, %.2f, %.2f)",
					m_isGrounded ? "TRUE" : "FALSE",
					m_position.x, m_position.y, m_position.z,
					m_currentVelocity.x, m_currentVelocity.y, m_currentVelocity.z,
					m_currentAcceleration.x, m_currentAcceleration.y, m_currentAcceleration.z);
		DebugAddScreenText(info, textBox, fontSize + 2.f, Vec2(1.f, 0.01f), 0.f, Rgba8::CYAN);
		DebugAddScreenText("Player Debugs", textBox, fontSize + 2.f, Vec2(1.f, 0.1f), 0.f, Rgba8::GREEN);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::DebugRenderInventory()
{
	std::vector<Vertex_PCU> verts;
	AABB2 screenBounds = AABB2(IntVec2(0, 0), g_theWindow->GetClientDimensions());

	AddVertsForAABB2D(verts, screenBounds, Rgba8(0, 0, 0, 100));

	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(nullptr);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(verts);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::RenderInventory()
{
	if(!m_isInventoryMenuOpen)
	{
		RenderHotbar();
	}
	else
	{
		Texture* inventoryMenuTexture = g_theRenderer->CreateOrGetTextureFromFilePath("Data/Images/inventory.png");

		std::vector<Vertex_PCU> overlayVerts;
		std::vector<Vertex_PCU> inventorySlotVerts;
		std::vector<Vertex_PCU> inventoryMenuVerts;
		std::vector<Vertex_PCU> inventoryTextVerts;

		AABB2 screenBox = AABB2(IntVec2::ZERO, g_theWindow->GetClientDimensions());
		
		AddVertsForAABB2D(overlayVerts, screenBox, Rgba8(0, 0, 0, 200));

		g_theRenderer->SetModelConstants();
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexArray(overlayVerts);

		// Inventory Slots
		AABB2 inventoryBox = screenBox;
		inventoryBox.ShrinkAABB2(0.7f);
		inventoryBox.m_mins.y -= inventoryBox.m_mins.y * 0.4f;

		inventoryBox.SetCenter(screenBox.GetCenter());

		AddVertsForAABB2D(inventoryMenuVerts, inventoryBox, Rgba8::WHITE);

		g_theRenderer->SetModelConstants();
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->BindTexture(inventoryMenuTexture);
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexArray(inventoryMenuVerts);


		for(int i = 0; i < TOTAL_SLOTS_HOTBAR + TOTAL_STORAGE_SLOTS; i++)
		{
// 			if(m_inventory->m_slots[i].m_itemCount == 0)
// 			{
// 				continue;
// 			}

			AABB2 uvs = BlockDefinition::s_blockDefinitions[m_inventory->m_slots[i].m_itemType]->GetItemSpriteUVs();
			AddVertsForAABB2D(inventorySlotVerts, m_inventoryUISlots[i], Rgba8::WHITE, uvs);

			g_gameFont->AddVertsForTextInBox2D(inventoryTextVerts, Stringf("%d", m_inventory->m_slots[i].m_itemCount), m_inventoryUISlots[i], 12.f, Rgba8::WHITE, 1.f, Vec2(0.9f, 0.f));

		}

		if(!m_inventory->m_copiedSlot.IsEmpty())
		{

			AABB2 uvs = BlockDefinition::s_blockDefinitions[m_inventory->m_copiedSlot.m_itemType]->GetItemSpriteUVs();

			AABB2 block = m_inventoryUISlots[0];

			Vec2 mousePos = g_theWindow->GetNormalizedMouseUV();
			Vec2 trueMousePos = g_theWindow->GetClientDimensions().GetAsVec2() * mousePos;


			block.SetCenter(trueMousePos);

			AddVertsForAABB2D(inventorySlotVerts, block, Rgba8::WHITE, uvs);
			g_gameFont->AddVertsForTextInBox2D(inventoryTextVerts, Stringf("%d", m_inventory->m_copiedSlot.m_itemCount), block, 16.f, Rgba8::WHITE, 1.f, Vec2(1.f, 0.f));

			g_theRenderer->SetModelConstants();
			g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
			g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
			g_theRenderer->SetBlendMode(BlendMode::ALPHA);
			g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
			g_theRenderer->BindTexture(&g_itemSpriteSheet->GetTexture());
			g_theRenderer->BindShader(nullptr);
			g_theRenderer->DrawVertexArray(inventorySlotVerts);

			g_theRenderer->SetModelConstants();
			g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
			g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
			g_theRenderer->SetBlendMode(BlendMode::ALPHA);
			g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
			g_theRenderer->BindTexture(&g_gameFont->GetTexture());
			g_theRenderer->BindShader(nullptr);
			g_theRenderer->DrawVertexArray(inventoryTextVerts);

		}

		g_theRenderer->SetModelConstants();
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	//	g_theRenderer->BindTexture(nullptr);
		g_theRenderer->BindTexture(&g_itemSpriteSheet->GetTexture());
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexArray(inventorySlotVerts);

		g_theRenderer->SetModelConstants();
		g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
		g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
		g_theRenderer->SetBlendMode(BlendMode::ALPHA);
		g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
		g_theRenderer->BindTexture(&g_gameFont->GetTexture());
		g_theRenderer->BindShader(nullptr);
		g_theRenderer->DrawVertexArray(inventoryTextVerts);

	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::RenderHotbar()
{
	Texture* hotbarTexture = g_theRenderer->CreateOrGetTextureFromFilePath("Data/Images/hotbar.png");
	Texture* activeTexture = g_theRenderer->CreateOrGetTextureFromFilePath("Data/Images/hotbar_selection.png");

	std::vector<Vertex_PCU> verts;
	std::vector<Vertex_PCU> slotVerts;
	std::vector<Vertex_PCU> selectedSlotVerts;
	std::vector<Vertex_PCU> textVerts;

	AABB2 thrityLeft;
	thrityLeft.m_mins.x = 0.f;
	thrityLeft.m_mins.y = 0.f;
	thrityLeft.m_maxs.x = g_theWindow->GetClientDimensions().GetAsVec2().x * 0.32f;
	thrityLeft.m_maxs.y = g_theWindow->GetClientDimensions().GetAsVec2().y;

	AABB2 thrityRight;
	thrityRight.m_mins.y = 0.f;
	thrityRight.m_maxs.x = g_theWindow->GetClientDimensions().GetAsVec2().x;
	thrityRight.m_maxs.y = g_theWindow->GetClientDimensions().GetAsVec2().y;
	thrityRight.m_mins.x = thrityRight.m_maxs.x - g_theWindow->GetClientDimensions().GetAsVec2().x * 0.32f;

	AABB2 fourtyMid;
	fourtyMid.m_mins.x = thrityLeft.m_maxs.x;
	fourtyMid.m_mins.y = g_theWindow->GetClientDimensions().GetAsVec2().y * 0.008f;
	fourtyMid.m_maxs.x = thrityRight.m_mins.x;
	fourtyMid.m_maxs.y = (fourtyMid.m_maxs.x - fourtyMid.m_mins.x) / 9.f;

	float fortyMidWidth = fourtyMid.GetDimensions().x;

	float fortyMidWitdthNinth = fortyMidWidth / 9.f;

	for(int x = 0; x < 9; x++)
	{
		if(m_inventory->m_slots[x].m_itemCount == 0)
		{
			continue;
		}

		AABB2 box;
		box.m_mins.x = fourtyMid.m_mins.x + (x * fortyMidWitdthNinth);
		box.m_mins.y = fourtyMid.m_mins.y;
		box.m_maxs.x = box.m_mins.x + fortyMidWitdthNinth;
		box.m_maxs.y = fourtyMid.m_maxs.y;

		if(m_inventory->m_activeSlotIndex == x)
		{
			AddVertsForAABB2D(selectedSlotVerts, box, Rgba8::WHITE);
		}

		g_gameFont->AddVertsForTextInBox2D(textVerts, Stringf("%d", m_inventory->m_slots[x].m_itemCount), box, 16.f, Rgba8::WHITE, 1.f, Vec2(0.8f, 0.23f));

		box.ShrinkAABB2(0.45f);

		AABB2 uvs = BlockDefinition::s_blockDefinitions[m_inventory->m_slots[x].m_itemType]->GetItemSpriteUVs();

		AddVertsForAABB2D(slotVerts, box, Rgba8::WHITE, uvs);

	}
	//	AddVertsForAABB2D(verts, thrityLeft, Rgba8(255, 0, 0, 100));
	AddVertsForAABB2D(verts, fourtyMid, Rgba8(255, 255, 255, 255));
	//	AddVertsForAABB2D(verts, thrityRight, Rgba8(255, 0, 0, 100));

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(hotbarTexture);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(verts);

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(&g_itemSpriteSheet->GetTexture());
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(slotVerts);

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(activeTexture);
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(selectedSlotVerts);

	g_theRenderer->SetModelConstants();
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetBlendMode(BlendMode::ALPHA);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->BindTexture(&g_gameFont->GetTexture());
	g_theRenderer->BindShader(nullptr);
	g_theRenderer->DrawVertexArray(textVerts);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::PlaceBlock()
{
	if(!m_rayResult.m_didImpact || m_blockTypeToPlace == 0)
	{
		return;
	}
	Vec3			hitBlockNormal	= m_rayResult.m_impactNormal;
	BlockIterator	iter			= m_rayResult.m_blockIterator;
	uint32_t		blockIndex		= iter.m_blockIndex;
	IntVec3			hitBlockCoord	= GetGlobalCoords(iter.m_chunk->GetChunkCoordinates(), blockIndex);

	IntVec3			newBlockCoord	= hitBlockCoord + IntVec3(hitBlockNormal);
	uint32_t		newBlockIndex	= GlobalCoordsToIndex(newBlockCoord);
	IntVec2			newChunkCoord	= GetChunkCoords(newBlockCoord);
	Chunk*			newChunk		= Game::GetWorld()->GetChunk(newChunkCoord);
	BlockIterator	newIter			= BlockIterator(newChunk, newBlockIndex);

	bool placed = Game::GetWorld()->PlaceBlock(newIter, m_blockTypeToPlace);
	
	if(placed)
	{
		m_inventory->RemoveItemFromActiveSlot();
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::Dig()
{
	BlockIterator& iter = m_rayResult.m_blockIterator;

	if(iter.m_chunk == nullptr)
	{
		return;
	}

	unsigned char blockType = iter.GetBlockType();

	Game::GetWorld()->DigBlock(iter);
	
	m_inventory->AddItemToInventory(blockType, 1);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::SwitchBlockType()
{
	if(g_inputSystem->WasKeyJustPressed('1'))
	{
		m_blockTypeToPlace = BlockDefinition::GetBlockDefinitionIndex("Glowstone");
	}

	if(g_inputSystem->WasKeyJustPressed('2'))
	{
		m_blockTypeToPlace = BlockDefinition::GetBlockDefinitionIndex("Cobblestone");
	}

	if(g_inputSystem->WasKeyJustPressed('3'))
	{
		m_blockTypeToPlace = BlockDefinition::GetBlockDefinitionIndex("ChiseledBrick");
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::NextHotbarSlot()
{
	m_inventory->m_activeSlotIndex = (m_inventory->m_activeSlotIndex + 1) % TOTAL_SLOTS_HOTBAR;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::PrevHotbarSlot()
{
	m_inventory->m_activeSlotIndex = (m_inventory->m_activeSlotIndex - 1 + TOTAL_SLOTS_HOTBAR) % TOTAL_SLOTS_HOTBAR;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::SetHotbarSlot(int slot)
{
	m_inventory->m_activeSlotIndex = slot;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::HitTestInvetoryUI(Vec2 const& mousePos)
{
	AABB2 screenBox = AABB2(IntVec2::ZERO, g_theWindow->GetClientDimensions());
	
	Vec2 trueMousePos = g_theWindow->GetClientDimensions().GetAsVec2() * mousePos;

	for(int i = 0; i < TOTAL_SLOTS_HOTBAR + TOTAL_STORAGE_SLOTS; i++)
	{
		if(m_inventoryUISlots[i].IsPointInside(trueMousePos))
		{
			DebugAddScreenText(Stringf("Hovered Slot %d", i), screenBox, 12.f, Vec2(0.5f, 0.9f), 0.f, Rgba8::YELLOW);
			return;
		}
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::MakeInventoryUISlots()
{
	AABB2 screenBox = AABB2(IntVec2::ZERO, g_theWindow->GetClientDimensions());

	AABB2 inventoryBox = screenBox;

	float oneOverNine = 1.f / 9.f;

	// Shrink the overall UI area
	inventoryBox.ShrinkAABB2(0.73f);
	
	Vec2 newCenter = Vec2(inventoryBox.GetCenter().x, inventoryBox.GetCenter().y - inventoryBox.GetCenter().y * 0.12f);

	inventoryBox.SetCenter(newCenter);

	float slotWidth = (inventoryBox.m_maxs.x - inventoryBox.m_mins.x) * oneOverNine;
	float slotHeight = slotWidth;

	for(int slot = 0; slot < TOTAL_SLOTS_HOTBAR + TOTAL_STORAGE_SLOTS; slot++)
	{
		AABB2 slotBox;
		slotBox.m_mins.x = inventoryBox.m_mins.x + ((slot % 9) * slotWidth);
		slotBox.m_mins.y = inventoryBox.m_mins.y + ((slot / 9) * (slotHeight * 0.935f));
		slotBox.m_maxs.x = slotBox.m_mins.x + slotWidth;
		slotBox.m_maxs.y = slotBox.m_mins.y + slotHeight;

		slotBox.ShrinkAABB2(0.22f);

		if(slot * oneOverNine < 1.f)
		{
			Vec2 center = slotBox.GetCenter();
			Vec2 newSlotCenter = Vec2(center.x, center.y - center.y * 0.03f);

			slotBox.SetCenter(newSlotCenter);
		}

		if(slot % 9 < 4)
		{
			Vec2 center = slotBox.GetCenter();
			Vec2 newSlotCenter = Vec2(center.x - center.x * 0.003f, center.y);
			slotBox.SetCenter(newSlotCenter);
		}

		if(slot % 9 > 4)
		{
			Vec2 center = slotBox.GetCenter();
			Vec2 newSlotCenter = Vec2(center.x + center.x * 0.003f, center.y);
			slotBox.SetCenter(newSlotCenter);
		}

		m_inventoryUISlots[slot] = slotBox;
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::HandleInventoryLeftClick()
{
	Vec2 mousePos = g_theWindow->GetNormalizedMouseUV();

	AABB2 screenBox = AABB2(IntVec2::ZERO, g_theWindow->GetClientDimensions());

	Vec2 trueMousePos = g_theWindow->GetClientDimensions().GetAsVec2() * mousePos;

	for(int i = 0; i < TOTAL_SLOTS_HOTBAR + TOTAL_STORAGE_SLOTS; i++)
	{
		if(m_inventoryUISlots[i].IsPointInside(trueMousePos))
		{
			if(m_inventory->m_copiedSlot.m_itemType != m_inventory->m_slots[i].m_itemType)
			{
				Slot tempSlot = m_inventory->m_slots[i];
				m_inventory->m_slots[i] = m_inventory->m_copiedSlot;
				m_inventory->m_copiedSlot = tempSlot;
			}
			else
			{
				if(m_inventory->m_slots[i].m_itemCount == SLOTS_MAX_STACK_SIZE)
				{
					return;
				}

				int availableSpaceInSelectedSlot = SLOTS_MAX_STACK_SIZE - m_inventory->m_slots[i].m_itemCount;

				if(m_inventory->m_copiedSlot.m_itemCount <= availableSpaceInSelectedSlot)
				{
					m_inventory->m_slots[i].m_itemCount += m_inventory->m_copiedSlot.m_itemCount;
					m_inventory->m_copiedSlot.m_itemCount = 0;
					m_inventory->m_copiedSlot.m_itemType = 0;
				}
				else
				{
					m_inventory->m_slots[i].m_itemCount = SLOTS_MAX_STACK_SIZE;
					int itemDifference = m_inventory->m_copiedSlot.m_itemCount - availableSpaceInSelectedSlot;

					m_inventory->m_copiedSlot.m_itemCount = static_cast<uint8_t>(itemDifference);
				}
			}
			return;
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Player::HandleInventoryRightClick()
{
	Vec2 mousePos = g_theWindow->GetNormalizedMouseUV();

	AABB2 screenBox = AABB2(IntVec2::ZERO, g_theWindow->GetClientDimensions());

	Vec2 trueMousePos = g_theWindow->GetClientDimensions().GetAsVec2() * mousePos;

	for(int i = 0; i < TOTAL_SLOTS_HOTBAR + TOTAL_STORAGE_SLOTS; i++)
	{
		if(m_inventoryUISlots[i].IsPointInside(trueMousePos))
		{
			if(m_inventory->m_copiedSlot.IsEmpty())
			{
				if(m_inventory->m_slots[i].IsEmpty())
				{
					return;
				}

				m_inventory->m_copiedSlot.m_itemType = m_inventory->m_slots[i].m_itemType;
				m_inventory->m_copiedSlot.m_itemCount = static_cast<uint8_t>(RoundDownToInt(static_cast<float>(m_inventory->m_slots[i].m_itemCount) / 2.f));

				if(m_inventory->m_slots[i].m_itemCount % 2 != 0)
				{
					m_inventory->m_copiedSlot.m_itemCount += 1;
				}

				m_inventory->m_slots[i].m_itemCount -= m_inventory->m_copiedSlot.m_itemCount;

				if(m_inventory->m_slots[i].m_itemCount < 1)
				{
					m_inventory->m_slots[i].m_itemType = 0;
				}

			}
			else
			{
				if(m_inventory->m_slots[i].IsEmpty())
				{
					m_inventory->m_slots[i].m_itemType = m_inventory->m_copiedSlot.m_itemType;
					m_inventory->m_slots[i].m_itemCount = 1;
					m_inventory->m_copiedSlot.m_itemCount -= 1;
					if(m_inventory->m_copiedSlot.m_itemCount < 1)
					{
						m_inventory->m_copiedSlot.m_itemType = 0;
					}
					return;
				}
				
				if(m_inventory->m_slots[i].m_itemType != m_inventory->m_copiedSlot.m_itemType)
				{
					Slot tempSlot = m_inventory->m_slots[i];
					m_inventory->m_slots[i] = m_inventory->m_copiedSlot;
					m_inventory->m_copiedSlot = tempSlot;
					return;
				}

				if(m_inventory->m_slots[i].m_itemCount == SLOTS_MAX_STACK_SIZE)
				{
					return;
				}

				m_inventory->m_slots[i].m_itemCount += 1;
				m_inventory->m_copiedSlot.m_itemCount -= 1;
				if(m_inventory->m_copiedSlot.m_itemCount < 1)
				{
					m_inventory->m_copiedSlot.m_itemType = 0;
				}
			}
			return;
		}
	}

}
