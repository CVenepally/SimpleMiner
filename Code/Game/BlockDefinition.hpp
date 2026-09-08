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
