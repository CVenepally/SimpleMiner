#pragma once
#include <cstdint>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct AABB2;

constexpr int INDOOR_LIGHT_INFLUENCE_BITS = 4;
constexpr int OUTDOOR_LIGHT_INFLUENCE_BITS = 4;

constexpr int INDOOR_LIGHT_INFLUENCE_MAX = (1 << INDOOR_LIGHT_INFLUENCE_BITS) - 1;
constexpr int OUTDOOR_LIGHT_INFLUENCE_MAX = (1 << OUTDOOR_LIGHT_INFLUENCE_BITS) - 1;

constexpr int INDOOR_LIGHT_INFLUENCE_MASK = INDOOR_LIGHT_INFLUENCE_MAX;
constexpr int OUTDOOR_LIGHT_INFLUENCE_MASK = OUTDOOR_LIGHT_INFLUENCE_MAX << INDOOR_LIGHT_INFLUENCE_BITS;

enum BlockBitFlag : uint8_t
{
    BLOCK_BIT_FLAG_IS_SKY           = 1 << 0,
    BLOCK_BIT_FLAG_IS_LIGHT_DIRTY   = 1 << 1,
    BLOCK_BIT_FLAG_IS_LIGHT_OPAQUE  = 1 << 2,
    BLOCK_BIT_FLAG_IS_SOLID         = 1 << 3,
    BLOCK_BIT_FLAG_IS_VISIBLE       = 1 << 4,
};

//------------------------------------------------------------------------------------------------------------------

class Block
{
private:
    friend class Chunk;
    friend class World;
    friend class BlockIterator;

    Block();
    ~Block() = default;
public:
    bool IsBlockVisible() const;
    bool IsBlockOpaque() const;
    bool IsBlockAir() const;
    bool IsBlockWater() const;

    AABB2 GetBlockTopSpriteUVs() const;
    AABB2 GetBlockBottomSpriteUVs() const;
    AABB2 GetBlockSideSpriteUVs() const;
    
    // Light Influence
    uint8_t  GetOutdoorLightInfluence() {return (m_lightInfluence & OUTDOOR_LIGHT_INFLUENCE_MASK) >> INDOOR_LIGHT_INFLUENCE_BITS;}
    uint8_t  GetIndoorLightInfluence() {return (m_lightInfluence & INDOOR_LIGHT_INFLUENCE_MASK);}

    void     SetOutdoorLightInfluence(uint8_t outdoorLightInfluence)
    {
        m_lightInfluence = (m_lightInfluence & INDOOR_LIGHT_INFLUENCE_MASK) | (outdoorLightInfluence << INDOOR_LIGHT_INFLUENCE_BITS);
    }

    void     SetIndoorLightInfluence(uint8_t indoorLightInfluence)
    {
        m_lightInfluence = (m_lightInfluence & OUTDOOR_LIGHT_INFLUENCE_MASK) | indoorLightInfluence;
    }

    // BitFlags
    bool IsSky() {return m_bitflags & BLOCK_BIT_FLAG_IS_SKY;}
    bool IsLightDirty() {return m_bitflags & BLOCK_BIT_FLAG_IS_LIGHT_DIRTY;}
    bool IsLightOpaque(){ return m_bitflags & BLOCK_BIT_FLAG_IS_LIGHT_OPAQUE;}
    bool IsSolid(){ return m_bitflags & BLOCK_BIT_FLAG_IS_SOLID;}
    bool IsVisible(){ return m_bitflags & BLOCK_BIT_FLAG_IS_VISIBLE; }

    void SetIsSky(bool isSky) 
    {
        m_bitflags = (m_bitflags & ~BLOCK_BIT_FLAG_IS_SKY) | (isSky ? BLOCK_BIT_FLAG_IS_SKY : 0);
    }

    void SetIsLightDirty(bool isLightDirty)
    {
        m_bitflags = (m_bitflags & ~BLOCK_BIT_FLAG_IS_LIGHT_DIRTY) | (isLightDirty ? BLOCK_BIT_FLAG_IS_LIGHT_DIRTY : 0);
    }

    void SetIsLightOpaque(bool isLightOpaque)
    {
        m_bitflags = (m_bitflags & ~BLOCK_BIT_FLAG_IS_LIGHT_OPAQUE) | (isLightOpaque ? BLOCK_BIT_FLAG_IS_LIGHT_OPAQUE : 0);
    }

    void SetIsSolid(bool isSolid)
    {
        m_bitflags = (m_bitflags & ~BLOCK_BIT_FLAG_IS_SOLID) | (isSolid ? BLOCK_BIT_FLAG_IS_SOLID : 0);
    }

    void SetIsVisible(bool isVisible)
    {
        m_bitflags = (m_bitflags & ~BLOCK_BIT_FLAG_IS_VISIBLE) | (isVisible ? BLOCK_BIT_FLAG_IS_VISIBLE : 0);
    }

private:
    unsigned char   m_blockDefinitionIndex  = 0; 
    
    // 1111 1111
    // Out  In   
    uint8_t         m_lightInfluence        = 0;    
    uint8_t         m_bitflags              = 0u;
};
