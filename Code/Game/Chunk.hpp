#pragma once

#include "Game/GameCommon.hpp"
#include "Game/Block.hpp"

#include "Engine/Math/CurveUtils.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Math/IntVec2.hpp"

#include <atomic>
#include <deque>

//------------------------------------------------------------------------------------------------------------------
class Block;
class IndexBuffer;
class VertexBuffer;
class NamedStrings;
class BlockIterator;

struct Vertex_PCUTBN;

typedef NamedStrings EventArgs;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum Neighbours
{
    NEIGHBOUR_POS_X,
    NEIGHBOUR_POS_Y,
    NEIGHBOUR_NEG_X,
    NEIGHBOUR_NEG_Y,
    NEIGHBOUR_COUNT
};

enum class ChunkState
{
    INACTIVE,           // When chunk is outside of activation range

    PENDING_LOAD,       // Chunk is in activation range -- save file exists
    PENDING_GENERATE,   // Chunk is in activation range -- save file doesn't exist
    PENDING_SAVE,       // Deactivated -- Main

    LOADING,            // Being loaded from save file -- By Load Chunk Job
    GENERATING,         // Being generated -- By GenerateChunkJob
    SAVING,             // Being saved -- SaveChunkJob
    
    LOADED,             // Loaded, not in active map
    GENERATED,          // Generated, not in active map
    SAVED,               
    
    ACTIVE_DIRTY,
    ACTIVE             // Generated / Loaded -- By Main (?)
    
};

//------------------------------------------------------------------------------------------------------------------
class Chunk
{
    friend class World;
    friend class GenerateChunkJob;
    friend class LoadChunkJob;
    friend class SaveChunkJob;

private:
    Chunk();
    virtual ~Chunk();
    Chunk(Chunk const&) = delete;

    explicit                    Chunk(IntVec2 const& chunkCoordinates);
                                
    void                        Update();
    void                        Render();

    void                        InitializeBlocks();

    void                        GenerateTerrain(std::vector<float>& densityMap, std::vector<float>& continentalnessMap, std::vector<float>& erosionMap, 
                                                        std::vector<float>& peaksAndValleysMap, std::vector<float>& temperatureMap, 
                                                        std::vector<float>& humidityMap, std::vector<int>& heightMap, std::vector<int>& dirtDepthMap, 
                                                        std::vector<int>& lavaDepthMap, std::vector<int>& obsidianDepthMap);

    void                        GenerateTerrain_New(std::vector<float>& densityMap, std::vector<float>& continentalnessMap, std::vector<float>& erosionMap, 
                                                        std::vector<float>& peaksAndValleysMap, std::vector<float>& temperatureMap, 
                                                        std::vector<float>& humidityMap, std::vector<int>& heightMap, std::vector<int>& dirtDepthMap, 
                                                        std::vector<int>& lavaDepthMap, std::vector<int>& obsidianDepthMap);

    void                        SurfaceBlockReplacement(std::vector<float>& densityMap, std::vector<float>& continentalnessMap, std::vector<float>& erosionMap,
                                                        std::vector<float>& peaksAndValleysMap, std::vector<float>& temperatureMap,
                                                        std::vector<float>& humidityMap, std::vector<int>& heightMap, std::vector<int>& dirtDepthMap, 
                                                        std::vector<int>& lavaDepthMap, std::vector<int>& obsidianDepthMap);

    float                       ComputeTerrainHeightOffset(float blockGlobalX, float blockGlobalY, std::vector<float>& continentalnessMap, 
                                                            std::vector<float>& erosionMap, std::vector<float>& peaksAndValleysMap, int colIndex);

    void                        BuildChunkMesh();
    Rgba8                       CalculateVertexLighting(Block* sideA, Block* sideB, Block* corner, Block* center, Rgba8 faceColor);

    int                         GetTerrainHeightForBlockColumn(int globalBlockColumnXCoord, int globalBlockColumnYCoord) const;
    int                         GetBlockIndex(IntVec3 const& localBlockCoords) const;
    Block                       GenerateBlock(int terrainHeight, int currentZ, int numDirtBlocks) const;
    Block                       GenerateBlockOfType(std::string const& blockType) const;   

    void                        AddNeighbour(Chunk* chunk, Neighbours direction);
    void                        RemoveNeighbors();
    void                        RemoveNeighbor(Chunk* chunk);

    std::string                 GetBlockTypeForNonInlandBiomeBlock(BiomeGenParams const& params);
    std::string                 GetBlockTypeForDeepOceanBiomeBlock(BiomeGenParams const& params);
    std::string                 GetBlockTypeForOceanBiomeBlock(BiomeGenParams const& params);
    std::string                 GetBlockTypeForFrozenOceanBiomeBlock(BiomeGenParams const& params);


    std::string                 GetBlockTypeForInlandBiomeBlock(BiomeGenParams const& params);
    
    std::string                 GetBlockTypeForCoastBiomeBlock(BiomeGenParams const& params);
    std::string                 GetBlockTypeForNearInlandBiomeBlock(BiomeGenParams const& params);
    std::string                 GetBlockTypeForMidInlandBiomeBlock(BiomeGenParams const& params);
    std::string                 GetBlockTypeForFarInlandBiomeBlock(BiomeGenParams const& params);

    std::string                 GetBlockTypeForBeachBiomes(BiomeGenParams const& params);
    std::string                 GetBlockTypeForSnowyBeach(BiomeGenParams const& params);
    std::string                 GetBlockTypeForNormalBeachBiome(BiomeGenParams const& params);
    std::string                 GetBlockTypeForDesertBiome(BiomeGenParams const& params);

    std::string                 GetBlockTypeForMiddleBiomes(BiomeGenParams const& params);
    std::string                 GetBlockTypeForSnowyPlains(BiomeGenParams const& params);
    std::string                 GetBlockTypeForSnowyTaiga(BiomeGenParams const& params);
    std::string                 GetBlockTypeForTaiga(BiomeGenParams const& params);
    std::string                 GetBlockTypeForPlains(BiomeGenParams const& params);
    std::string                 GetBlockTypeForForest(BiomeGenParams const& params);
    std::string                 GetBlockTypeForJungle(BiomeGenParams const& params);
    std::string                 GetBlockTypeForSavanna(BiomeGenParams const& params);
    std::string                 GetBlockTypeForBadlandBiomes(BiomeGenParams const& params);
    std::string                 GetBlockTypeForStoneyPeaks(BiomeGenParams const& params);
    std::string                 GetBlockTypeForSnowyPeaks(BiomeGenParams const& params);

    InlandBiomeType             GetInlandBiomeTypeForContinentalness(float continentalness);
    PeaksAndValleysLevel        GetPeaksAndValleysLevel(float pvValue);
	ErosionLevel                GetErosionLevel(float erosionValue);
	Continentalness             GetContinentalnessLevel(float value);
	
    void                        MarkNeighborsDirty(IntVec3 const& removedBlockCoord);
    bool                        AreAllNeighborsValid();
    void                        SaveToFile();
    bool                        TryLoadFromFile();
    inline bool                 IsValidNeighbour(Neighbours neighbourDirection);

    void                        RemoveBlock(uint32_t blockIndex);
    void                        PlaceBlockOfType(uint32_t blockIndex, unsigned char blockType);
    void                        InitializeLighting(std::deque<BlockIterator>& dirtyLightingBlocks);
    void                        MarkNonSkyHorizontalBlockLightDirty(IntVec3 const& localCoords);

    void                        RecomputeLightingForBlockRemoved(uint32_t blockIndex);
    void                        RecomputeLightingForBlockAdded(uint32_t blockIndex, bool didReplaceSky);

public:
    AABB3                       GetChunkBounds() const;
    IntVec2                     GetChunkCoordinates() const;
    
    bool                        IsBlockOpaque(uint32_t blockIndex) const;
    bool                        IsBlockWater(uint32_t blockIndex) const;
    bool                        IsBlockAir(uint32_t blockIndex) const;

    Block                       GetBlockAtCoord(IntVec3 const& localCoord) const;
    Chunk*                      GetNeighborChunk(Neighbours direction) const;
    Block&                      GetBlock(uint32_t blockIndex);

private:
    Block                       m_blocks[BLOCKS_PER_CHUNK]  = {};
    IntVec2                     m_chunkCoords               = IntVec2();
    AABB3                       m_chunkBounds               = AABB3();

//     bool                        m_needsRegeneration         = true;
    bool                        m_isModified               = false;

    std::vector<Vertex_PCUTBN>  m_chunkVerts;
    std::vector<unsigned int>   m_indices;
    VertexBuffer*               m_vertexBuffer          = nullptr;
    IndexBuffer*                m_indexBuffer           = nullptr;
    
    std::vector<Vertex_PCUTBN>  m_debugChunkVerts;
    std::vector<unsigned int>   m_debugIndices;
    VertexBuffer*               m_debugVertexBuffer     = nullptr;
    IndexBuffer*                m_debugIndexBuffer      = nullptr;
    
    Chunk*                      m_neighbours[NEIGHBOUR_COUNT] = {};
    std::atomic<ChunkState>     m_chunkState            = ChunkState::INACTIVE;
};
