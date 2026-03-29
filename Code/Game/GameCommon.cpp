#include "Game/GameCommon.hpp"
#include "Game/BlockIterator.hpp"

float g_octavePersistence       = DEFAULT_OCTAVE_PERSISTENCE;
float g_octaveScale             = DEFAULT_NOISE_OCTAVE_SCALE;

float g_densityNoiseScale           = DENSITY_NOISE_SCALE;
float g_densityNoiseOctaves         = DENSITY_NOISE_OCTAVES;
float g_densityBiasPerBlockDelta    = DENSITY_BIAS_PER_BLOCK_DELTA;
float g_densityTerrainHeight        = DEFAULT_TERRAIN_HEIGHT;

Vec2  g_fogRange = Vec2(FOG_RANGE_NEAR, FOG_RANGE_FAR);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float g_continentalnessNoiseScale   = CONTINENTALNESS_NOISE_SCALE;
float g_continentalnessNoiseOctaves = CONTINENTALNESS_NOISE_OCTAVES;

//LinearCurve1D g_deepOceanCurve  = LinearCurve1D(-1.f, -0.4f);
//LinearCurve1D g_oceanCurve      = LinearCurve1D(-0.4f, 0.f);
//LinearCurve1D g_coastCurve      = LinearCurve1D(0.f, 0.1f);
//LinearCurve1D g_nearInlandCurve = LinearCurve1D(0.1f, 0.4f);
//LinearCurve1D g_midInlandCurve  = LinearCurve1D(0.4f, 0.8f);
//LinearCurve1D g_farInlandCurve  = LinearCurve1D(0.8f, 1.f);
//
//float g_deepOceanCurveStartTime = -1.f;
//float g_oceanCurveStartTime = -0.7f;
//float g_coastCurveStartTime = -0.35f;
//float g_nearInlandStartTime = 0.0f;
//float g_midInlandStartTime = 0.45f;
//float g_farInlandStartTime = 0.8f;

LinearCurve1D   g_deepOceanCurve            = LinearCurve1D(-1.f, -1.f);
LinearCurve1D   g_oceanCurve                = LinearCurve1D(-0.6f, -0.4f);
LinearCurve1D   g_coastCurve                = LinearCurve1D(-0.1f, -0.1f);
LinearCurve1D   g_nearInlandCurve           = LinearCurve1D(0.2f, 0.2f);
LinearCurve1D   g_midInlandCurve            = LinearCurve1D(0.4f, 0.4f);
LinearCurve1D   g_farInlandCurve            = LinearCurve1D(0.8f, 0.7f);
LinearCurve1D   g_maxContinentalnessCurve   = LinearCurve1D(1.f, 1.f);
PieceWiseCurves g_continentalnessCurve      = PieceWiseCurves();

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// LinearCurve1D    g_erosionL0 = LinearCurve1D(0.f, 0.1f); // Low erosion -- Mountains and peaks
// LinearCurve1D    g_erosionL1 = LinearCurve1D(0.1f, 0.3f);
// LinearCurve1D    g_erosionL2 = LinearCurve1D(0.3f, 0.45f);
// LinearCurve1D    g_erosionL3 = LinearCurve1D(0.45f, 0.65f);
// LinearCurve1D    g_erosionL4 = LinearCurve1D(0.65f, 0.75f);
// LinearCurve1D    g_erosionL5 = LinearCurve1D(0.75f, 0.9f);
// LinearCurve1D    g_erosionL6 = LinearCurve1D(0.9f, 1.f); // High Erosion
// PieceWiseCurves  g_erosionCurve = PieceWiseCurves();
// 
// float g_erosionL0StartTime = -1.f;
// float g_erosionL1StartTime = -0.4f;
// float g_erosionL2StartTime = -0.1f;
// float g_erosionL3StartTime = 0.f;
// float g_erosionL4StartTime = 0.4f;
// float g_erosionL5StartTime = 0.65f;
// float g_erosionL6StartTime = 0.8f;


LinearCurve1D g_erosionL0 = LinearCurve1D(-1.0f, -0.7f);   // very low erosion
LinearCurve1D g_erosionL1 = LinearCurve1D(-0.7f, -0.3f);   // low erosion
LinearCurve1D g_erosionL2 = LinearCurve1D(-0.3f, 0.0f);   // mild erosion
LinearCurve1D g_erosionL3 = LinearCurve1D(0.0f, 0.35f);  // moderate erosion
LinearCurve1D g_erosionL4 = LinearCurve1D(0.35f, 0.6f);   // strong erosion
LinearCurve1D g_erosionL5 = LinearCurve1D(0.6f, 0.85f);  // very strong erosion
LinearCurve1D g_erosionL6 = LinearCurve1D(0.85f, 1.0f);   // extreme erosion
PieceWiseCurves  g_erosionCurve = PieceWiseCurves();

float g_erosionNoiseScale   = 256.f;
float g_erosionNoiseOctaves = 4.f;
float g_erosionDefaultTerrainHeight = DEFAULT_TERRAIN_HEIGHT / 4.f;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
LinearCurve1D g_weirdnessValleys    = LinearCurve1D(-1.f, -1.f);
LinearCurve1D g_weirdnessLow        = LinearCurve1D(-0.6f, -0.4f);
LinearCurve1D g_weirdnessMid        = LinearCurve1D(0.f, 0.f);
LinearCurve1D g_weirdnessHigh       = LinearCurve1D(0.4f, 0.3f);
LinearCurve1D g_weirdnessPeaks      = LinearCurve1D(0.7f, 0.65f);
LinearCurve1D g_weirdnessMax        = LinearCurve1D(1.f, 1.f);

PieceWiseCurves  g_weirdnessCurve = PieceWiseCurves();

float g_weirdnessNoiseScale = 256.f;
float g_weirdnessNoiseOctaves = 8.f;

// float g_weirdnessValleyStartTime    = -1.f;
// float g_weirdnessLowStartTime       = -0.6f;
// float g_weirdnessMidStartTime       = -0.3f;
// float g_weirdnessHighStartTime      = 0.6f;
// float g_weirdnessPeakStartTime      = 0.9f;


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool g_computeDensity                       = true;
bool g_computeContinentalness               = true;
bool g_computeErosion                       = true;
bool g_computePeaksAndValley                = true;
bool g_lockTerrainGenerationToOrigin        = false;
bool g_enableSurfaceBlockReplacement        = true;
bool g_enableLighting                       = true;
bool g_enableDiggingAndPlacing              = true;
bool g_enableCrossChunkHiddenSurfaceRemoval = true;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
FloatRange g_t0Range = FloatRange(-1.f, -0.45f);
FloatRange g_t1Range = FloatRange(-0.45f, -0.15f);
FloatRange g_t2Range = FloatRange(-0.15f, 0.2f);
FloatRange g_t3Range = FloatRange(0.2f, 0.55f);
FloatRange g_t4Range = FloatRange(0.55f, 1.f);

FloatRange g_h0Range = FloatRange(-1.f, -0.35f);
FloatRange g_h1Range = FloatRange(-0.35f, -0.1f);
FloatRange g_h2Range = FloatRange(-0.1f, 0.1f);
FloatRange g_h3Range = FloatRange(0.1f, 0.3f);
FloatRange g_h4Range = FloatRange(0.3f, 1.f);

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float g_nonInlandBiomeMaxContinentalness = g_oceanCurve.m_end;

//------------------------------------------------------------------------------------------------------------------
uint32_t LocalCoordsToIndex(IntVec3 const& localCoords)
{
	return static_cast<uint32_t>(localCoords.x + (localCoords.y << CHUNK_BITS_X) + (localCoords.z << (CHUNK_BITS_X + CHUNK_BITS_Y)));
}

//------------------------------------------------------------------------------------------------------------------
uint32_t GlobalCoordsToIndex(IntVec3 const& globalCoords)
{
	IntVec3 localCoords = GlobalCoordsToLocalCoords(globalCoords);

	return LocalCoordsToIndex(localCoords);
}

//------------------------------------------------------------------------------------------------------------------
uint32_t LocalCoordsToIndex(uint32_t x, uint32_t y, uint32_t z)
{
	return x + (y << CHUNK_BITS_X) + (z << (CHUNK_BITS_X + CHUNK_BITS_Y));
}

//------------------------------------------------------------------------------------------------------------------
uint32_t GlobalCoordsToIndex(int x, int y, int z)
{
    IntVec3 localCoords = GlobalCoordsToLocalCoords(IntVec3(x, y, z));

    return LocalCoordsToIndex(localCoords);
}

//------------------------------------------------------------------------------------------------------------------
IntVec3 IndexToLocalCoords(uint32_t index)
{
    int x = index & CHUNK_MASK_X;
    int y = (index & CHUNK_MASK_Y) >> CHUNK_BITS_X;
    int z = (index & CHUNK_MASK_Z) >> (CHUNK_BITS_Y + CHUNK_BITS_X);
    
    return IntVec3(x, y, z);
}

//------------------------------------------------------------------------------------------------------------------
uint32_t IndexToLocalCoordsX(uint32_t index)
{
    return index & CHUNK_MASK_X;
}

//------------------------------------------------------------------------------------------------------------------
uint32_t IndexToLocalCoordsY(uint32_t index)
{
    return (index & CHUNK_MASK_Y) >> CHUNK_BITS_X;
}

//------------------------------------------------------------------------------------------------------------------
uint32_t IndexToLocalCoordsZ(uint32_t index)
{
    return (index & CHUNK_MASK_Z) >> (CHUNK_BITS_Y + CHUNK_BITS_X);
}

//------------------------------------------------------------------------------------------------------------------
IntVec2 GetChunkCoords(IntVec3 const& globalCoords)
{
	int x = RoundDownToInt(static_cast<float>(globalCoords.x) / CHUNK_SIZE_X);
	int y = RoundDownToInt(static_cast<float>(globalCoords.y) / CHUNK_SIZE_Y);

    return IntVec2(x, y);
}

//------------------------------------------------------------------------------------------------------------------
IntVec2 GetChunkCenter(IntVec2 const& chunkCoords)
{
   int globalXCoords = chunkCoords.x * CHUNK_SIZE_X;
   int globalYCoords = chunkCoords.y * CHUNK_SIZE_Y;
   
   int x = globalXCoords + (CHUNK_SIZE_X >> 1);
   int y = globalYCoords + (CHUNK_SIZE_Y >> 1);
   
   return IntVec2(x, y);
}

//------------------------------------------------------------------------------------------------------------------
IntVec3 GetGlobalCoords(IntVec2 const& chunkCoords, uint32_t blockIndex)
{
    IntVec3 globalCoords    = IntVec3(0, 0, 0);
    IntVec3 localCoords     = IndexToLocalCoords(blockIndex);
    
    globalCoords.x = (chunkCoords.x * CHUNK_SIZE_X) + localCoords.x;
    globalCoords.y = (chunkCoords.y * CHUNK_SIZE_Y) + localCoords.y;
    globalCoords.z = localCoords.z;

    return globalCoords;
}

//------------------------------------------------------------------------------------------------------------------
IntVec3 GetGlobalCoords(IntVec2 const& chunkCoords, IntVec3 const& localCoords)
{
	IntVec3 globalCoords = IntVec3(0, 0, 0);

	globalCoords.x = (chunkCoords.x * CHUNK_SIZE_X) + localCoords.x;
	globalCoords.y = (chunkCoords.y * CHUNK_SIZE_Y) + localCoords.y;
	globalCoords.z = localCoords.z;

    return globalCoords;
}

//------------------------------------------------------------------------------------------------------------------
IntVec3 GetGlobalCoords(Vec3 const& position)
{
    return IntVec3(RoundDownToInt(position.x), RoundDownToInt(position.y), RoundDownToInt(position.z));
}

//------------------------------------------------------------------------------------------------------------------
IntVec3 GlobalCoordsToLocalCoords(IntVec3 const& globalCoords)
{
	IntVec3 localCoords = IntVec3(0, 0, 0);
	localCoords.x = globalCoords.x % CHUNK_SIZE_X;
	localCoords.y = globalCoords.y % CHUNK_SIZE_Y;
	localCoords.z = globalCoords.z;

	if(localCoords.x < 0)
    {
        localCoords.x += CHUNK_SIZE_X;
    }

	if(localCoords.y < 0) 
    {
        localCoords.y += CHUNK_SIZE_Y;
	}

	return localCoords;
}

//------------------------------------------------------------------------------------------------------------------
bool IsChunkWithinActivationRange(IntVec2 const& chunkCoords, IntVec2 const& playerChunkCoords)
{
    IntVec2 chunkCenter         = GetChunkCenter(chunkCoords);
    IntVec2 playerChunkCenter   = GetChunkCenter(playerChunkCoords);

    if (GetVectorDistanceSquared2D(chunkCenter, playerChunkCenter) <= CHUNK_ACTIVATION_RANGE * CHUNK_ACTIVATION_RANGE)
    {
        return true;
    }
    
    return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool IsChunkOutsideDeactivationRange(IntVec2 const& chunkCoords, IntVec2 const& playerChunkCoords)
{
	IntVec2 chunkCenter         = GetChunkCenter(chunkCoords);
	IntVec2 playerChunkCenter   = GetChunkCenter(playerChunkCoords);

	if(GetVectorDistanceSquared2D(chunkCenter, playerChunkCenter) > CHUNK_DEACTIVATION_RANGE * CHUNK_DEACTIVATION_RANGE)
	{
		return true;
	}

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool IsValidLocalCoord(IntVec3 const& localBlockCoord)
{
    if(localBlockCoord.x < 0 || localBlockCoord.y < 0 || localBlockCoord.z < 0
       || localBlockCoord.x > CHUNK_MAX_X || localBlockCoord.y > CHUNK_MAX_Y || localBlockCoord.z > CHUNK_MAX_Z)
    {
        return false;
    }

    return true;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool IsValidIterator(BlockIterator const& iter)
{
    return iter.m_chunk && iter.m_blockIndex < BLOCKS_PER_CHUNK;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char GetColorChannelForLightInfluence(uint8_t influence)
{
    float rangeMappedValue = RangeMapClamped(static_cast<float>(influence), 0.f, 15.f, 0.f, 1.f);

    return DenormalizeByte(rangeMappedValue);
}
