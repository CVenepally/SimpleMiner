#pragma once

#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/NamedStrings.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/CurveUtils.hpp"
#include "Engine/Math/Vec4.hpp"
#include "Engine/Math/Vec2.hpp"

#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/JobSystem/JobSystem.hpp"

#include <cstdint>

#include "Engine/Math/IntVec3.hpp"
//------------------------------------------------------------------------------------------------------------------
class App;

extern App*         g_theApp;
extern Renderer*    g_theRenderer;
extern InputSystem* g_inputSystem;
extern AudioSystem* g_theAudioSystem;
extern Window*      g_theWindow;
extern JobSystem*   g_jobSystem;
extern BitmapFont*  g_gameFont;

class  BlockIterator;

extern bool g_computeDensity;
extern bool g_computeContinentalness;
extern bool g_computeErosion;
extern bool g_computePeaksAndValley;
extern bool g_lockTerrainGenerationToOrigin;
extern bool g_enableSurfaceBlockReplacement;
extern bool g_enableLighting;
extern bool g_enableDiggingAndPlacing;
extern bool g_enableCrossChunkHiddenSurfaceRemoval;

//Chunk Variables---------------------------------------------------------------------------------------------------
#ifdef ENGINE_DEBUG_RENDER
constexpr int CHUNK_BITS_X = 4;
constexpr int CHUNK_BITS_Y = 4;
constexpr int CHUNK_BITS_Z = 6;
#else
constexpr int CHUNK_BITS_X = 4;
constexpr int CHUNK_BITS_Y = 4;
constexpr int CHUNK_BITS_Z = 8;
#endif // ENGINE_DEBUG_RENDER

constexpr int CHUNK_SIZE_X = 1 << CHUNK_BITS_X;
constexpr int CHUNK_SIZE_Y = 1 << CHUNK_BITS_Y;
constexpr int CHUNK_SIZE_Z = 1 << CHUNK_BITS_Z;

constexpr int CHUNK_MAX_X = CHUNK_SIZE_X - 1;
constexpr int CHUNK_MAX_Y = CHUNK_SIZE_Y - 1;
constexpr int CHUNK_MAX_Z = CHUNK_SIZE_Z - 1;

constexpr int CHUNK_MASK_X = CHUNK_MAX_X;
constexpr int CHUNK_MASK_Y = CHUNK_MAX_Y << CHUNK_BITS_X;
constexpr int CHUNK_MASK_Z = CHUNK_MAX_Z << (CHUNK_BITS_X + CHUNK_BITS_Y);

constexpr int BLOCKS_PER_CHUNK = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

#ifdef ENGINE_DEBUG_RENDER
    constexpr int CHUNK_ACTIVATION_RANGE = 40;
	constexpr int CHUNK_DEACTIVATION_RANGE = CHUNK_ACTIVATION_RANGE + CHUNK_SIZE_X + CHUNK_SIZE_Y;
	constexpr int CHUNK_ACTIVATION_RADIUS_X = 1 + (CHUNK_ACTIVATION_RANGE >> CHUNK_BITS_X);
	constexpr int CHUNK_ACTIVATION_RADIUS_Y = 1 + (CHUNK_ACTIVATION_RANGE >> CHUNK_BITS_Y);
	constexpr int MAX_ACTIVE_CHUNKS = (CHUNK_ACTIVATION_RADIUS_X << 1) * (CHUNK_ACTIVATION_RADIUS_Y << 1);
#else

    constexpr int CHUNK_ACTIVATION_RANGE = 320;
    constexpr int CHUNK_DEACTIVATION_RANGE = CHUNK_ACTIVATION_RANGE + CHUNK_SIZE_X + CHUNK_SIZE_Y;
    constexpr int CHUNK_ACTIVATION_RADIUS_X = 1 + (CHUNK_ACTIVATION_RANGE >> CHUNK_BITS_X);
    constexpr int CHUNK_ACTIVATION_RADIUS_Y = 1 + (CHUNK_ACTIVATION_RANGE >> CHUNK_BITS_Y);
    constexpr int MAX_ACTIVE_CHUNKS = (CHUNK_ACTIVATION_RADIUS_X << 1) * (CHUNK_ACTIVATION_RADIUS_Y << 1);
#endif // ENGINE_DEBUG_RENDER


//Thread Variables--------------------------------------------------------------------------------------------------
#ifdef ENGINE_DEBUG_RENDER
	constexpr int MAX_GENERATE_JOBS = 10;
	constexpr int MAX_SAVE_JOBS = 6;
	constexpr int MAX_LOAD_JOBS = 6;
	constexpr int MAX_CHUNK_MESHES_TO_BUILD = 1;
#else
	constexpr int MAX_GENERATE_JOBS = 256;
	constexpr int MAX_SAVE_JOBS = 6;
	constexpr int MAX_LOAD_JOBS = 6;
	constexpr int MAX_CHUNK_MESHES_TO_BUILD = 1;
#endif // ENGINE_DEBUG_RENDER

// Game Variables---------------------------------------------------------------------------------------------------
constexpr unsigned int  GAME_SEED                   = 0u;
constexpr unsigned int  CONTINENTALNESS_SEED        = GAME_SEED + 1;
constexpr unsigned int  EROSION_SEED                = CONTINENTALNESS_SEED + 1;
constexpr unsigned int  WEIRDNESS_SEED              = EROSION_SEED + 1;
constexpr unsigned int  TEMPERATURE_SEED            = WEIRDNESS_SEED + 1;
constexpr unsigned int  HUMIDITY_SEED               = TEMPERATURE_SEED + 1;
constexpr unsigned int  DENSITY_SEED                = HUMIDITY_SEED + 1;
constexpr unsigned int  DIRT_DEPTH_SEED             = DENSITY_SEED + 1;
constexpr unsigned int  LAVA_DEPTH_SEED             = DIRT_DEPTH_SEED + 1;
constexpr unsigned int  OBSIDIAN_DEPTH_SEED         = LAVA_DEPTH_SEED + 1;

constexpr float         DEFAULT_OCTAVE_PERSISTENCE          = 0.5f;
constexpr float         DEFAULT_NOISE_OCTAVE_SCALE          = 2.f;
constexpr float         DEFAULT_TERRAIN_HEIGHT              = CHUNK_SIZE_Z >> 1;
constexpr float         RIVER_DEPTH                         = 8.f;

constexpr float         TERRAIN_NOISE_SCALE         = 200.f;
constexpr unsigned int  TERRAIN_NOISE_OCTAVES       = 5u;

constexpr float         HUMIDITY_NOISE_SCALE        = 800.f;
constexpr unsigned int  HUMIDITY_NOISE_OCTAVES      = 1u;

constexpr float         TEMPERATURE_RAW_NOISE_SCALE = 0.0075f;
constexpr float         TEMPERATURE_NOISE_SCALE     = 400.f;
constexpr unsigned int  TEMPERATURE_NOISE_OCTAVES   = 4u;

constexpr float         HILLINESS_NOISE_SCALE       = 250.f;
constexpr unsigned int  HILLINESS_NOISE_OCTAVES     = 4u;

constexpr float         OCEAN_START_THRESHOLD       = 0.f;
constexpr float         OCEAN_END_THRESHOLD         = 0.5f;
constexpr float         OCEAN_DEPTH                 = 30.f;

constexpr float         OCEANESS_NOISE_SCALE        = 600.f;
constexpr unsigned int  OCEANESS_NOISE_OCTAVES      = 3u;

constexpr int           MIN_DIRT_OFFSET_Z           = 3;
constexpr int           MAX_DIRT_OFFSET_Z           = 4;

constexpr float         MIN_SAND_HUMIDITY           = 0.4f;
constexpr float         MAX_SAND_HUMIDITY           = 0.7f;
constexpr int           SEA_LEVEL_Z                 = CHUNK_SIZE_Z >> 1;

constexpr float         ICE_TEMPERATURE_MAX         = 0.37f;
constexpr float         ICE_TEMPERATURE_MIN         = 0.0f;
constexpr float         ICE_DEPTH_MIN               = 0.0f;
constexpr float         ICE_DEPTH_MAX               = 8.0f;
                        
constexpr float         MIN_SAND_DEPTH_HUMIDITY     = 0.4f;
constexpr float         MAX_SAND_DEPTH_HUMIDITY     = 0.0f;
constexpr float         SAND_DEPTH_MIN              = 0.0f;
constexpr float         SAND_DEPTH_MAX              = 6.0f;
                        
constexpr float         COAL_CHANCE                 = 0.05f;
constexpr float         IRON_CHANCE                 = 0.02f;
constexpr float         GOLD_CHANCE                 = 0.005f;
constexpr float         DIAMOND_CHANCE              = 0.0001f;

constexpr int           MIN_LAVA_BLOCKS             = 3;
constexpr int           MAX_LAVA_BLOCKS             = 4;

constexpr int           MIN_OBSIDIAN_BLOCKS         = 3;
constexpr int           MAX_OBSIDIAN_BLOCKS         = 4;

constexpr float         DENSITY_NOISE_SCALE          = 64.f;
constexpr float         DENSITY_NOISE_OCTAVES        = 4.f;
constexpr float         DENSITY_BIAS_PER_BLOCK_DELTA = 0.012f;

constexpr float         CONTINENTALNESS_NOISE_SCALE   = 512.f;
constexpr float         CONTINENTALNESS_NOISE_OCTAVES = 4.f;

#ifdef ENGINE_DEBUG_RENDER
constexpr float         FOG_RANGE_FAR   = 100;
constexpr float         FOG_RANGE_NEAR  = 100;
#else
// constexpr float         FOG_RANGE_FAR = 1000.f;
// constexpr float         FOG_RANGE_NEAR = 1000.f;
constexpr float         FOG_RANGE_FAR = CHUNK_ACTIVATION_RANGE - (2 * CHUNK_SIZE_X);
constexpr float         FOG_RANGE_NEAR = FOG_RANGE_FAR / 3.f;
#endif
extern Vec2             g_fogRange;

constexpr float         RAY_MAX_LENGTH = 8.f;

// Time of Day
constexpr float GAME_SECONDS_PER_REAL_TIME_SECONDS      = 500.f;
constexpr float TOTAL_GAME_SECONDS_PER_GAME_DAY         = 24.f * 60.f * 60.f;
constexpr float TOTAL_REAL_TIME_SECONDS_PER_GAME_DAY    = TOTAL_GAME_SECONDS_PER_GAME_DAY / GAME_SECONDS_PER_REAL_TIME_SECONDS;

constexpr float BASE_MIDNIGHT_FRACTION  = 0.f;
constexpr float BASE_DAWN_FRACTION      = 0.25f;
constexpr float BASE_NOON_FRACTION      = 0.45f;
constexpr float BASE_DUSK_FRACTION      = 0.75f;

constexpr float TIME_OF_DAY_MULTIPLIER  = 100.f;

Rgba8 const DUSK_TO_DAWN_COLOR = Rgba8(10, 10, 15, 255);
Rgba8 const NOON_COLOR = Rgba8(200, 230, 255, 255);

Rgba8 const OUT_DOOR_MIDNIGHT = Rgba8(15, 15, 25, 255);
Rgba8 const OUT_DOOR_NOON = Rgba8(255, 255, 255, 255);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Player
constexpr uint8_t TOTAL_SLOTS_HOTBAR    = 9u;
constexpr uint8_t TOTAL_SLOTS_OFFHAND   = 1u;
constexpr uint8_t TOTAL_STORAGE_SLOTS   = 27u;
constexpr uint8_t TOTAL_SLOTS_INVENTORY = TOTAL_SLOTS_HOTBAR + TOTAL_SLOTS_OFFHAND + TOTAL_STORAGE_SLOTS;

constexpr uint8_t SLOTS_MAX_STACK_SIZE  = 64u;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Height Offset Curves
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------

extern float g_octavePersistence;
extern float g_octaveScale;

// Density Noise Values
extern float g_densityNoiseScale;
extern float g_densityNoiseOctaves;
extern float g_densityBiasPerBlockDelta;
extern float g_densityTerrainHeight;

// Continentalness Curves
extern LinearCurve1D    g_deepOceanCurve;
extern LinearCurve1D    g_oceanCurve;
extern LinearCurve1D    g_coastCurve;
extern LinearCurve1D    g_nearInlandCurve;
extern LinearCurve1D    g_midInlandCurve;
extern LinearCurve1D    g_farInlandCurve;
extern LinearCurve1D    g_maxContinentalnessCurve;
extern PieceWiseCurves  g_continentalnessCurve;

extern float g_deepOceanCurveStartTime;
extern float g_oceanCurveStartTime;
extern float g_coastCurveStartTime;
extern float g_nearInlandStartTime;
extern float g_midInlandStartTime;
extern float g_farInlandStartTime;

extern float g_continentalnessNoiseScale;
extern float g_continentalnessNoiseOctaves;

// Erosion Curves
extern LinearCurve1D    g_erosionL0;
extern LinearCurve1D    g_erosionL1;
extern LinearCurve1D    g_erosionL2;
extern LinearCurve1D    g_erosionL3;
extern LinearCurve1D    g_erosionL4;
extern LinearCurve1D    g_erosionL5;
extern LinearCurve1D    g_erosionL6;
extern PieceWiseCurves  g_erosionCurve;

extern float g_erosionL0StartTime;
extern float g_erosionL1StartTime;
extern float g_erosionL2StartTime;
extern float g_erosionL3StartTime;
extern float g_erosionL4StartTime;
extern float g_erosionL5StartTime;
extern float g_erosionL6StartTime;

extern float g_erosionNoiseScale;
extern float g_erosionNoiseOctaves;
extern float g_erosionDefaultTerrainHeight;

// Weirdness (Peaks and Valleys) Curves
extern LinearCurve1D    g_weirdnessValleys;
extern LinearCurve1D    g_weirdnessLow;
extern LinearCurve1D    g_weirdnessMid;
extern LinearCurve1D    g_weirdnessHigh;
extern LinearCurve1D    g_weirdnessPeaks;
extern LinearCurve1D    g_weirdnessMax;

extern PieceWiseCurves  g_weirdnessCurve;

extern float g_weirdnessValleyStartTime;
extern float g_weirdnessLowStartTime;
extern float g_weirdnessMidStartTime;
extern float g_weirdnessHighStartTime;
extern float g_weirdnessPeakStartTime;

extern float g_weirdnessNoiseScale;
extern float g_weirdnessNoiseOctaves;
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern FloatRange g_t0Range;
extern FloatRange g_t1Range;
extern FloatRange g_t2Range;
extern FloatRange g_t3Range;
extern FloatRange g_t4Range;

extern FloatRange g_h0Range;
extern FloatRange g_h1Range;
extern FloatRange g_h2Range;
extern FloatRange g_h3Range;
extern FloatRange g_h4Range;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
constexpr float g_playerHorizontalGroundAcceleration = 64.0f;
constexpr float g_playerHorizontalAirAcceleration = 4.0f;
constexpr float g_playerHorizontalGroundDragCoefficient = 8.0f;
constexpr float g_playerHorizontalAirDragCoefficient = 4.0f;
constexpr float g_playerMaxHorizontalSpeed = 10.0f;
constexpr float g_playerMaxVerticalSpeed = 56.0f;
constexpr float g_playerGravityAcceleration = 9.8f;
constexpr float g_playerJumpImpulse = 6.0f;
constexpr float g_colliderPointNudgeFactor = 0.03f;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
extern float g_nonInlandBiomeMaxContinentalness;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum InlandBiomeType
{
    InlandBiomeType_Coast,
    InlandBiomeType_NearInland,
    InlandBiomeType_MidInland,
    InlandBiomeType_FarInland,

    InlandBiomeType_Count
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum PeaksAndValleysLevel
{
    PV_Valley,
    PV_Low,
    PV_Mid,
    PV_High,
    PV_Peak,
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum ErosionLevel 
{ 
    ErosionLevel_L0, 
    ErosionLevel_L1, 
    ErosionLevel_L2, 
    ErosionLevel_L3, 
    ErosionLevel_L4, 
    ErosionLevel_L5, 
    ErosionLevel_L6 
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum Continentalness
{
	Continentalness_DeepOcean,
	Continentalness_Ocean,
	Continentalness_Coast,
	Continentalness_NearInland,
	Continentalness_MidInland,
	Continentalness_FarInland,
    Continentalness_COUNT
};


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct BiomeGenParams
{
    float   m_continentalness;
    float   m_erosion;
    float   m_peaksAndValleys;
    float   m_temperature;
    float   m_humidity;
    float   m_density;
    int     m_height;
    int     m_dirtDepth;
    int     m_blockZ;

    std::string m_blockBeforeReplacement;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct WorldConstants
{
	Vec4 m_cameraPosition;
    float m_skyColor[4];
    float m_indoorLightColor[4];
    float m_outdoorLightColor[4];
	Vec2 m_distanceRange;
	Vec2 m_padding0;
};
constexpr int g_worldConstRegister = 0;

//------------------------------------------------------------------------------------------------------------------
enum Directions : uint8_t
{
    DIRECTION_NONE  = 0,
    DIRECTION_POS_X = 1 << 0,
    DIRECTION_NEG_X = 1 << 1,
    DIRECTION_POS_Y = 1 << 2,
    DIRECTION_NEG_Y = 1 << 3,
    DIRECTION_POS_Z = 1 << 4,
    DIRECTION_NEG_Z = 1 << 5,
    DIRECTION_ALL   = DIRECTION_POS_X | DIRECTION_NEG_X | DIRECTION_POS_Y | DIRECTION_NEG_Y | DIRECTION_POS_Z | DIRECTION_NEG_Z
};

inline Directions operator|(Directions a, Directions b)
{
    return static_cast<Directions>(static_cast<int>(a) | static_cast<int>(b));
}

inline Directions operator&(Directions a, Directions b)
{
    return static_cast<Directions>(static_cast<int>(a) & static_cast<int>(b));
}

inline Directions& operator|=(Directions& a, Directions b)
{
    a = a | b;
    return a;
}

inline Directions& operator&=(Directions& a, Directions b)
{
    a = a & b;
    return a;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
enum EntityType
{
	ENTITY_TYPE_NONE = -1,
	ENTITIY_TYPE_PLAYER = 0,
	ENTITY_TYPE_NPC,
	ENTITY_TYPE_COUNT,
};


//------------------------------------------------------------------------------------------------------------------
// Utility Functions

// Coords to Index
uint32_t    LocalCoordsToIndex(IntVec3 const& localCoords);
uint32_t    GlobalCoordsToIndex(IntVec3 const& globalCoords);

uint32_t    LocalCoordsToIndex(uint32_t x, uint32_t y, uint32_t z);
uint32_t    GlobalCoordsToIndex(int x, int y, int z);

// Index to coords
IntVec3     IndexToLocalCoords(uint32_t index);
uint32_t    IndexToLocalCoordsX(uint32_t index);
uint32_t    IndexToLocalCoordsY(uint32_t index);
uint32_t    IndexToLocalCoordsZ(uint32_t index);

// Chunk
IntVec2     GetChunkCoords(IntVec3 const& globalCoords);
IntVec2     GetChunkCenter(IntVec2 const& chunkCoords);

// Global Coords
IntVec3     GetGlobalCoords(IntVec2 const& chunkCoords, uint32_t blockIndex);
IntVec3     GetGlobalCoords(IntVec2 const& chunkCoords, IntVec3 const& localCoords);
IntVec3     GetGlobalCoords(Vec3 const& position);
IntVec3     GlobalCoordsToLocalCoords(IntVec3 const& globalCoords);

// Distance Checks
bool        IsChunkWithinActivationRange(IntVec2 const& chunkCoords, IntVec2 const& playerChunkCoords);
bool        IsChunkOutsideDeactivationRange(IntVec2 const& chunkCoords, IntVec2 const& playerChunkCoords);

// Coord Validity
bool        IsValidLocalCoord(IntVec3 const& localBlockCoord);

bool		IsValidIterator(BlockIterator const& iter);

unsigned char GetColorChannelForLightInfluence(uint8_t influence);
