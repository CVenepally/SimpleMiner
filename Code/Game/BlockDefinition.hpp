#pragma once
#include <string>
#include <vector>

#include "Engine/Math/IntVec2.hpp"
#include "ThirdParty/tinyXML2/tinyxml2.h"

//------------------------------------------------------------------------------------------------------------------
class SpriteSheet;

struct AABB2;

typedef tinyxml2::XMLElement XmlElement;

//------------------------------------------------------------------------------------------------------------------
struct BlockTypeIndices
{
    unsigned char m_air             = 0;
    unsigned char m_water           = 0;
    unsigned char m_sand            = 0;
    unsigned char m_snow            = 0;
    unsigned char m_ice             = 0;
    unsigned char m_dirt            = 0;
    unsigned char m_stone           = 0;
    unsigned char m_coal            = 0;
    unsigned char m_iron            = 0;
    unsigned char m_gold            = 0;
    unsigned char m_diamond         = 0;
    unsigned char m_obsidian        = 0;
    unsigned char m_lava            = 0;
    unsigned char m_glowstone       = 0;
    unsigned char m_cobblestone     = 0;
    unsigned char m_chiseledBrick   = 0;
    unsigned char m_grass           = 0;
    unsigned char m_grassLight      = 0;
    unsigned char m_grassDark       = 0;
    unsigned char m_grassYellow     = 0;
};

extern BlockTypeIndices g_blockTypes;

//------------------------------------------------------------------------------------------------------------------
class BlockDefinition
{
    friend class Block;
    
public:
    BlockDefinition() = default;
    explicit BlockDefinition(XmlElement const& blockDefElement);
    ~BlockDefinition() = default;

    AABB2       GetSpriteTopSpriteUVs() const;
    AABB2       GetSpriteBottomSpriteUVs() const;
    AABB2       GetSpriteSideSpriteUVs() const;
    AABB2       GetItemSpriteUVs() const;
    std::string GetBlockName() const;
    static void             InitBlockDefinitions();
    static void             InitBlockTypeIndices();
    static unsigned char    GetBlockDefinitionIndex(std::string const& blockType);
    static int              GetIndoorLighting(unsigned char index) {return s_blockDefinitions[index]->m_indoorLighting;}
    
public:
    static std::vector<BlockDefinition*> s_blockDefinitions;

private:
    std::string m_blockName             = "Generic";
    bool        m_isVisible             = true;
    bool        m_isSolid               = false;
    bool        m_isOpaque              = false;
    IntVec2     m_topSpriteCoords       = IntVec2(0, 0);     
    IntVec2     m_bottomSpriteCoords    = IntVec2(0, 0);
    IntVec2     m_sideSpriteCoords      = IntVec2(0, 0);
    IntVec2     m_itemSpriteCoords      = IntVec2(0, 0);
    int         m_indoorLighting        = 0;
};
