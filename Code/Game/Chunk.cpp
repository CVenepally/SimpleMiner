#include "Game/Chunk.hpp"
#include "Game/Game.hpp"
#include "Game/World.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/BlockIterator.hpp"
#include "Game/GameCommon.hpp"
#include "Game/ChunkSaveFile.hpp"

#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/FileUtils.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/NamedStrings.hpp"

#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/IntRange.hpp"
#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/EasingFunctions.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"

#include "ThirdParty/Noise/RawNoise.hpp"
#include "ThirdParty/Noise/SmoothNoise.hpp"

//------------------------------------------------------------------------------------------------------------------
extern  SpriteSheet* g_blockSpriteSheet;
//------------------------------------------------------------------------------------------------------------------
Chunk::Chunk()
{
    m_chunkCoords = IntVec2::ZERO;
    InitializeBlocks();

}

//------------------------------------------------------------------------------------------------------------------
Chunk::Chunk(IntVec2 const& chunkCoordinates, bool hasSaveFile)
    : m_chunkCoords(chunkCoordinates)
{
	if(hasSaveFile)
	{
		m_chunkState = ChunkState::PENDING_LOAD;
	}
    else
    {
        m_chunkState = ChunkState::PENDING_GENERATE;
    }
}
//------------------------------------------------------------------------------------------------------------------
Chunk::~Chunk()
{
	delete m_vertexBuffer;
	delete m_indexBuffer;
}

//------------------------------------------------------------------------------------------------------------------
void Chunk::Update()
{
}

//------------------------------------------------------------------------------------------------------------------
void Chunk::Render()
{
    if(!m_vertexBuffer || !m_indexBuffer)
    {
        return;
    }
    // DebugAddWorldWireframeBox(m_chunkBounds.m_mins, m_chunkBounds.m_maxs, 0.f);

    g_theRenderer->SetModelConstants();
    g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
    g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
    g_theRenderer->SetBlendMode(BlendMode::ALPHA);
    g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
    g_theRenderer->BindTexture(&g_blockSpriteSheet->GetTexture());
    g_theRenderer->DrawIndexedVertexBuffer(m_vertexBuffer, m_indexBuffer, static_cast<unsigned int>(m_indices.size()));
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::InitializeBlocks()
{
	m_chunkBounds.m_mins = Vec3(m_chunkCoords, 0.f) * Vec3(CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z);
	m_chunkBounds.m_maxs = m_chunkBounds.m_mins + Vec3(CHUNK_SIZE_X, CHUNK_SIZE_Y, CHUNK_SIZE_Z);

    std::vector<float> densityMap;
    densityMap.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z);

	std::vector<float> continentalnessMap;
    continentalnessMap.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y);

	std::vector<float> erosionMap;
    erosionMap.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y);

	std::vector<float> peaksAndValleysMap;
    peaksAndValleysMap.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y);

	std::vector<float> temperatureMap;
    temperatureMap.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y);

	std::vector<float> humidityMap;
    humidityMap.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y);

	std::vector<int> heightMap;
    heightMap.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y);

	std::vector<int> dirtDepthMap;
    dirtDepthMap.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y);

	std::vector<int> lavaDepthMap;
	lavaDepthMap.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y);

	std::vector<int> obsidianDepthMap;
	obsidianDepthMap.resize(CHUNK_SIZE_X * CHUNK_SIZE_Y);

	GenerateTerrain_New(densityMap, continentalnessMap, erosionMap, peaksAndValleysMap, temperatureMap, humidityMap, heightMap, dirtDepthMap, 
 					lavaDepthMap, obsidianDepthMap);

    SurfaceBlockReplacement(densityMap, continentalnessMap, erosionMap, peaksAndValleysMap, temperatureMap, humidityMap, heightMap, dirtDepthMap, 
							lavaDepthMap, obsidianDepthMap);

//     m_chunkVerts.reserve(BLOCKS_PER_CHUNK * 18);
//     m_indices.reserve(BLOCKS_PER_CHUNK * 30);

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::GenerateTerrain_New(std::vector<float>& densityMap, std::vector<float>& continentalnessMap, std::vector<float>& erosionMap, std::vector<float>& peaksAndValleysMap, std::vector<float>& temperatureMap, std::vector<float>& humidityMap, std::vector<int>& heightMap, std::vector<int>& dirtDepthMap, std::vector<int>& lavaDepthMap, std::vector<int>& obsidianDepthMap)
{
	constexpr int BORDERED_SIZE_X = CHUNK_SIZE_X + 2;
	constexpr int BORDERED_SIZE_Y = CHUNK_SIZE_Y + 2;

	std::vector<float> borderedTerrainHeights;
	borderedTerrainHeights.resize(BORDERED_SIZE_X * BORDERED_SIZE_Y);

	for(int y = -1; y <= CHUNK_SIZE_Y; ++y)
	{
		for(int x = -1; x <= CHUNK_SIZE_X; ++x)
		{
			float globalX = static_cast<float>(m_chunkCoords.x * CHUNK_SIZE_X + x);
			float globalY = static_cast<float>(m_chunkCoords.y * CHUNK_SIZE_Y + y);

			TerrainColumn column = ComputeTerrainColumn(globalX, globalY);
			borderedTerrainHeights[(y + 1) * BORDERED_SIZE_X + (x + 1)] = column.m_height;

			if(x >= 0 && x < CHUNK_SIZE_X && y >= 0 && y < CHUNK_SIZE_Y)
			{
				int colIndex = y * CHUNK_SIZE_X + x;
				continentalnessMap[colIndex]	= column.m_continentalness;
				erosionMap[colIndex]			= column.m_erosion;
				peaksAndValleysMap[colIndex]	= column.m_peaksAndValleys;
				temperatureMap[colIndex]		= column.m_temperature;
			}
		}
	}

	for(int y = 0; y < CHUNK_SIZE_Y; ++y)
	{
		for(int x = 0; x < CHUNK_SIZE_X; ++x)
		{	
			float   globalX = static_cast<float>(m_chunkCoords.x * CHUNK_SIZE_X + x);
			float   globalY = static_cast<float>(m_chunkCoords.y * CHUNK_SIZE_Y + y);
			int     colIndex = y * CHUNK_SIZE_X + x;

			int		borderedIndex = (y + 1) * BORDERED_SIZE_X + (x + 1);
			float	terrainHeight = borderedTerrainHeights[borderedIndex];

			float humidity = Compute2dPerlinNoise(globalX, globalY, HUMIDITY_NOISE_SCALE, HUMIDITY_NOISE_OCTAVES, DEFAULT_OCTAVE_PERSISTENCE,
												  DEFAULT_NOISE_OCTAVE_SCALE, true, HUMIDITY_SEED);
			humidityMap[colIndex] = humidity;

			// Dirt Depth
			float dirtDepthPercent = Get2dNoiseNegOneToOne(static_cast<int>(globalX), static_cast<int>(globalY), DIRT_DEPTH_SEED);
			int dirtDepth = MIN_DIRT_OFFSET_Z + RoundDownToInt(dirtDepthPercent * (MAX_DIRT_OFFSET_Z - MIN_DIRT_OFFSET_Z));

			float largestDrop = 0.f;
			largestDrop = GetMax(largestDrop, terrainHeight - borderedTerrainHeights[borderedIndex - 1]);
			largestDrop = GetMax(largestDrop, terrainHeight - borderedTerrainHeights[borderedIndex + 1]);
			largestDrop = GetMax(largestDrop, terrainHeight - borderedTerrainHeights[borderedIndex - BORDERED_SIZE_X]);
			largestDrop = GetMax(largestDrop, terrainHeight - borderedTerrainHeights[borderedIndex + BORDERED_SIZE_X]);

			int slopeDepth = GetMin(RoundDownToInt(largestDrop) + 1, MAX_SURFACE_DEPTH);
			dirtDepth = GetMax(dirtDepth, slopeDepth);

			dirtDepthMap[colIndex] = dirtDepth;

			// Lava Depth
			float	lavaDepthPercent = Get2dNoiseNegOneToOne(static_cast<int>(globalX), static_cast<int>(globalY), LAVA_DEPTH_SEED);
			int		lavaDepth = MIN_LAVA_BLOCKS + RoundDownToInt(lavaDepthPercent * (MAX_LAVA_BLOCKS - MIN_LAVA_BLOCKS));

			lavaDepthMap[colIndex] = lavaDepth;

			// Obsidian Depth
			float obsidianDepthPercent = Get2dNoiseNegOneToOne(static_cast<int>(globalX), static_cast<int>(globalY), OBSIDIAN_DEPTH_SEED);
			int obsidianDepth = MIN_OBSIDIAN_BLOCKS + RoundDownToInt(obsidianDepthPercent * (MAX_OBSIDIAN_BLOCKS - MIN_OBSIDIAN_BLOCKS));

			obsidianDepthMap[colIndex] = obsidianDepth;

			int highestZ = 0;

			for(int z = 0; z < CHUNK_SIZE_Z; ++z)
			{
				IntVec3 localCoords = IntVec3(x, y, z);
				IntVec3 globalCoords = GetGlobalCoords(m_chunkCoords, localCoords);
				Vec3    globalCoordsFloats = Vec3(globalCoords);
				int index = LocalCoordsToIndex(localCoords);

				// Density
				float biasDensity = g_densityBiasPerBlockDelta * (terrainHeight - globalCoordsFloats.z);
				float rawDensityValue = 0.f;
				if(g_computeDensity && fabsf(biasDensity) <= 1.f)
				{
					rawDensityValue = Compute3dPerlinNoise(globalCoordsFloats.x, globalCoordsFloats.y, globalCoordsFloats.z, g_densityNoiseScale, static_cast<unsigned int>(g_densityNoiseOctaves), g_octavePersistence, g_octaveScale, true, DENSITY_SEED);
				}

				float density = rawDensityValue + biasDensity;

				densityMap[index] = density;

				if(density > 0.f && globalCoords.z > highestZ)
				{
					highestZ = globalCoords.z;
				}
			}

			heightMap[colIndex] = highestZ;
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
TerrainColumn Chunk::ComputeTerrainColumn(float globalX, float globalY) const
{
	TerrainColumn column;

	float continentalness = Compute2dPerlinNoise(globalX, globalY, g_continentalnessNoiseScale, static_cast<unsigned int>(g_continentalnessNoiseOctaves),
												 DEFAULT_OCTAVE_PERSISTENCE, DEFAULT_NOISE_OCTAVE_SCALE, true, CONTINENTALNESS_SEED);

	float erosion = Compute2dPerlinNoise(globalX, globalY, g_erosionNoiseScale, static_cast<unsigned int>(g_erosionNoiseOctaves),
										 DEFAULT_OCTAVE_PERSISTENCE, DEFAULT_NOISE_OCTAVE_SCALE, true, EROSION_SEED);

	float weirdness = Compute2dPerlinNoise(globalX, globalY, g_weirdnessNoiseScale, static_cast<unsigned int>(g_weirdnessNoiseOctaves),
										   DEFAULT_OCTAVE_PERSISTENCE, DEFAULT_NOISE_OCTAVE_SCALE, true, WEIRDNESS_SEED);

	float mountainRegion = Compute2dPerlinNoise(globalX, globalY, MOUNTAIN_REGION_NOISE_SCALE, MOUNTAIN_REGION_NOISE_OCTAVES,
												DEFAULT_OCTAVE_PERSISTENCE, DEFAULT_NOISE_OCTAVE_SCALE, true, MOUNTAIN_REGION_SEED);

	float temperature = Compute2dPerlinNoise(globalX, globalY, TEMPERATURE_NOISE_SCALE, TEMPERATURE_NOISE_OCTAVES, DEFAULT_OCTAVE_PERSISTENCE,
											 DEFAULT_NOISE_OCTAVE_SCALE, true, TEMPERATURE_SEED);

	float mountainScale = RangeMapClamped(mountainRegion, MOUNTAIN_REGION_START, MOUNTAIN_REGION_FULL, 0.f, 1.f);

	float weirdnessStretch = Lerp(1.f, WEIRDNESS_NOISE_STRETCH, mountainScale);
	weirdness = GetClamped(weirdness * weirdnessStretch, -1.f, 1.f);

	float peaksAndValleys = 1.f - fabsf(3.f * fabsf(weirdness) - 2.f);

	column.m_continentalness	= continentalness;
	column.m_erosion			= Lerp(erosion, -1.f, mountainScale * MOUNTAIN_REGION_EROSION_PULL);
	column.m_peaksAndValleys	= peaksAndValleys;

	float continentalnessHeightOffset = 0.f;
	if(g_computeContinentalness)
	{
		continentalnessHeightOffset = g_continentalnessCurve.Evaluate_new(continentalness) * CONTINENTALNESS_HEIGHT_RANGE;
	}

	float erosionScale = 1.f;
	if(g_computeErosion)
	{
		erosionScale = g_erosionCurve.Evaluate_new(column.m_erosion);
	}

	float peaksAndValleysHeightOffset = 0.f;
	if(g_computePeaksAndValley)
	{
		float inlandScale = RangeMapClamped(continentalness, g_coastCurve.m_start, g_midInlandCurve.m_start, 0.f, 1.f);
		peaksAndValleysHeightOffset = g_weirdnessCurve.Evaluate_new(peaksAndValleys) * erosionScale * inlandScale * PEAKS_AND_VALLEYS_HEIGHT_RANGE;

		if(peaksAndValleysHeightOffset > 0.f)
		{
			float coldScale = RangeMapClamped(-temperature, -COLD_MOUNTAIN_START_TEMPERATURE, -COLD_MOUNTAIN_FULL_TEMPERATURE, 1.f, COLD_MOUNTAIN_HEIGHT_MULTIPLIER);
			peaksAndValleysHeightOffset *= coldScale;
		}
	}

	column.m_height = g_densityTerrainHeight + continentalnessHeightOffset + peaksAndValleysHeightOffset;
	column.m_height = GetMin(column.m_height, MAX_TERRAIN_HEIGHT);

	float altitudeAboveSnowLine = GetMax(column.m_height - ALTITUDE_TEMPERATURE_START_Z, 0.f);
	column.m_temperature = temperature - (altitudeAboveSnowLine * ALTITUDE_TEMPERATURE_DROP);

	return column;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::SurfaceBlockReplacement(std::vector<float>& densityMap, std::vector<float>& continentalnessMap, std::vector<float>& erosionMap,
									std::vector<float>& peaksAndValleysMap, std::vector<float>& temperatureMap, std::vector<float>& humidityMap,
									std::vector<int>& heightMap, std::vector<int>& dirtDepthMap, std::vector<int>& lavaDepthMap, std::vector<int>& obsidianDepthMap)
{
	unsigned char blockType = g_blockTypes.m_air;

	for(int y = 0; y < CHUNK_SIZE_Y; ++y)
	{
		for(int x = 0; x < CHUNK_SIZE_X; ++x)
		{
			for(int z = CHUNK_SIZE_Z - 1; z >= 0; --z)
			{

				IntVec3 localCoords = IntVec3(x, y, z);
				IntVec3 globalCoords = GetGlobalCoords(m_chunkCoords, localCoords);
				int index = LocalCoordsToIndex(localCoords);
				int indexXY = y * CHUNK_SIZE_X + x;

				BiomeGenParams biomeGenParams;
				biomeGenParams.m_density = densityMap[index];
				biomeGenParams.m_height = heightMap[indexXY];
				biomeGenParams.m_continentalness = continentalnessMap[indexXY];
				biomeGenParams.m_erosion = erosionMap[indexXY];
				biomeGenParams.m_peaksAndValleys = peaksAndValleysMap[indexXY];
				biomeGenParams.m_temperature = temperatureMap[indexXY];
				biomeGenParams.m_humidity = humidityMap[indexXY];
				biomeGenParams.m_dirtDepth = dirtDepthMap[indexXY];
				biomeGenParams.m_blockZ = globalCoords.z;

				int lavaDepth		= lavaDepthMap[indexXY];
				int obsidianDepth	= obsidianDepthMap[indexXY];

				int lavaStartZ = 0;
				int lavaEndZ = lavaDepth;
				int obsidianStartZ = lavaEndZ + 1;
				int obsidianEndZ = obsidianStartZ + obsidianDepth;

				if(biomeGenParams.m_density > 0.f)
				{
					blockType = g_blockTypes.m_stone;
				}
				else
				{
					blockType = g_blockTypes.m_air;
				}

				if(g_enableSurfaceBlockReplacement)
				{
					if(globalCoords.z <= SEA_LEVEL_Z && blockType == g_blockTypes.m_air)
					{
						blockType = g_blockTypes.m_water;
					}

					biomeGenParams.m_blockBeforeReplacement = blockType;

					if(biomeGenParams.m_density > 0.f)
					{
						if(continentalnessMap[indexXY] < g_nonInlandBiomeMaxContinentalness)
						{
							blockType = GetBlockTypeForNonInlandBiomeBlock(biomeGenParams);
						}
						else
						{
							blockType = GetBlockTypeForInlandBiomeBlock(biomeGenParams);
						}
					}

					if(globalCoords.z < biomeGenParams.m_height - biomeGenParams.m_dirtDepth)
					{
						if(globalCoords.z >= lavaStartZ && globalCoords.z <= lavaEndZ)
						{
							blockType = g_blockTypes.m_lava;
						}
						else if(globalCoords.z >= obsidianStartZ && globalCoords.z <= obsidianEndZ)
						{
							blockType = g_blockTypes.m_obsidian;
						}
						else
						{
							float oreNoise = Get3dNoiseZeroToOne(globalCoords.x, globalCoords.y, globalCoords.z);
							if(oreNoise < DIAMOND_CHANCE)
							{
								blockType = g_blockTypes.m_diamond;
							}
							else if(oreNoise < GOLD_CHANCE)
							{
								blockType = g_blockTypes.m_gold;
							}
							else if(oreNoise < IRON_CHANCE)
							{
								blockType = g_blockTypes.m_iron;
							}
							else if(oreNoise < COAL_CHANCE)
							{
								blockType = g_blockTypes.m_coal;
							}
							else
							{
								blockType = g_blockTypes.m_stone;
							}
						}
					}

					if(globalCoords.z == biomeGenParams.m_height + 1 && blockType == g_blockTypes.m_air)
					{
						float glowstoneNoise = Get2dNoiseZeroToOne(globalCoords.x, globalCoords.y, GLOWSTONE_SEED);
						if(glowstoneNoise < GLOWSTONE_SURFACE_CHANCE)
						{
							blockType = g_blockTypes.m_glowstone;
						}
					}

				}

				m_blocks[index].m_blockDefinitionIndex = blockType;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::BuildChunkMesh()
{    
    m_chunkVerts.clear();
    m_indices.clear();

    for(int blockIndex = 0; blockIndex < BLOCKS_PER_CHUNK; ++blockIndex)
    {
		if(m_blocks[blockIndex].IsBlockVisible())
		{
			BlockIterator iter = BlockIterator(this, blockIndex);
			Directions blockingDirections = iter.GetBlockingDirections(blockIndex);

			if(blockingDirections == DIRECTION_ALL)
			{
				continue;
			}

            IntVec3 globalBlockCoords = GetGlobalCoords(m_chunkCoords, blockIndex);

			Vec3 mins = Vec3(globalBlockCoords);
			Vec3 maxs = mins + Vec3::ONE;

			Rgba8 zFaceColor = Rgba8::WHITE;
			Rgba8 xFaceColor = Rgba8(230, 230, 230);
			Rgba8 yFaceColor = Rgba8(200, 200, 200);

			Vec3 eightCornerPoints[AABB3_COUNT];
			eightCornerPoints[AABB3_BACK_RIGHT_BOTTOM]		= mins;
			eightCornerPoints[AABB3_BACK_RIGHT_TOP]			= Vec3(mins.x, mins.y, maxs.z);
			eightCornerPoints[AABB3_BACK_LEFT_TOP]			= Vec3(mins.x, maxs.y, maxs.z);
			eightCornerPoints[AABB3_BACK_LEFT_BOTTOM]		= Vec3(mins.x, maxs.y, mins.z);
			eightCornerPoints[AABB3_FORWARD_LEFT_BOTTOM]	= Vec3(maxs.x, maxs.y, mins.z);
			eightCornerPoints[AABB3_FORWARD_RIGHT_BOTTOM]	= Vec3(maxs.x, mins.y, mins.z);
			eightCornerPoints[AABB3_FORWARD_RIGHT_TOP]		= Vec3(maxs.x, mins.y, maxs.z);
			eightCornerPoints[AABB3_FORWARD_LEFT_TOP]		= maxs;

			AABB2 topUVs = m_blocks[blockIndex].GetBlockTopSpriteUVs();
			AABB2 bottomUVs = m_blocks[blockIndex].GetBlockBottomSpriteUVs();
			AABB2 sideUVs = m_blocks[blockIndex].GetBlockSideSpriteUVs();

			// Neighbor Blocks
			Block* posXBlock = iter.GetPosXBlock();
			Block* negXBlock = iter.GetNegXBlock();
			Block* posYBlock = iter.GetPosYBlock();
			Block* negYBlock = iter.GetNegYBlock();
			Block* posZBlock = iter.GetPosZBlock();
			Block* negZBlock = iter.GetNegZBlock();

			// Positive X Face
			if(!(blockingDirections & DIRECTION_POS_X))
			{
				BlockIterator posXNeighbor = iter.GetPosXBlockIterator();

				Rgba8 frontLeftTopVertColor;
				Rgba8 frontLeftBottomVertColor;
				Rgba8 frontRightTopVertColor;
				Rgba8 frontRightBottomVertColor;

				if(!g_enableSmoothLighting)
				{
					Rgba8 faceLightColor = CalculateFaceLighting(posXBlock, xFaceColor);
					frontLeftTopVertColor		= faceLightColor;
					frontLeftBottomVertColor	= faceLightColor;
					frontRightTopVertColor		= faceLightColor;
					frontRightBottomVertColor	= faceLightColor;
				}
				else if(IsValidIterator(posXNeighbor))
				{
					Block* lightPosY = posXNeighbor.GetPosYBlock();
					Block* lightPosZ = posXNeighbor.GetPosZBlock();
					Block* lightNegY = posXNeighbor.GetNegYBlock();
					Block* lightNegZ = posXNeighbor.GetNegZBlock();
					Block* lightPosYPosZ = posXNeighbor.GetPosYPosZBlock();
					Block* lightPosYNegZ = posXNeighbor.GetPosYNegZBlock();
					Block* lightNegYPosZ = posXNeighbor.GetNegYPosZBlock();
					Block* lightNegYNegZ = posXNeighbor.GetNegYNegZBlock();

					frontLeftTopVertColor = CalculateVertexLighting(lightPosY, lightPosZ, lightPosYPosZ, posXBlock, xFaceColor);
					frontLeftBottomVertColor = CalculateVertexLighting(lightPosY, lightNegZ, lightPosYNegZ, posXBlock, xFaceColor);
					frontRightTopVertColor = CalculateVertexLighting(lightNegY, lightPosZ, lightNegYPosZ, posXBlock, xFaceColor);
					frontRightBottomVertColor = CalculateVertexLighting(lightNegY, lightNegZ, lightNegYNegZ, posXBlock, xFaceColor);
				}

				AddVertsForQuad3DPerVertexColor(m_chunkVerts, m_indices, eightCornerPoints[AABB3_FORWARD_RIGHT_BOTTOM], eightCornerPoints[AABB3_FORWARD_LEFT_BOTTOM],
												eightCornerPoints[AABB3_FORWARD_LEFT_TOP], eightCornerPoints[AABB3_FORWARD_RIGHT_TOP], 
												frontRightBottomVertColor, frontLeftBottomVertColor, frontLeftTopVertColor, frontRightTopVertColor, sideUVs);

			}

			// Negative X Face
			if(!(blockingDirections & DIRECTION_NEG_X))
			{
				BlockIterator negXNeighbor = iter.GetNegXBlockIterator();

				Rgba8 backLeftTopVertColor;
				Rgba8 backLeftBottomVertColor;
				Rgba8 backRightTopVertColor;
				Rgba8 backRightBottomVertColor;

				if(!g_enableSmoothLighting)
				{
					Rgba8 faceLightColor = CalculateFaceLighting(negXBlock, xFaceColor);
					backLeftTopVertColor		= faceLightColor;
					backLeftBottomVertColor		= faceLightColor;
					backRightTopVertColor		= faceLightColor;
					backRightBottomVertColor	= faceLightColor;
				}
				else if(IsValidIterator(negXNeighbor))
				{
					Block* lightPosY = negXNeighbor.GetPosYBlock();
					Block* lightPosZ = negXNeighbor.GetPosZBlock();
					Block* lightNegY = negXNeighbor.GetNegYBlock();
					Block* lightNegZ = negXNeighbor.GetNegZBlock();
					Block* lightPosYPosZ = negXNeighbor.GetPosYPosZBlock();
					Block* lightPosYNegZ = negXNeighbor.GetPosYNegZBlock();
					Block* lightNegYPosZ = negXNeighbor.GetNegYPosZBlock();
					Block* lightNegYNegZ = negXNeighbor.GetNegYNegZBlock();

					backLeftTopVertColor		= CalculateVertexLighting(lightPosY, lightPosZ, lightPosYPosZ, negXBlock, xFaceColor);
					backLeftBottomVertColor		= CalculateVertexLighting(lightPosY, lightNegZ, lightPosYNegZ, negXBlock, xFaceColor);
					backRightTopVertColor		= CalculateVertexLighting(lightNegY, lightPosZ, lightNegYPosZ, negXBlock, xFaceColor);
					backRightBottomVertColor	= CalculateVertexLighting(lightNegY, lightNegZ, lightNegYNegZ, negXBlock, xFaceColor);
				}

				AddVertsForQuad3DPerVertexColor(m_chunkVerts, m_indices, eightCornerPoints[AABB3_BACK_LEFT_BOTTOM], eightCornerPoints[AABB3_BACK_RIGHT_BOTTOM],
												eightCornerPoints[AABB3_BACK_RIGHT_TOP], eightCornerPoints[AABB3_BACK_LEFT_TOP], backLeftBottomVertColor, backRightBottomVertColor, backRightTopVertColor,
												backLeftTopVertColor, sideUVs);
			}

			// Positive Y Face
			if(!(blockingDirections & DIRECTION_POS_Y))
			{
				BlockIterator posYNeighbor = iter.GetPosYBlockIterator();

				Rgba8 backLeftTopVertColor;
				Rgba8 backLeftBottomVertColor;
				Rgba8 frontLeftBottomVertColor;
				Rgba8 frontLeftTopVertColor;

				if(!g_enableSmoothLighting)
				{
					Rgba8 faceLightColor = CalculateFaceLighting(posYBlock, yFaceColor);
					backLeftTopVertColor		= faceLightColor;
					backLeftBottomVertColor		= faceLightColor;
					frontLeftBottomVertColor	= faceLightColor;
					frontLeftTopVertColor		= faceLightColor;
				}
				else if(IsValidIterator(posYNeighbor))
				{
					Block* lightPosX = posYNeighbor.GetPosXBlock();
					Block* lightPosZ = posYNeighbor.GetPosZBlock();
					Block* lightNegX = posYNeighbor.GetNegXBlock();
					Block* lightNegZ = posYNeighbor.GetNegZBlock();
					Block* lightPosZPosX = posYNeighbor.GetPosZPosXBlock();
					Block* lightNegZPosX = posYNeighbor.GetNegZPosXBlock();
					Block* lightNegZNegX = posYNeighbor.GetNegZNegXBlock();
					Block* lightPosZNegX = posYNeighbor.GetPosZNegXBlock();

					backLeftTopVertColor = CalculateVertexLighting(lightPosZ, lightNegX, lightPosZNegX, posYBlock, yFaceColor);
					backLeftBottomVertColor = CalculateVertexLighting(lightNegX, lightNegZ, lightNegZNegX, posYBlock, yFaceColor);
					frontLeftBottomVertColor = CalculateVertexLighting(lightPosX, lightNegZ, lightNegZPosX, posYBlock, yFaceColor);
					frontLeftTopVertColor = CalculateVertexLighting(lightPosX, lightPosZ, lightPosZPosX, posYBlock, yFaceColor);
				}

				AddVertsForQuad3DPerVertexColor(m_chunkVerts, m_indices, eightCornerPoints[AABB3_FORWARD_LEFT_BOTTOM], eightCornerPoints[AABB3_BACK_LEFT_BOTTOM],
												eightCornerPoints[AABB3_BACK_LEFT_TOP], eightCornerPoints[AABB3_FORWARD_LEFT_TOP], frontLeftBottomVertColor, backLeftBottomVertColor, backLeftTopVertColor,
												frontLeftTopVertColor, sideUVs);
			}

			// Negative Y Face
			if(!(blockingDirections & DIRECTION_NEG_Y))
			{
				BlockIterator negYNeighbor = iter.GetNegYBlockIterator();

				Rgba8 backRightTopVertColor;
				Rgba8 backRightBottomVertColor;
				Rgba8 frontRightBottomVertColor;
				Rgba8 frontRightTopVertColor;

				if(!g_enableSmoothLighting)
				{
					Rgba8 faceLightColor = CalculateFaceLighting(negYBlock, yFaceColor);
					backRightTopVertColor		= faceLightColor;
					backRightBottomVertColor	= faceLightColor;
					frontRightBottomVertColor	= faceLightColor;
					frontRightTopVertColor		= faceLightColor;
				}
				else if(IsValidIterator(negYNeighbor))
				{
					Block* lightPosX = negYNeighbor.GetPosXBlock();
					Block* lightPosZ = negYNeighbor.GetPosZBlock();
					Block* lightNegX = negYNeighbor.GetNegXBlock();
					Block* lightNegZ = negYNeighbor.GetNegZBlock();
					Block* lightPosZPosX = negYNeighbor.GetPosZPosXBlock();
					Block* lightNegZPosX = negYNeighbor.GetNegZPosXBlock();
					Block* lightNegZNegX = negYNeighbor.GetNegZNegXBlock();
					Block* lightPosZNegX = negYNeighbor.GetPosZNegXBlock();

					backRightTopVertColor = CalculateVertexLighting(lightPosZ, lightNegX, lightPosZNegX, negYBlock, yFaceColor);
					backRightBottomVertColor = CalculateVertexLighting(lightNegX, lightNegZ, lightNegZNegX, negYBlock, yFaceColor);
					frontRightBottomVertColor = CalculateVertexLighting(lightPosX, lightNegZ, lightNegZPosX, negYBlock, yFaceColor);
					frontRightTopVertColor = CalculateVertexLighting(lightPosX, lightPosZ, lightPosZPosX, negYBlock, yFaceColor);
				}

				AddVertsForQuad3DPerVertexColor(m_chunkVerts, m_indices, eightCornerPoints[AABB3_BACK_RIGHT_BOTTOM], eightCornerPoints[AABB3_FORWARD_RIGHT_BOTTOM],
								  eightCornerPoints[AABB3_FORWARD_RIGHT_TOP], eightCornerPoints[AABB3_BACK_RIGHT_TOP], backRightBottomVertColor, frontRightBottomVertColor, frontRightTopVertColor,
								  backRightTopVertColor, sideUVs);
			}

			// Positive Z Face
			if(!(blockingDirections & DIRECTION_POS_Z))
			{
				IntVec3 localCoord = IndexToLocalCoords(blockIndex);

				Rgba8 topLeftVertColor;
				Rgba8 topRightVertColor;
				Rgba8 bottomLeftVertColor;
				Rgba8 bottomRightVertColor;


				if(!g_enableSmoothLighting)
				{
					Rgba8 faceLightColor = CalculateFaceLighting(posZBlock, zFaceColor);
					topLeftVertColor		= faceLightColor;
					topRightVertColor		= faceLightColor;
					bottomLeftVertColor		= faceLightColor;
					bottomRightVertColor	= faceLightColor;
				}
				else if(localCoord.z < CHUNK_MAX_Z)
				{
					IntVec3 posZCoord = localCoord + IntVec3::UP;
					uint32_t posZIndex = LocalCoordsToIndex(posZCoord);

					BlockIterator posZIter = BlockIterator(this, posZIndex);

					Block* lightPosX = posZIter.GetPosXBlock();
					Block* lightPosY = posZIter.GetPosYBlock();
					Block* lightNegX = posZIter.GetNegXBlock();
					Block* lightNegY = posZIter.GetNegYBlock();
					Block* lightPosXPosY = posZIter.GetPosXPosYBlock();
					Block* lightPosXNegY = posZIter.GetPosXNegYBlock();
					Block* lightNegXPosY = posZIter.GetNegXPosYBlock();
					Block* lightNegXNegY = posZIter.GetNegXNegYBlock();

					topLeftVertColor		= CalculateVertexLighting(lightPosX, lightPosY, lightPosXPosY, posZBlock, zFaceColor);
					topRightVertColor		= CalculateVertexLighting(lightPosX, lightNegY, lightPosXNegY, posZBlock, zFaceColor);
					bottomLeftVertColor		= CalculateVertexLighting(lightNegX, lightPosY, lightNegXPosY, posZBlock, zFaceColor);
					bottomRightVertColor	= CalculateVertexLighting(lightNegX, lightNegY, lightNegXNegY, posZBlock, zFaceColor);
				}

				
 				AddVertsForQuad3DPerVertexColor(m_chunkVerts, m_indices, eightCornerPoints[AABB3_BACK_RIGHT_TOP], eightCornerPoints[AABB3_FORWARD_RIGHT_TOP],
 								  eightCornerPoints[AABB3_FORWARD_LEFT_TOP], eightCornerPoints[AABB3_BACK_LEFT_TOP], bottomRightVertColor, topRightVertColor, topLeftVertColor, bottomLeftVertColor, topUVs);
			}

			// Negative Z Face
			if(!(blockingDirections & DIRECTION_NEG_Z))
			{
				IntVec3 localCoord = IndexToLocalCoords(blockIndex);

				Rgba8 forwardRightBottomVertColor;
				Rgba8 backRightBottomVertColor;
				Rgba8 backLeftBottom;
				Rgba8 forwardLeftBottom;

				if(!g_enableSmoothLighting)
				{
					Rgba8 faceLightColor = CalculateFaceLighting(negZBlock, zFaceColor);
					forwardRightBottomVertColor = faceLightColor;
					backRightBottomVertColor	= faceLightColor;
					backLeftBottom				= faceLightColor;
					forwardLeftBottom			= faceLightColor;
				}
				else if(localCoord.z > 0)
				{
					IntVec3 negZCoord = localCoord + IntVec3::DOWN;
					uint32_t negZIndex = LocalCoordsToIndex(negZCoord);

					BlockIterator negZIter = BlockIterator(this, negZIndex);

					Block* lightPosX = negZIter.GetPosXBlock();
					Block* lightPosY = negZIter.GetPosYBlock();
					Block* lightNegX = negZIter.GetNegXBlock();
					Block* lightNegY = negZIter.GetNegYBlock();
					Block* lightPosXPosY = negZIter.GetPosXPosYBlock();
					Block* lightPosXNegY = negZIter.GetPosXNegYBlock();
					Block* lightNegXPosY = negZIter.GetNegXPosYBlock();
					Block* lightNegXNegY = negZIter.GetNegXNegYBlock();

					forwardRightBottomVertColor = CalculateVertexLighting(lightPosX, lightNegY, lightPosXNegY, negZBlock, zFaceColor);
					backRightBottomVertColor = CalculateVertexLighting(lightNegX, lightNegY, lightNegXNegY, negZBlock, zFaceColor);
					backLeftBottom = CalculateVertexLighting(lightNegX, lightPosY, lightNegXPosY, negZBlock, zFaceColor);
					forwardLeftBottom = CalculateVertexLighting(lightPosX, lightPosY, lightPosXPosY, negZBlock, zFaceColor);
				}


				AddVertsForQuad3DPerVertexColor(m_chunkVerts, m_indices, eightCornerPoints[AABB3_FORWARD_RIGHT_BOTTOM], eightCornerPoints[AABB3_BACK_RIGHT_BOTTOM],
								   								  eightCornerPoints[AABB3_BACK_LEFT_BOTTOM], eightCornerPoints[AABB3_FORWARD_LEFT_BOTTOM], forwardRightBottomVertColor, 
																  backRightBottomVertColor, backLeftBottom, forwardLeftBottom, bottomUVs);

			}
		}
    }

	unsigned int vboSize = sizeof(Vertex_PCUTBN) * static_cast<unsigned int>(m_chunkVerts.size());
	unsigned int iboSize = sizeof(unsigned int) * static_cast<unsigned int>(m_indices.size());

	if(m_indices.empty())
	{
		delete m_vertexBuffer;
		m_vertexBuffer = nullptr;

		delete m_indexBuffer;
		m_indexBuffer = nullptr;

		m_chunkState = ChunkState::ACTIVE;
		return;
	}

	if(!m_vertexBuffer)
	{
		m_vertexBuffer = g_theRenderer->CreateVertexBuffer(vboSize, sizeof(Vertex_PCUTBN));
	}

	if(!m_indexBuffer)
	{
		m_indexBuffer = g_theRenderer->CreateIndexBuffer(iboSize, sizeof(unsigned int));
	}

	g_theRenderer->CopyCPUToGPU(m_chunkVerts.data(), vboSize, m_vertexBuffer);
	g_theRenderer->CopyCPUToGPU(m_indices.data(), iboSize, m_indexBuffer);

    m_chunkState = ChunkState::ACTIVE;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Rgba8 Chunk::CalculateVertexLighting(Block* sideA, Block* sideB, Block* corner, Block* center, Rgba8 faceColor)
{
	float sideAOutdoorInfluence = 0.f;
	float sideBOutdoorInfluence = 0.f;
	float cornerOutdoorInfluence = 0.f;
	float centerOutdoorInfluence = 0.f;

	float sideAIndoorInfluence = 0.f;
	float sideBIndoorInfluence = 0.f;
	float cornerIndoorInfluence = 0.f;
	float centerIndoorInfluence = 0.f;

	float numSamples = 0.f;

	if(sideA)
	{
		sideAOutdoorInfluence = static_cast<float>(sideA->GetOutdoorLightInfluence());
		sideAIndoorInfluence = static_cast<float>(sideA->GetIndoorLightInfluence());
		numSamples += 1.f;
	}

	if(sideB)
	{
		sideBOutdoorInfluence = static_cast<float>(sideB->GetOutdoorLightInfluence());
		sideBIndoorInfluence = static_cast<float>(sideB->GetIndoorLightInfluence());
		numSamples += 1.f;
	}

	if(center)
	{
		centerOutdoorInfluence = static_cast<float>(center->GetOutdoorLightInfluence());
		centerIndoorInfluence = static_cast<float>(center->GetIndoorLightInfluence());
		numSamples += 1.f;
	}

	if(corner)
	{
		cornerOutdoorInfluence = static_cast<float>(corner->GetOutdoorLightInfluence());
		cornerIndoorInfluence = static_cast<float>(corner->GetIndoorLightInfluence());
		numSamples += 1.f;
	}

	if(numSamples == 0.f)
	{
		numSamples = 1.f;
	}

	float avgOutdoorInfluence = (sideAOutdoorInfluence + sideBOutdoorInfluence + cornerOutdoorInfluence + centerOutdoorInfluence) / numSamples;
	float avgIndoorInfluence = (sideAIndoorInfluence + sideBIndoorInfluence + cornerIndoorInfluence + centerIndoorInfluence) / numSamples;

	uint8_t outdoorInfluence = static_cast<uint8_t>(avgOutdoorInfluence);
	uint8_t indoorInfluence = static_cast<uint8_t>(avgIndoorInfluence);

	unsigned char outdoorColorChannel = GetColorChannelForLightInfluence(outdoorInfluence);
	unsigned char indoorColorChannel = GetColorChannelForLightInfluence(indoorInfluence);
	unsigned char color = faceColor.r;

	return Rgba8(outdoorColorChannel, indoorColorChannel, color, 255);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Rgba8 Chunk::CalculateFaceLighting(Block* neighbor, Rgba8 faceColor)
{
	if(!g_enableLighting)
	{
		return faceColor;
	}

	uint8_t outdoorLightInfluence	= 0;
	uint8_t indoorLightInfluence	= 0;

	if(neighbor)
	{
		outdoorLightInfluence	= neighbor->GetOutdoorLightInfluence();
		indoorLightInfluence	= neighbor->GetIndoorLightInfluence();
	}

	unsigned char r = GetColorChannelForLightInfluence(outdoorLightInfluence);
	unsigned char g = GetColorChannelForLightInfluence(indoorLightInfluence);
	unsigned char b = faceColor.r;

	return Rgba8(r, g, b, 255);
}

//------------------------------------------------------------------------------------------------------------------
int Chunk::GetTerrainHeightForBlockColumn(int globalBlockColumnXCoord, int globalBlockColumnYCoord) const
{
    constexpr float divideByThree = 1.f / 3.f;
    constexpr float divideByFive = 1.f / 5.f;

    IntRange randomNumberRange= IntRange(0, 1);
    int      randInt = randomNumberRange.GetRandomInt();

    float xDivideByThree = static_cast<float>(globalBlockColumnXCoord) * divideByThree;
    float yDivideByFive = static_cast<float>(globalBlockColumnYCoord) * divideByFive;
    
    int terrainHeightZ = 50 + static_cast<int>(xDivideByThree) + static_cast<int>(yDivideByFive) + randInt;

    return terrainHeightZ;
}

//------------------------------------------------------------------------------------------------------------------
int Chunk::GetBlockIndex(IntVec3 const& localBlockCoords) const
{
    return localBlockCoords.x + (localBlockCoords.y << CHUNK_BITS_X) + (localBlockCoords.z << (CHUNK_BITS_X + CHUNK_BITS_Y));
}

//------------------------------------------------------------------------------------------------------------------
Block Chunk::GenerateBlock(int terrainHeight, int currentZ, int numDirtBlocks) const
{
    if (currentZ == terrainHeight)
    {
        return GenerateBlockOfType("Grass");
    }

    if (currentZ > terrainHeight)
    {
        // x >> y is equal to x / 2^y. Here it is 128 / 2^1
        if (currentZ <= (CHUNK_SIZE_Z >> 1)) 
        {
           //return GenerateBlockOfType("Water");
        }

        return GenerateBlockOfType("Air");
    }

    if (currentZ < terrainHeight && currentZ > terrainHeight - numDirtBlocks)
    {
        return GenerateBlockOfType("Dirt"); 
    }

    IntRange chanceOfOresRange = IntRange(1, 1000);
    int chanceOfOres = chanceOfOresRange.GetRandomInt();

    // Chance of getting any number below 50 (in case of 100, below 5) is 5% -- Coal
    if (chanceOfOres <= 50) 
    {
        return GenerateBlockOfType("Coal"); 
    }

    // Getting any number from 51 to 70 (in case of 100, 6 and 7) is a 2% chance -- Iron
    if (chanceOfOres > 50 && chanceOfOres <= 70) 
    {
        return GenerateBlockOfType("Iron");
    }

    // Getting any number from 71 to 75 (in case of 100, getting 7.1 to 7.5 or 0.1 to 0.5) is a 0.5% chance -- Gold
    if (chanceOfOres > 70 && chanceOfOres <= 75) 
    {
        return GenerateBlockOfType("Gold");
    }

    // Getting a single number in a range of 1 - 1000 is 0.1% -- Diamond
    if (chanceOfOres == 76) 
    {
        return GenerateBlockOfType("Diamond");
    }
    
    return GenerateBlockOfType("Stone");
}

//------------------------------------------------------------------------------------------------------------------
Block Chunk::GenerateBlockOfType(std::string const& blockType) const
{
    Block block = Block();

    int defIndex = BlockDefinition::GetBlockDefinitionIndex(blockType);

    if (defIndex < 0)
    {
        ERROR_AND_DIE(Stringf("Could not get block def index for %s", blockType.c_str()))
    }
        
    block.m_blockDefinitionIndex = static_cast<unsigned char>(defIndex);
    return block;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::AddNeighbour(Chunk* chunk, Neighbours direction)
{
    m_neighbours[direction] = chunk;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::RemoveNeighbors()
{
    for(int i = 0; i < NEIGHBOUR_COUNT; ++i)
    {
        if(m_neighbours[i])
        {
            m_neighbours[i]->RemoveNeighbor(this);
            m_neighbours[i] = nullptr;
        }
    }
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::RemoveNeighbor(Chunk* chunk)
{
	for(int i = 0; i < NEIGHBOUR_COUNT; ++i)
	{
		if(m_neighbours[i] && m_neighbours[i] == chunk)
		{
			m_neighbours[i] = nullptr;
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForNonInlandBiomeBlock(BiomeGenParams const& params)
{   
    unsigned char blockType = params.m_blockBeforeReplacement;

	Continentalness continentType = GetContinentalnessLevel(params.m_continentalness);

    // Frozen Ocean
    if(g_t0Range.IsOnRange(params.m_temperature))
    {
		blockType = GetBlockTypeForFrozenOceanBiomeBlock(params);
    }
    else
    {	
		switch(continentType)
		{
			case Continentalness_DeepOcean:
			{
				blockType = GetBlockTypeForDeepOceanBiomeBlock(params);
				break;
			}
			case Continentalness_Ocean:
			{
				blockType = GetBlockTypeForOceanBiomeBlock(params);
				break;
			}
		}
    }

    return blockType;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForDeepOceanBiomeBlock(BiomeGenParams const& params)
{
	if(params.m_blockZ == params.m_height)
	{
		if(params.m_blockZ > SEA_LEVEL_Z)
		{
			if(params.m_temperature < 0.f)
			{
				return g_blockTypes.m_snow;
			}
			else
			{
				return g_blockTypes.m_sand;
			}
		}
		else if(params.m_blockZ == SEA_LEVEL_Z && params.m_blockBeforeReplacement == g_blockTypes.m_water && params.m_temperature < 0.f)
			return g_blockTypes.m_ice;
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForOceanBiomeBlock(BiomeGenParams const& params)
{
	if(params.m_blockZ == params.m_height)
	{
		if(params.m_blockZ > SEA_LEVEL_Z)
		{
			return g_blockTypes.m_sand;
		}
		else if(params.m_blockZ < SEA_LEVEL_Z)
		{
			return g_blockTypes.m_dirt;
		}
	}

	if(params.m_blockZ < params.m_height)
	{
		if(params.m_blockZ > SEA_LEVEL_Z && params.m_blockZ < params.m_height - params.m_dirtDepth)
		{
			return g_blockTypes.m_dirt;
		}
	}


	return params.m_blockBeforeReplacement;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForFrozenOceanBiomeBlock(BiomeGenParams const& params)
{
	if(params.m_blockZ == params.m_height)
	{
		if(params.m_blockZ > SEA_LEVEL_Z)
		{
			return g_blockTypes.m_snow;
		}
		else if(params.m_blockZ == SEA_LEVEL_Z && params.m_blockBeforeReplacement == g_blockTypes.m_water)
		{
			return g_blockTypes.m_ice;
		}
	}

	if(params.m_blockZ < params.m_height)
	{
		if(params.m_blockZ > SEA_LEVEL_Z && params.m_blockZ < params.m_height - params.m_dirtDepth)
		{
			return g_blockTypes.m_snow;
		}
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForInlandBiomeBlock(BiomeGenParams const& params)
{
	InlandBiomeType biomeType = GetInlandBiomeTypeForContinentalness(params.m_continentalness);

	switch(biomeType)
	{
		case InlandBiomeType_Coast:
			return GetBlockTypeForCoastBiomeBlock(params);
 		case InlandBiomeType_NearInland:
 			return GetBlockTypeForNearInlandBiomeBlock(params);
		case InlandBiomeType_MidInland:
			return GetBlockTypeForMidInlandBiomeBlock(params);
		case InlandBiomeType_FarInland:
			return GetBlockTypeForFarInlandBiomeBlock(params);
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForCoastBiomeBlock(BiomeGenParams const& params)
{
	PeaksAndValleysLevel pvLevel = GetPeaksAndValleysLevel(params.m_peaksAndValleys);
	ErosionLevel erosionLevel = GetErosionLevel(params.m_erosion);

	switch(pvLevel)
	{
		case PV_Valley:
			return GetBlockTypeForBeachBiomes(params);
  		case PV_Low:
 			return GetBlockTypeForBeachBiomes(params);
  		case PV_Mid:
 		{
 			if(erosionLevel != ErosionLevel_L3 && erosionLevel != ErosionLevel_L4)
 			{
 				return GetBlockTypeForBeachBiomes(params);
 			}
 			else
 			{
 				return GetBlockTypeForMiddleBiomes(params);
 			}
 		}
  		case PV_High:
 			return GetBlockTypeForMiddleBiomes(params);
  		case PV_Peak:
 		{
 			if(erosionLevel >= ErosionLevel_L2)
 			{
 				return GetBlockTypeForMiddleBiomes(params);
 			}
 			else if(erosionLevel == ErosionLevel_L1)
 			{
 				if(!g_t4Range.IsOnRange(params.m_temperature))
 				{
 					return GetBlockTypeForMiddleBiomes(params);
 				}
 				else
 				{
 					return GetBlockTypeForBadlandBiomes(params);
 				}
 			}
 			else
 			{
 				if(params.m_temperature <= g_t2Range.m_max)
 				{
 					return GetBlockTypeForSnowyPeaks(params);
 				}
 				else
 				{
 					return GetBlockTypeForStoneyPeaks(params);
 				}
 
 			}
 		} 	
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForNearInlandBiomeBlock(BiomeGenParams const& params)
{
	PeaksAndValleysLevel pvLevel = GetPeaksAndValleysLevel(params.m_peaksAndValleys);
	ErosionLevel erosionLevel = GetErosionLevel(params.m_erosion);

	switch(pvLevel)
	{
		case PV_Valley:
		{
			if(g_t4Range.IsOnRange(params.m_temperature))
			{
				return GetBlockTypeForBadlandBiomes(params);
			}
			else
			{
				return GetBlockTypeForMiddleBiomes(params);
			}
		}
		case PV_Low:
		{
			if(g_t4Range.IsOnRange(params.m_temperature) && erosionLevel < ErosionLevel_L2)
			{
				return GetBlockTypeForBadlandBiomes(params);
			}
			return GetBlockTypeForMiddleBiomes(params);
		}
		case PV_Mid:
		{
			if(erosionLevel == ErosionLevel_L0 || erosionLevel == ErosionLevel_L1)
			{
				if(g_t4Range.IsOnRange(params.m_temperature))
				{
					return GetBlockTypeForBadlandBiomes(params);
				}
				else
				{
					return GetBlockTypeForMiddleBiomes(params);
				}
			}
			return GetBlockTypeForMiddleBiomes(params);
		}
		case PV_High:
		{
			if(erosionLevel == ErosionLevel_L1)
			{
				if(g_t4Range.IsOnRange(params.m_temperature))
				{
					return GetBlockTypeForBadlandBiomes(params);
				}
				else
				{
					return GetBlockTypeForMiddleBiomes(params);
				}
			}
			else
			{
				return GetBlockTypeForMiddleBiomes(params);
			}
		}
		case PV_Peak:
		{
			if(erosionLevel == ErosionLevel_L0)
			{
				if(params.m_temperature <= g_t2Range.m_max)
				{
					return GetBlockTypeForSnowyPeaks(params);
				}
				else
				{
					return GetBlockTypeForStoneyPeaks(params);
				}
			}
			else if(erosionLevel == ErosionLevel_L1)
			{
				if(g_t4Range.IsOnRange(params.m_temperature))
				{
					return GetBlockTypeForBadlandBiomes(params);
				}
				else
				{
					return GetBlockTypeForMiddleBiomes(params);
				}
			}
			else
			{
				return GetBlockTypeForMiddleBiomes(params);
			}
		}
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForMidInlandBiomeBlock(BiomeGenParams const& params)
{
	PeaksAndValleysLevel pvLevel = GetPeaksAndValleysLevel(params.m_peaksAndValleys);
	ErosionLevel erosionLevel = GetErosionLevel(params.m_erosion);

	switch(pvLevel)
	{
		case PV_Valley:
		{
			if(g_t4Range.IsOnRange(params.m_temperature))
			{
				return GetBlockTypeForBadlandBiomes(params);
			}
			else
			{
				return GetBlockTypeForMiddleBiomes(params);
			}
		}

		case PV_Low:
		{
			if(g_t4Range.IsOnRange(params.m_temperature) && erosionLevel < ErosionLevel_L4)
			{
				return GetBlockTypeForBadlandBiomes(params);
			}
			return GetBlockTypeForMiddleBiomes(params);
		}

		case PV_Mid:
		{
			if(erosionLevel <= ErosionLevel_L3)
			{
				if(g_t4Range.IsOnRange(params.m_temperature))
				{
					return GetBlockTypeForBadlandBiomes(params);
				}
				else
				{
					return GetBlockTypeForMiddleBiomes(params);
				}
			}
			else
			{
				return GetBlockTypeForMiddleBiomes(params);
			}
		}

		case PV_High:
		{
			if(erosionLevel == ErosionLevel_L0)
			{
				if(params.m_temperature <= g_t2Range.m_max)
				{
					return GetBlockTypeForSnowyPeaks(params);
				}
				else
				{
					return GetBlockTypeForStoneyPeaks(params);
				}
			}
			else if(erosionLevel <= ErosionLevel_L3)
			{
				if(g_t4Range.IsOnRange(params.m_temperature))
				{
					return GetBlockTypeForBadlandBiomes(params);
				}
				else
				{
					return GetBlockTypeForMiddleBiomes(params);
				}
			}
			else
			{
				return GetBlockTypeForMiddleBiomes(params);
			}
		}

		case PV_Peak:
		{
			if(erosionLevel < ErosionLevel_L2)
			{
				if(params.m_temperature <= g_t2Range.m_max)
				{
					return GetBlockTypeForSnowyPeaks(params);
				}
				else
				{
					return GetBlockTypeForStoneyPeaks(params);
				}
			}
			else if(erosionLevel == ErosionLevel_L3)
			{
				if(g_t4Range.IsOnRange(params.m_temperature))
				{
					return GetBlockTypeForBadlandBiomes(params);
				}
				else
				{
					return GetBlockTypeForMiddleBiomes(params);
				}
			}
			else
			{
				return GetBlockTypeForMiddleBiomes(params);
			}
		}
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForFarInlandBiomeBlock(BiomeGenParams const& params)
{
	PeaksAndValleysLevel pvLevel = GetPeaksAndValleysLevel(params.m_peaksAndValleys);
	ErosionLevel erosionLevel = GetErosionLevel(params.m_erosion);

	switch(pvLevel)
	{
		case PV_Valley:
		{
			if(g_t4Range.IsOnRange(params.m_temperature))
				return GetBlockTypeForBadlandBiomes(params);
			else
				return GetBlockTypeForMiddleBiomes(params);
		}

		case PV_Low:
		{
			if(g_t4Range.IsOnRange(params.m_temperature) && erosionLevel < ErosionLevel_L4)
			{
				return GetBlockTypeForBadlandBiomes(params);
			}
			return GetBlockTypeForMiddleBiomes(params);
		}

		case PV_Mid:
		{
			if(erosionLevel == ErosionLevel_L0 || erosionLevel == ErosionLevel_L1 || erosionLevel == ErosionLevel_L3)
			{
				if(g_t4Range.IsOnRange(params.m_temperature))
				{
					return GetBlockTypeForBadlandBiomes(params);
				}
				else
				{
					return GetBlockTypeForMiddleBiomes(params);
				}
			}
			else
			{
				return GetBlockTypeForMiddleBiomes(params);
			}
		}

		case PV_High:
		{
			if(erosionLevel == ErosionLevel_L0)
			{
				if(params.m_temperature <= g_t2Range.m_max)
				{
					return GetBlockTypeForSnowyPeaks(params);
				}
				else
				{
					return GetBlockTypeForStoneyPeaks(params);
				}
			}
			else
			{
				return GetBlockTypeForMiddleBiomes(params);
			}
		}

		case PV_Peak:
		{
			if(erosionLevel <= ErosionLevel_L1)
			{
				if(params.m_temperature <= g_t2Range.m_max)
				{
					return GetBlockTypeForSnowyPeaks(params);
				}
				else
				{
					return GetBlockTypeForStoneyPeaks(params);
				}
			}
			else
			{
				return GetBlockTypeForMiddleBiomes(params);
			}
		}
	}
	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForBeachBiomes(BiomeGenParams const& params)
{
	if(g_t0Range.IsOnRange(params.m_temperature))
	{
		return GetBlockTypeForSnowyBeach(params);
	}
	else if(g_t4Range.IsOnRange(params.m_temperature))
	{
		return GetBlockTypeForDesertBiome(params);
	}
	else
	{
		return GetBlockTypeForNormalBeachBiome(params);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForSnowyBeach(BiomeGenParams const& params)
{
	if(params.m_blockZ == params.m_height)
	{
		return g_blockTypes.m_snow;
	}
	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForNormalBeachBiome(BiomeGenParams const& params)
{
	if(params.m_blockZ == params.m_height)
	{
		return g_blockTypes.m_sand;
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForDesertBiome(BiomeGenParams const& params)
{
	int sandDepth = params.m_dirtDepth * 3;
	if(params.m_blockZ == params.m_height || params.m_blockZ >= params.m_height - sandDepth)
	{
		return g_blockTypes.m_sand;
	}
	else if(params.m_blockZ < params.m_height - sandDepth && params.m_blockZ > params.m_height - (params.m_dirtDepth + sandDepth))
	{
		return g_blockTypes.m_dirt;
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForMiddleBiomes(BiomeGenParams const& params)
{
	if(g_t4Range.IsOnRange(params.m_temperature))
	{
		return GetBlockTypeForDesertBiome(params);
	}
	else if(g_t0Range.IsOnRange(params.m_temperature))
	{
		if(g_h0Range.IsOnRange(params.m_humidity) || g_h1Range.IsOnRange(params.m_humidity))
		{
			return GetBlockTypeForSnowyPlains(params);
		}
		else if(g_h2Range.IsOnRange(params.m_humidity) || g_h3Range.IsOnRange(params.m_humidity))
		{
			return GetBlockTypeForSnowyTaiga(params);
		}
		else
		{
			return GetBlockTypeForTaiga(params);
		}
	}
	else if(g_t1Range.IsOnRange(params.m_temperature))
	{
		if(g_h0Range.IsOnRange(params.m_humidity) || g_h1Range.IsOnRange(params.m_humidity))
		{
			return GetBlockTypeForPlains(params);
		}
		else if(g_h3Range.IsOnRange(params.m_humidity) || g_h4Range.IsOnRange(params.m_humidity))
		{
			return GetBlockTypeForTaiga(params);
		}
		else
		{
			return GetBlockTypeForForest(params);
		}
	}
	else if(g_t2Range.IsOnRange(params.m_temperature))
	{
		if(g_h0Range.IsOnRange(params.m_humidity) || g_h1Range.IsOnRange(params.m_humidity))
		{
			return GetBlockTypeForPlains(params);
		}
		else if(g_h2Range.IsOnRange(params.m_humidity) || g_h3Range.IsOnRange(params.m_humidity))
		{
			return GetBlockTypeForForest(params);
		}
		else
		{
			return GetBlockTypeForJungle(params);
		}
	}
	else if(g_t3Range.IsOnRange(params.m_temperature))
	{
		if(g_h0Range.IsOnRange(params.m_humidity) || g_h1Range.IsOnRange(params.m_humidity))
		{
			return GetBlockTypeForSavanna(params);
		}
		else if(g_h3Range.IsOnRange(params.m_humidity) || g_h4Range.IsOnRange(params.m_humidity))
		{
			return GetBlockTypeForJungle(params);
		}
		else
		{
			return GetBlockTypeForPlains(params);
		}
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForSnowyPlains(BiomeGenParams const& params)
{
	int snowDepth = GetMax(1, params.m_dirtDepth / 2);

	if(params.m_blockZ <= params.m_height && params.m_blockZ > params.m_height - snowDepth)
	{
		return g_blockTypes.m_snow;
	}
	else if(params.m_blockZ <params.m_height && params.m_blockZ >= params.m_height - params.m_dirtDepth)
	{
		return g_blockTypes.m_dirt;
	}
	else if(params.m_blockZ == SEA_LEVEL_Z)
	{
		return g_blockTypes.m_water;
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForSnowyTaiga(BiomeGenParams const& params)
{
	int snowDepth = GetMax(1, params.m_dirtDepth / 2);

	if(params.m_blockZ <= static_cast<int>(params.m_height) && params.m_blockZ > static_cast<int>(params.m_height) - snowDepth)
	{
		return g_blockTypes.m_snow;
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return g_blockTypes.m_dirt;
	}
	else if(params.m_blockZ == SEA_LEVEL_Z)
	{
		return g_blockTypes.m_ice;
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForTaiga(BiomeGenParams const& params)
{
	if(params.m_blockZ == static_cast<int>(params.m_height))
	{
		return g_blockTypes.m_grassLight;
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return g_blockTypes.m_dirt;
	}
	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForPlains(BiomeGenParams const& params)
{
	if(params.m_blockZ == static_cast<int>(params.m_height))
	{
		return g_blockTypes.m_grassLight;
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return g_blockTypes.m_dirt;
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForForest(BiomeGenParams const& params)
{
	if(params.m_blockZ == static_cast<int>(params.m_height))
	{
		return g_blockTypes.m_grass;
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return g_blockTypes.m_dirt;
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForJungle(BiomeGenParams const& params)
{
	if(params.m_blockZ == static_cast<int>(params.m_height))
	{
		return g_blockTypes.m_grassDark;
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return g_blockTypes.m_dirt;
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForSavanna(BiomeGenParams const& params)
{
	if(params.m_blockZ == static_cast<int>(params.m_height))
	{
		return g_blockTypes.m_grassYellow;
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return g_blockTypes.m_dirt;
	}
	return params.m_blockBeforeReplacement;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForBadlandBiomes(BiomeGenParams const& params)
{
	
	if(g_h3Range.IsOnRange(params.m_humidity) || g_h4Range.IsOnRange(params.m_humidity))
	{
		return GetBlockTypeForSavanna(params);
	}
	else
	{
		return GetBlockTypeForForest(params);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForStoneyPeaks(BiomeGenParams const& params)
{
	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char Chunk::GetBlockTypeForSnowyPeaks(BiomeGenParams const& params)
{
	if(params.m_blockZ <= params.m_height && params.m_blockZ >= params.m_height - params.m_dirtDepth)
		return g_blockTypes.m_snow;
	else if(params.m_blockZ == SEA_LEVEL_Z && params.m_blockBeforeReplacement == g_blockTypes.m_water)
		return g_blockTypes.m_ice;

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
InlandBiomeType Chunk::GetInlandBiomeTypeForContinentalness(float continentalness)
{
	if(continentalness < g_nearInlandCurve.m_start)
	{
		return InlandBiomeType_Coast;
	}
	else if(continentalness < g_midInlandCurve.m_start)
	{
		return InlandBiomeType_NearInland;
	}
	else if(continentalness < g_farInlandCurve.m_start)
	{
		return InlandBiomeType_MidInland;
	}

	return InlandBiomeType_FarInland;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PeaksAndValleysLevel Chunk::GetPeaksAndValleysLevel(float pvValue)
{
	if(pvValue < g_weirdnessLow.m_start)
	{
		return PV_Valley;
	}
	else if(pvValue < g_weirdnessMid.m_start)
	{
		return PV_Low;
	}
	else if(pvValue < g_weirdnessHigh.m_start)
	{
		return PV_Mid;
	}
	else if(pvValue < g_weirdnessPeaks.m_start)
	{
		return PV_High;
	}

	return PV_Peak;
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ErosionLevel Chunk::GetErosionLevel(float erosionValue)
{
	if(erosionValue < g_erosionL1.m_start)
	{
		return ErosionLevel_L0;
	}
	else if(erosionValue < g_erosionL2.m_start)
	{
		return ErosionLevel_L1;
	}
	else if(erosionValue < g_erosionL3.m_start)
	{
		return ErosionLevel_L2;
	}
	else if(erosionValue < g_erosionL4.m_start)
	{
		return ErosionLevel_L3;
	}
	else if(erosionValue < g_erosionL5.m_start)
	{
		return ErosionLevel_L4;
	}
	else if(erosionValue < g_erosionL6.m_start)
	{
		return ErosionLevel_L5;
	}

	return ErosionLevel_L6;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Continentalness Chunk::GetContinentalnessLevel(float value)
{
	if(value < g_oceanCurve.m_start)
		return Continentalness_DeepOcean;
	if(value < g_coastCurve.m_start)
		return Continentalness_Ocean;
	if(value < g_nearInlandCurve.m_start)
		return Continentalness_Coast;
	if(value < g_midInlandCurve.m_start)
		return Continentalness_NearInland;
	if(value < g_farInlandCurve.m_start)
		return Continentalness_MidInland;
	return Continentalness_FarInland;
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::MarkNeighborsDirty(IntVec3 const& removedBlockCoord)
{	
	if(removedBlockCoord.x == 0)
	{
		if(IsValidNeighbour(NEIGHBOUR_NEG_X))
		{
			Game::GetWorld()->MarkChunkDirty(m_neighbours[NEIGHBOUR_NEG_X]);
		}
	}
	if(removedBlockCoord.y == 0)
	{
		if(IsValidNeighbour(NEIGHBOUR_NEG_Y))
		{
			Game::GetWorld()->MarkChunkDirty(m_neighbours[NEIGHBOUR_NEG_Y]);
		}
	}
	if(removedBlockCoord.x == CHUNK_MAX_X)
	{
		if(IsValidNeighbour(NEIGHBOUR_POS_X))
		{
			Game::GetWorld()->MarkChunkDirty(m_neighbours[NEIGHBOUR_POS_X]);
		}
	}
	if(removedBlockCoord.y == CHUNK_MAX_Y)
	{
		if(IsValidNeighbour(NEIGHBOUR_POS_Y))
		{
			Game::GetWorld()->MarkChunkDirty(m_neighbours[NEIGHBOUR_POS_Y]);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Chunk::AreAllNeighborsValid()
{
	for(Chunk* neighbor : m_neighbours)
	{
		if(!neighbor)
		{
			return false;
		}
	}
	return true;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::SaveToFile()
{
    std::vector<uint8_t> buffer;

    ChunkSaveHeader header;
    buffer.push_back(header.m_gchk[0]);
    buffer.push_back(header.m_gchk[1]);
    buffer.push_back(header.m_gchk[2]);
    buffer.push_back(header.m_gchk[3]);
    buffer.push_back(header.m_gchk[4]);
    buffer.push_back(header.m_gchk[5]);
    buffer.push_back(header.m_gchk[6]);
    buffer.push_back(header.m_gchk[7]);

    ChunkSaveRun run;
    run.m_type = m_blocks[0].m_blockDefinitionIndex;
    run.m_length = 0;

    for(Block block : m_blocks)
    {
        unsigned char blockType = block.m_blockDefinitionIndex;

        if(blockType == run.m_type && run.m_length < 255)
        {
            run.m_length++;
        }
        else
        {
            buffer.push_back(run.m_type);
            buffer.push_back(run.m_length);

            run.m_type = blockType;
            run.m_length = 1;
        }
    }

    buffer.push_back(run.m_type);
    buffer.push_back(run.m_length);

    std::string fileName = Stringf("Saves/Chunk(%i,%i).chunk", m_chunkCoords.x, m_chunkCoords.y);
    FileWriteFromBuffer(buffer, fileName);

    m_isModified = false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Chunk::TryLoadFromFile()
{
	std::string fileName = Stringf("Saves/Chunk(%i,%i).chunk", m_chunkCoords.x, m_chunkCoords.y);
    if(!DoesFileExist(fileName))
    {
        return false;
    }

	std::vector<uint8_t> buffer;

    FileReadToBuffer(buffer, fileName);

    uint8_t* currentBuffer = buffer.data();

    ChunkSaveHeader* saveHeader = (ChunkSaveHeader*)(currentBuffer);

	uint8_t gchk[8] = {'G', 'C', 'H', 'K', 1, 4, 4, 7};

    if(!saveHeader || memcmp(saveHeader->m_gchk, gchk, 8) != 0)
    {
        return false;
    }

    currentBuffer += sizeof(ChunkSaveHeader);
    int runCount = static_cast<int>((buffer.size() - sizeof(ChunkSaveHeader)) / sizeof(ChunkSaveRun));

    int blockIndex = 0;

    for(int i = 0; i < runCount; i++)
    {
        ChunkSaveRun* currentRun = (ChunkSaveRun*)currentBuffer;

        for(int block = blockIndex; block < blockIndex + currentRun->m_length; ++block)
        {
            m_blocks[block].m_blockDefinitionIndex = currentRun->m_type;
        }

        blockIndex += currentRun->m_length;
        currentBuffer += sizeof(ChunkSaveRun);
    }

    return true;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
inline bool Chunk::IsValidNeighbour(Neighbours neighbourDirection)
{
	return m_neighbours[neighbourDirection] && (m_neighbours[neighbourDirection]->m_chunkState == ChunkState::ACTIVE || m_neighbours[neighbourDirection]->m_chunkState == ChunkState::ACTIVE_DIRTY);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::RemoveBlock(uint32_t blockIndex)
{
	m_blocks[blockIndex].m_blockDefinitionIndex = g_blockTypes.m_air;
	Game::GetWorld()->MarkChunkDirty(this);
	MarkNeighborsDirty(IndexToLocalCoords(blockIndex));
	m_isModified = true;
	RecomputeLightingForBlockRemoved(blockIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::PlaceBlockOfType(uint32_t blockIndex, unsigned char blockType)
{
	bool didReplaceSky = false;

	if(m_blocks[blockIndex].IsSky())
	{
		didReplaceSky = true;
	}

	m_blocks[blockIndex].m_blockDefinitionIndex = blockType;

	Game::GetWorld()->MarkChunkDirty(this);
	
	MarkNeighborsDirty(IndexToLocalCoords(blockIndex));
	m_isModified = true;
	RecomputeLightingForBlockAdded(blockIndex, didReplaceSky);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::InitializeLighting()
{
	World* world = Game::GetWorld();

	bool hasNegXNeighbour = IsValidNeighbour(NEIGHBOUR_NEG_X);
	bool hasPosXNeighbour = IsValidNeighbour(NEIGHBOUR_POS_X);
	bool hasNegYNeighbour = IsValidNeighbour(NEIGHBOUR_NEG_Y);
	bool hasPosYNeighbour = IsValidNeighbour(NEIGHBOUR_POS_Y);

	// Boundary Blocks Loop
	for(int z = 0; z <= CHUNK_MAX_Z; z++)
	{
		if(hasNegYNeighbour)
		{
			for(int x = 0; x <= CHUNK_MAX_X; x++)
			{
				world->MarkLightingDirtyIfNotOpaque(BlockIterator(this, LocalCoordsToIndex(x, 0, z)));
			}
		}

		if(hasPosYNeighbour)
		{
			for(int x = 0; x <= CHUNK_MAX_X; x++)
			{
				world->MarkLightingDirtyIfNotOpaque(BlockIterator(this, LocalCoordsToIndex(x, CHUNK_MAX_Y, z)));
			}
		}

		if(hasNegXNeighbour)
		{
			for(int y = 0; y <= CHUNK_MAX_Y; y++)
			{
				world->MarkLightingDirtyIfNotOpaque(BlockIterator(this, LocalCoordsToIndex(0, y, z)));
			}
		}

		if(hasPosXNeighbour)
		{
			for(int y = 0; y <= CHUNK_MAX_Y; y++)
			{
				world->MarkLightingDirtyIfNotOpaque(BlockIterator(this, LocalCoordsToIndex(CHUNK_MAX_X, y, z)));
			}
		}
	}

	// Blocks and Outdoor Lighting Loop(s)
	for(int x = 0; x <= CHUNK_MAX_X; x++)
	{
		for(int y = 0; y <= CHUNK_MAX_Y; y++)
		{
			for(int z = CHUNK_MAX_Z; z >= 0; z--)
			{
				uint32_t blockIndex = LocalCoordsToIndex(x, y, z);
				if(m_blocks[blockIndex].IsBlockOpaque())
				{
					break;
				}
				m_blocks[blockIndex].SetIsSky(true);
			}
		}
	}

	for(int x = 0; x <= CHUNK_MAX_X; x++)
	{
		for(int y = 0; y <= CHUNK_MAX_Y; y++)
		{
			for(int z = CHUNK_MAX_Z; z >= 0; z--)
			{
				uint32_t blockIndex = LocalCoordsToIndex(x, y, z);
				if(m_blocks[blockIndex].IsBlockOpaque())
				{
					break;
				}

				m_blocks[blockIndex].SetOutdoorLightInfluence(OUTDOOR_LIGHT_INFLUENCE_MAX);
				MarkNonSkyHorizontalBlockLightDirty(IntVec3(x, y, z));
			}
		}
	}

	// Glowstone Loop
	unsigned char glowstoneIndex = g_blockTypes.m_glowstone;
	for(uint32_t i = 0; i < BLOCKS_PER_CHUNK; i++)
	{
		if(m_blocks[i].m_blockDefinitionIndex == glowstoneIndex)
		{
			world->MarkLightingDirty(BlockIterator(this, i));
		}
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::MarkNonSkyHorizontalBlockLightDirty(IntVec3 const& localCoords)
{
	World* world = Game::GetWorld();

	BlockIterator iterator(this, LocalCoordsToIndex(localCoords));

	BlockIterator posXIterator = iterator.GetPosXBlockIterator();
	BlockIterator posYIterator = iterator.GetPosYBlockIterator();
	BlockIterator negXIterator = iterator.GetNegXBlockIterator();
	BlockIterator negYIterator = iterator.GetNegYBlockIterator();

	if(posXIterator.m_chunk && !posXIterator.GetBlockRef().IsSky())
	{
		world->MarkLightingDirty(posXIterator);
	}
	if(posYIterator.m_chunk && !posYIterator.GetBlockRef().IsSky())
	{
		world->MarkLightingDirty(posYIterator);
	}
	if(negXIterator.m_chunk && !negXIterator.GetBlockRef().IsSky())
	{
		world->MarkLightingDirty(negXIterator);
	}
	if(negYIterator.m_chunk && !negYIterator.GetBlockRef().IsSky())
	{
		world->MarkLightingDirty(negYIterator);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::RecomputeLightingForBlockRemoved(uint32_t blockIndex)
{
	Block& block = m_blocks[blockIndex];
	
	IntVec3 localCoords = IndexToLocalCoords(blockIndex);
	Game::GetWorld()->MarkLightingDirty(BlockIterator(this, blockIndex));
	
	// if this is the top most block, mark it as sky
	if(localCoords.z == CHUNK_MAX_Z)
	{
		block.SetIsSky(true);
	}

	IntVec3 localCoordAbove = localCoords + IntVec3::UP;

	if(!IsValidLocalCoord(localCoordAbove))
	{
		return;
	}

	uint32_t blockIndexAbove = LocalCoordsToIndex(localCoordAbove);
	if(!m_blocks[blockIndexAbove].IsSky())
	{
		return;
	}

	for(int z = localCoords.z; z >= 0; z--)
	{
		uint32_t index = LocalCoordsToIndex(localCoords.x, localCoords.y, z);

		if(m_blocks[index].IsBlockOpaque())
		{
			break;
		}

		m_blocks[index].SetIsSky(true);
		Game::GetWorld()->MarkLightingDirty(BlockIterator(this, index));
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::RecomputeLightingForBlockAdded(uint32_t blockIndex, bool didReplaceSky)
{
	Block& block = m_blocks[blockIndex];

	IntVec3 localCoords = IndexToLocalCoords(blockIndex);
	Game::GetWorld()->MarkLightingDirty(BlockIterator(this, blockIndex));

	if(didReplaceSky && block.IsBlockOpaque())
	{
		block.SetIsSky(false);

		for(int z = localCoords.z - 1; z >= 0; z--)
		{
			uint32_t index = LocalCoordsToIndex(localCoords.x, localCoords.y, z);

			if(m_blocks[index].IsBlockOpaque())
			{
				break;
			}

			m_blocks[index].SetIsSky(false);
			Game::GetWorld()->MarkLightingDirty(BlockIterator(this, index));
		}

	}
}

//------------------------------------------------------------------------------------------------------------------
AABB3 Chunk::GetChunkBounds() const
{
    return m_chunkBounds;
}

//------------------------------------------------------------------------------------------------------------------
IntVec2 Chunk::GetChunkCoordinates() const
{
    return m_chunkCoords;
}

//------------------------------------------------------------------------------------------------------------------
bool Chunk::IsBlockOpaque(uint32_t blockIndex) const
{
    return m_blocks[blockIndex].IsBlockOpaque();
}

//------------------------------------------------------------------------------------------------------------------
bool Chunk::IsBlockWater(uint32_t blockIndex) const
{
    return m_blocks[blockIndex].m_blockDefinitionIndex == g_blockTypes.m_water;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool Chunk::IsBlockAir(uint32_t blockIndex) const
{
	return m_blocks[blockIndex].IsBlockAir();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block Chunk::GetBlockAtCoord(IntVec3 const& localCoord) const
{
    uint32_t index = LocalCoordsToIndex(localCoord);

    return m_blocks[index];
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Chunk* Chunk::GetNeighborChunk(Neighbours direction) const
{
	return m_neighbours[direction];
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block& Chunk::GetBlock(uint32_t blockIndex)
{
	return m_blocks[blockIndex];
}

