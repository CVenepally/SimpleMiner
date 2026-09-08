#include "Game/Block.hpp"

#include "Engine/Math/AABB2.hpp"
#include "Game/BlockDefinition.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block::Block()
{
    m_blockDefinitionIndex = BlockDefinition::GetBlockDefinitionIndex("Air");
}

//------------------------------------------------------------------------------------------------------------------
bool Block::IsBlockVisible() const
{
    return BlockDefinition::s_blockDefinitions[m_blockDefinitionIndex]->m_isVisible;
}

//------------------------------------------------------------------------------------------------------------------
bool Block::IsBlockOpaque() const
{
    return BlockDefinition::s_blockDefinitions[m_blockDefinitionIndex]->m_isOpaque;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Block::IsBlockAir() const
{
    return m_blockDefinitionIndex == BlockDefinition::GetBlockDefinitionIndex("Air");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Block::IsBlockWater() const
{
	return m_blockDefinitionIndex == BlockDefinition::GetBlockDefinitionIndex("Water");
}

//------------------------------------------------------------------------------------------------------------------
AABB2 Block::GetBlockTopSpriteUVs() const
{
    return BlockDefinition::s_blockDefinitions[m_blockDefinitionIndex]->GetSpriteTopSpriteUVs();
}

//------------------------------------------------------------------------------------------------------------------
AABB2 Block::GetBlockBottomSpriteUVs() const
{
    return BlockDefinition::s_blockDefinitions[m_blockDefinitionIndex]->GetSpriteBottomSpriteUVs();
}

//------------------------------------------------------------------------------------------------------------------
AABB2 Block::GetBlockSideSpriteUVs() const
{
    return BlockDefinition::s_blockDefinitions[m_blockDefinitionIndex]->GetSpriteSideSpriteUVs();
}
