#include "Game/BlockDefinition.hpp"
#include "Game/GameCommon.hpp"

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/XMLUtils.hpp"

#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

//------------------------------------------------------------------------------------------------------------------
std::vector<BlockDefinition*> BlockDefinition::s_blockDefinitions;
BlockTypeIndices g_blockTypes;
extern SpriteSheet* g_blockSpriteSheet;
extern SpriteSheet* g_itemSpriteSheet;

//------------------------------------------------------------------------------------------------------------------
BlockDefinition::BlockDefinition(XmlElement const& blockDefElement)
{
    m_blockName             = ParseXmlAttribute(blockDefElement, "name", m_blockName);
    m_isVisible             = ParseXmlAttribute(blockDefElement, "isVisible", m_isVisible);
    m_isSolid               = ParseXmlAttribute(blockDefElement, "isSolid", m_isSolid);
    m_isOpaque              = ParseXmlAttribute(blockDefElement, "isOpaque", m_isOpaque);
    m_topSpriteCoords       = ParseXmlAttribute(blockDefElement, "topSpriteCoords", m_topSpriteCoords);
    m_bottomSpriteCoords    = ParseXmlAttribute(blockDefElement, "bottomSpriteCoords", m_bottomSpriteCoords);
    m_sideSpriteCoords      = ParseXmlAttribute(blockDefElement, "sideSpriteCoords", m_sideSpriteCoords);
    m_itemSpriteCoords      = ParseXmlAttribute(blockDefElement, "itemSpriteCoords", m_itemSpriteCoords);
    m_indoorLighting        = ParseXmlAttribute(blockDefElement, "indoorLighting", m_indoorLighting);
}

//------------------------------------------------------------------------------------------------------------------
AABB2 BlockDefinition::GetSpriteTopSpriteUVs() const
{
    return g_blockSpriteSheet->GetSpriteUVs(m_topSpriteCoords);
}

//------------------------------------------------------------------------------------------------------------------
AABB2 BlockDefinition::GetSpriteBottomSpriteUVs() const
{
    return g_blockSpriteSheet->GetSpriteUVs(m_bottomSpriteCoords);
}

//------------------------------------------------------------------------------------------------------------------
AABB2 BlockDefinition::GetSpriteSideSpriteUVs() const
{
    return g_blockSpriteSheet->GetSpriteUVs(m_sideSpriteCoords);
}

//------------------------------------------------------------------------------------------------------------------
AABB2 BlockDefinition::GetItemSpriteUVs() const
{
    return g_itemSpriteSheet->GetSpriteUVs(m_itemSpriteCoords);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string BlockDefinition::GetBlockName() const
{
    return m_blockName;
}

//------------------------------------------------------------------------------------------------------------------
void BlockDefinition::InitBlockDefinitions()
{
    XmlDocument blockDefinition;

    char const* filePath                = "Data/Definitions/BlockDefinitions.xml";
    XmlResult   blockDefinitionResult   = blockDefinition.LoadFile(filePath);
    GUARANTEE_OR_DIE(blockDefinitionResult == tinyxml2::XML_SUCCESS, "Could not open BlockDefinitions.xml")

    XmlElement* rootElement = blockDefinition.RootElement();
    GUARANTEE_OR_DIE(rootElement, "Failed to access BlockDefinition root element")

    XmlElement* blockDefElement = rootElement->FirstChildElement();

    while(blockDefElement)
    {
        std::string name = blockDefElement->Name();
        GUARANTEE_OR_DIE(name == "BlockDefinition", "Child Element(s) 'BlockDefinition' not found")

        BlockDefinition* newBlockDefinition = new BlockDefinition(*blockDefElement);
        s_blockDefinitions.push_back(newBlockDefinition);
        
        blockDefElement = blockDefElement->NextSiblingElement();
    }

    InitBlockTypeIndices();
}

//------------------------------------------------------------------------------------------------------------------
void BlockDefinition::InitBlockTypeIndices()
{
    g_blockTypes.m_air              = GetBlockDefinitionIndex("Air");
    g_blockTypes.m_water            = GetBlockDefinitionIndex("Water");
    g_blockTypes.m_sand             = GetBlockDefinitionIndex("Sand");
    g_blockTypes.m_snow             = GetBlockDefinitionIndex("Snow");
    g_blockTypes.m_ice              = GetBlockDefinitionIndex("Ice");
    g_blockTypes.m_dirt             = GetBlockDefinitionIndex("Dirt");
    g_blockTypes.m_stone            = GetBlockDefinitionIndex("Stone");
    g_blockTypes.m_coal             = GetBlockDefinitionIndex("Coal");
    g_blockTypes.m_iron             = GetBlockDefinitionIndex("Iron");
    g_blockTypes.m_gold             = GetBlockDefinitionIndex("Gold");
    g_blockTypes.m_diamond          = GetBlockDefinitionIndex("Diamond");
    g_blockTypes.m_obsidian         = GetBlockDefinitionIndex("Obsidian");
    g_blockTypes.m_lava             = GetBlockDefinitionIndex("Lava");
    g_blockTypes.m_glowstone        = GetBlockDefinitionIndex("Glowstone");
    g_blockTypes.m_cobblestone      = GetBlockDefinitionIndex("Cobblestone");
    g_blockTypes.m_chiseledBrick    = GetBlockDefinitionIndex("ChiseledBrick");
    g_blockTypes.m_grass            = GetBlockDefinitionIndex("Grass");
    g_blockTypes.m_grassLight       = GetBlockDefinitionIndex("GrassLight");
    g_blockTypes.m_grassDark        = GetBlockDefinitionIndex("GrassDark");
    g_blockTypes.m_grassYellow      = GetBlockDefinitionIndex("GrassYellow");
}

//------------------------------------------------------------------------------------------------------------------
unsigned char BlockDefinition::GetBlockDefinitionIndex(std::string const& blockType)
{
    for(size_t index = 0; index < s_blockDefinitions.size(); ++index)
    {
        if (s_blockDefinitions[index]->m_blockName == blockType)
        {
            return static_cast<unsigned char>(index);
        }
    }
    
    return 0;
}

