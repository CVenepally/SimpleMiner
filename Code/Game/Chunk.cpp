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
Chunk::Chunk(IntVec2 const& chunkCoordinates)
    : m_chunkCoords(chunkCoordinates)
{

	std::string fileName = Stringf("Saves/Chunk(%i,%i).chunk", m_chunkCoords.x, m_chunkCoords.y);
	if(DoesFileExist(fileName))
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

//     GenerateTerrain(densityMap, continentalnessMap, erosionMap, peaksAndValleysMap, temperatureMap, humidityMap, heightMap, dirtDepthMap, 
// 					lavaDepthMap, obsidianDepthMap);
// 
	GenerateTerrain_New(densityMap, continentalnessMap, erosionMap, peaksAndValleysMap, temperatureMap, humidityMap, heightMap, dirtDepthMap, 
 					lavaDepthMap, obsidianDepthMap);

    SurfaceBlockReplacement(densityMap, continentalnessMap, erosionMap, peaksAndValleysMap, temperatureMap, humidityMap, heightMap, dirtDepthMap, 
							lavaDepthMap, obsidianDepthMap);

//     m_chunkVerts.reserve(BLOCKS_PER_CHUNK * 18);
//     m_indices.reserve(BLOCKS_PER_CHUNK * 30);

    DebuggerPrintf("[Chunk] Blocks Initialized!\n");
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::GenerateTerrain(std::vector<float>& densityMap, std::vector<float>& continentalnessMap, std::vector<float>& erosionMap, 
                                   std::vector<float>& peaksAndValleysMap, std::vector<float>& temperatureMap, std::vector<float>& humidityMap, 
                                   std::vector<int>& heightMap, std::vector<int>& dirtDepthMap, std::vector<int>& lavaDepthMap, std::vector<int>& obsidianDepthMap)
{
	for(int y = 0; y < CHUNK_SIZE_Y; ++y)
	{
		for(int x = 0; x < CHUNK_SIZE_X; ++x)
		{
			float   globalX = static_cast<float>(m_chunkCoords.x * CHUNK_SIZE_X + x);
			float   globalY = static_cast<float>(m_chunkCoords.y * CHUNK_SIZE_Y + y);
			int     colIndex = y * CHUNK_SIZE_X + x;

			float heightOffset = ComputeTerrainHeightOffset(globalX, globalY, continentalnessMap, erosionMap, peaksAndValleysMap, colIndex);

			float baseTerrainHeight = DEFAULT_TERRAIN_HEIGHT + (heightOffset * (CHUNK_SIZE_Z / 2));
			float oneOverBaseHeight = 1.f / baseTerrainHeight;

            // Temperature and humidity
            float temperature = Compute2dPerlinNoise(globalX, globalY, TEMPERATURE_NOISE_SCALE, TEMPERATURE_NOISE_OCTAVES, DEFAULT_OCTAVE_PERSISTENCE, 
                                                     DEFAULT_NOISE_OCTAVE_SCALE, true, TEMPERATURE_SEED);
            temperatureMap[colIndex] = temperature;
            
            float humidity = Compute2dPerlinNoise(globalX, globalY, HUMIDITY_NOISE_SCALE, HUMIDITY_NOISE_OCTAVES, DEFAULT_OCTAVE_PERSISTENCE, 
                                                     DEFAULT_NOISE_OCTAVE_SCALE, true, HUMIDITY_SEED);
            humidityMap[colIndex] = humidity;

            // Dirt Depth
			float dirtDepthPercent  = Get2dNoiseNegOneToOne(static_cast<int>(globalX), static_cast<int>(globalY), DIRT_DEPTH_SEED);
			int dirtDepth           = MIN_DIRT_OFFSET_Z + RoundDownToInt(dirtDepthPercent * (MAX_DIRT_OFFSET_Z - MIN_DIRT_OFFSET_Z));

			dirtDepthMap[colIndex] = dirtDepth;

            // Lava Depth
			float	lavaDepthPercent	= Get2dNoiseNegOneToOne(static_cast<int>(globalX), static_cast<int>(globalY), LAVA_DEPTH_SEED);
			int		lavaDepth           = MIN_LAVA_BLOCKS + RoundDownToInt(lavaDepthPercent * (MAX_LAVA_BLOCKS - MIN_LAVA_BLOCKS));

			lavaDepthMap[colIndex] = lavaDepth;

            // Obsidian Depth
			float obsidianDepthPercent  = Get2dNoiseNegOneToOne(static_cast<int>(globalX), static_cast<int>(globalY), OBSIDIAN_DEPTH_SEED);
			int obsidianDepth           = MIN_OBSIDIAN_BLOCKS + RoundDownToInt(obsidianDepthPercent * (MAX_OBSIDIAN_BLOCKS - MIN_OBSIDIAN_BLOCKS));

			obsidianDepthMap[colIndex] = obsidianDepth;
			
			int highestZ = 0;

			for(int z = 0; z < CHUNK_SIZE_Z; ++z)
			{
				IntVec3 localCoords = IntVec3(x, y, z);
				IntVec3 globalCoords = GetGlobalCoords(m_chunkCoords, localCoords);
				Vec3    globalCoordsFloats = Vec3(globalCoords);
				int index = LocalCoordsToIndex(localCoords);

				// Density
				float rawDensityValue = Compute3dPerlinNoise(globalCoordsFloats.x, globalCoordsFloats.y, globalCoordsFloats.z, g_densityNoiseScale, static_cast<unsigned int>(g_densityNoiseOctaves), g_octavePersistence, g_octaveScale, true, DENSITY_SEED);

				float bias = g_densityBiasPerBlockDelta * (globalCoords.z - baseTerrainHeight);

				float biasedDensity = rawDensityValue + bias;

				if(!g_computeDensity)
				{
					biasedDensity = 0.f;
				}

				if(g_computeContinentalness)
				{
					float heightOffSetFromTerrainHeight = (globalCoords.z - baseTerrainHeight) * oneOverBaseHeight;
					biasedDensity += heightOffSetFromTerrainHeight;
				}

				densityMap[index] = biasedDensity;

				if(biasedDensity < 0.f)
				{
					if(globalCoords.z > highestZ)
					{
						highestZ = globalCoords.z;
					}
				}
			}
			heightMap[colIndex] = highestZ;
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::GenerateTerrain_New(std::vector<float>& densityMap, std::vector<float>& continentalnessMap, std::vector<float>& erosionMap, std::vector<float>& peaksAndValleysMap, std::vector<float>& temperatureMap, std::vector<float>& humidityMap, std::vector<int>& heightMap, std::vector<int>& dirtDepthMap, std::vector<int>& lavaDepthMap, std::vector<int>& obsidianDepthMap)
{
	for(int y = 0; y < CHUNK_SIZE_Y; ++y)
	{
		for(int x = 0; x < CHUNK_SIZE_X; ++x)
		{	
			// Continentalness Terrain Height Offset Calculation

			float   globalX = static_cast<float>(m_chunkCoords.x * CHUNK_SIZE_X + x);
			float   globalY = static_cast<float>(m_chunkCoords.y * CHUNK_SIZE_Y + y);
			int     colIndex = y * CHUNK_SIZE_X + x;

			float continentalnessHeightOffset = 0.f;
			float erosion = 0.f;
			float heightOffset = 0.f;

			if(g_computeContinentalness)
			{

				float continentalnessInput = Compute2dPerlinNoise(globalX, globalY, g_continentalnessNoiseScale, static_cast<unsigned int>(g_continentalnessNoiseOctaves),
															  DEFAULT_OCTAVE_PERSISTENCE, DEFAULT_NOISE_OCTAVE_SCALE, true, CONTINENTALNESS_SEED);
			
				continentalnessHeightOffset = g_continentalnessCurve.Evaluate_new(continentalnessInput);

				continentalnessMap[colIndex] = (continentalnessHeightOffset);
				heightOffset = continentalnessHeightOffset;
			}

			if(g_computeErosion)
			{
				float erosionInput = Compute2dPerlinNoise(globalX, globalY, g_erosionNoiseScale, static_cast<unsigned int>(g_erosionNoiseOctaves),
													  DEFAULT_OCTAVE_PERSISTENCE, DEFAULT_NOISE_OCTAVE_SCALE, true, EROSION_SEED);

				erosion = g_erosionCurve.Evaluate_new(continentalnessHeightOffset) * erosionInput;

				erosionMap[colIndex] = erosion;
				heightOffset = erosion;
			}
			
			if(g_computePeaksAndValley)
			{
				float weirdnessInput = Compute2dPerlinNoise(globalX, globalY, g_weirdnessNoiseScale, static_cast<unsigned int>(g_weirdnessNoiseOctaves),
														DEFAULT_OCTAVE_PERSISTENCE, DEFAULT_NOISE_OCTAVE_SCALE, true, WEIRDNESS_SEED);
				float weirdness = g_weirdnessCurve.Evaluate_new(continentalnessHeightOffset) * weirdnessInput;
				float pv = 1.f - fabsf(3.f * fabsf(weirdness) - 2);
				peaksAndValleysMap[colIndex] = pv;
			//	heightOffset = pv;
			}

			// Temperature and humidity
			float temperature = Compute2dPerlinNoise(globalX, globalY, TEMPERATURE_NOISE_SCALE, TEMPERATURE_NOISE_OCTAVES, DEFAULT_OCTAVE_PERSISTENCE,
													 DEFAULT_NOISE_OCTAVE_SCALE, true, TEMPERATURE_SEED);
			temperatureMap[colIndex] = temperature;

			float humidity = Compute2dPerlinNoise(globalX, globalY, HUMIDITY_NOISE_SCALE, HUMIDITY_NOISE_OCTAVES, DEFAULT_OCTAVE_PERSISTENCE,
												  DEFAULT_NOISE_OCTAVE_SCALE, true, HUMIDITY_SEED);
			humidityMap[colIndex] = humidity;

			// Dirt Depth
			float dirtDepthPercent = Get2dNoiseNegOneToOne(static_cast<int>(globalX), static_cast<int>(globalY), DIRT_DEPTH_SEED);
			int dirtDepth = MIN_DIRT_OFFSET_Z + RoundDownToInt(dirtDepthPercent * (MAX_DIRT_OFFSET_Z - MIN_DIRT_OFFSET_Z));

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
				float rawDensityValue = Compute3dPerlinNoise(globalCoordsFloats.x, globalCoordsFloats.y, globalCoordsFloats.z, g_densityNoiseScale, static_cast<unsigned int>(g_densityNoiseOctaves), g_octavePersistence, g_octaveScale, true, DENSITY_SEED);

				float baseTerrainHeight = g_densityTerrainHeight + (heightOffset * (CHUNK_SIZE_Z / 2));

				float bias = g_densityBiasPerBlockDelta * (baseTerrainHeight - globalCoords.z);

				float biasedDensity = rawDensityValue + bias;

				float heightOffSetFromTerrainHeight = (globalCoords.z - baseTerrainHeight) * (1.f / baseTerrainHeight);

				if(!g_computeContinentalness)
				{
					heightOffSetFromTerrainHeight = 0.f;
				}

				biasedDensity += heightOffset;
				biasedDensity -= heightOffSetFromTerrainHeight;

				if(!g_computeDensity)
				{
					biasedDensity = 1.f;
				}

				densityMap[index] = biasedDensity;

				if(biasedDensity > 0.f)
				{
					if(globalCoords.z > highestZ)
					{
						highestZ = globalCoords.z;
					}
				}
			}

			heightMap[colIndex] = highestZ;

		}
	}

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
float Chunk::ComputeTerrainHeightOffset(float blockGlobalX, float blockGlobalY, std::vector<float>& continentalnessMap, std::vector<float>& erosionMap,
										std::vector<float>& peaksAndValleysMap, int colIndex)
{
	float finalHeightOffset = 0.f;
    float erosion = 1.f;
    float pv = 0.f;

	// Continentalness
	float continentalnessInput = Compute2dPerlinNoise(blockGlobalX, blockGlobalY, g_continentalnessNoiseScale, static_cast<unsigned int>(g_continentalnessNoiseOctaves),
													  DEFAULT_OCTAVE_PERSISTENCE, DEFAULT_NOISE_OCTAVE_SCALE, true, CONTINENTALNESS_SEED);

	float continentalness = g_continentalnessCurve.Evaluate(continentalnessInput);

	continentalnessMap[colIndex] = (continentalness);

	float erosionInput = Compute2dPerlinNoise(blockGlobalX, blockGlobalY, g_erosionNoiseScale, static_cast<unsigned int>(g_erosionNoiseOctaves),
											  DEFAULT_OCTAVE_PERSISTENCE, DEFAULT_NOISE_OCTAVE_SCALE, true, EROSION_SEED);
	erosion = g_erosionCurve.Evaluate(continentalness) * erosionInput;
	//erosion = g_erosionCurve.Evaluate(erosionInput);

	erosionMap[colIndex] = erosion;

	float weirdnessInput = Compute2dPerlinNoise(blockGlobalX, blockGlobalY, g_weirdnessNoiseScale, static_cast<unsigned int>(g_weirdnessNoiseOctaves),
												DEFAULT_OCTAVE_PERSISTENCE, DEFAULT_NOISE_OCTAVE_SCALE, true, WEIRDNESS_SEED);
	float weirdness = g_weirdnessCurve.Evaluate(continentalness) * weirdnessInput;
	//float weirdness = g_weirdnessCurve.Evaluate(weirdnessInput);

	pv = 1.f - abs((3.f * abs(weirdness)) - 2.f);

	peaksAndValleysMap[colIndex] = pv;

	float continentalnessHeightOffset = continentalness;
	float erosionHeightOffset = erosion;
	float pvHeightOffset =  pv;

	if(!g_computeErosion)
	{
		erosionHeightOffset = 0.f;
	}
    
    if(!g_computePeaksAndValley)
    {
		pvHeightOffset = 0.f;
    }


    //finalHeightOffset = continentalnessHeightOffset * erosionHeightOffset + pvHeightOffset;
    finalHeightOffset = continentalnessHeightOffset + erosionHeightOffset + pvHeightOffset;

	return finalHeightOffset;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::SurfaceBlockReplacement(std::vector<float>& densityMap, std::vector<float>& continentalnessMap, std::vector<float>& erosionMap,
									std::vector<float>& peaksAndValleysMap, std::vector<float>& temperatureMap, std::vector<float>& humidityMap,
									std::vector<int>& heightMap, std::vector<int>& dirtDepthMap, std::vector<int>& lavaDepthMap, std::vector<int>& obsidianDepthMap)
{
	std::string blockType = "Air";

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
					blockType = "Stone";
				}
				else
				{
					blockType = "Air";
				}

				if(g_enableSurfaceBlockReplacement)
				{
					if(globalCoords.z <= SEA_LEVEL_Z && blockType == "Air")
					{
						blockType = "Water";
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
							blockType = "Lava";
						}
						else if(globalCoords.z >= obsidianStartZ && globalCoords.z <= obsidianEndZ)
						{
							blockType = "Obsidian";
						}
						else
						{
							float oreNoise = Get3dNoiseZeroToOne(globalCoords.x, globalCoords.y, globalCoords.z);
							if(oreNoise < DIAMOND_CHANCE)
							{
								blockType = "Diamond";
							}
							else if(oreNoise < GOLD_CHANCE)
							{
								blockType = "Gold";
							}
							else if(oreNoise < IRON_CHANCE)
							{
								blockType = "Iron";
							}
							else if(oreNoise < COAL_CHANCE)
							{
								blockType = "Coal";
							}
							else
							{
								blockType = "Stone";
							}
						}
					}

				}

				m_blocks[index].m_blockDefinitionIndex = BlockDefinition::GetBlockDefinitionIndex(blockType);
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::BuildChunkMesh()
{    
    m_chunkVerts.clear();
    m_indices.clear();

    if(m_vertexBuffer)
    {
        delete m_vertexBuffer;
        m_vertexBuffer = nullptr;
    }
    if(m_indexBuffer)
    {
        delete m_indexBuffer;
        m_indexBuffer = nullptr;
    }

    for(int blockIndex = 0; blockIndex < BLOCKS_PER_CHUNK; ++blockIndex)
    {
		if(m_blocks[blockIndex].IsBlockVisible())
		{
            IntVec3 globalBlockCoords = GetGlobalCoords(m_chunkCoords, blockIndex);

			AABB3 blockBounds;
			blockBounds.m_mins = Vec3(globalBlockCoords);
			blockBounds.m_maxs = Vec3(globalBlockCoords) + Vec3::ONE;

			Rgba8 zFaceColor = Rgba8::WHITE;
			Rgba8 xFaceColor = Rgba8(230, 230, 230);
			Rgba8 yFaceColor = Rgba8(200, 200, 200);

			std::vector<Vec3> eightCornerPoints;
			blockBounds.GetCornerPoints(eightCornerPoints);

			AABB2 topUVs = m_blocks[blockIndex].GetBlockTopSpriteUVs();
			AABB2 bottomUVs = m_blocks[blockIndex].GetBlockBottomSpriteUVs();
			AABB2 sideUVs = m_blocks[blockIndex].GetBlockSideSpriteUVs();

			BlockIterator iter = BlockIterator(this, blockIndex);
			Directions blockingDirections = iter.GetBlockingDirections(blockIndex);

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
// 				Block* neighborBlock = iter.GetPosXBlock();
// 
// 				uint8_t outdoorLightInfluence	= neighborBlock->GetOutdoorLightInfluence();
// 				uint8_t indoorLightInfluence	= neighborBlock->GetIndoorLightInfluence();
// 
// 				unsigned char r = GetColorChannelForLightInfluence(outdoorLightInfluence);
// 				unsigned char g = GetColorChannelForLightInfluence(indoorLightInfluence);
// 				unsigned char b = xFaceColor.r;
// 
// 				Rgba8 color(r, g, b, 255);
// 
// 				if(!g_enableLighting)
// 				{
// 					color = xFaceColor;
// 				}
// 
// 				AddVertsForQuad3D(m_chunkVerts, m_indices, eightCornerPoints[AABB3_FORWARD_RIGHT_BOTTOM], eightCornerPoints[AABB3_FORWARD_LEFT_BOTTOM],
// 								  eightCornerPoints[AABB3_FORWARD_LEFT_TOP], eightCornerPoints[AABB3_FORWARD_RIGHT_TOP], color, sideUVs);

				BlockIterator posXNeighbor = iter.GetPosXBlockIterator();

				Rgba8 frontLeftTopVertColor;
				Rgba8 frontLeftBottomVertColor;
				Rgba8 frontRightTopVertColor;
				Rgba8 frontRightBottomVertColor;

				if(IsValidIterator(posXNeighbor))
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
// 				Block* neighborBlock = iter.GetNegXBlock();
// 
// 				uint8_t outdoorLightInfluence = neighborBlock->GetOutdoorLightInfluence();
// 				uint8_t indoorLightInfluence =	neighborBlock->GetIndoorLightInfluence();
// 
// 				unsigned char r = GetColorChannelForLightInfluence(outdoorLightInfluence);
// 				unsigned char g = GetColorChannelForLightInfluence(indoorLightInfluence);
// 				unsigned char b = xFaceColor.r;
// 
// 				Rgba8 color(r, g, b, 255);
// 
// 				if(!g_enableLighting)
// 				{
// 					color = xFaceColor;
// 				}
// 
// 				AddVertsForQuad3D(m_chunkVerts, m_indices, eightCornerPoints[AABB3_BACK_LEFT_BOTTOM], eightCornerPoints[AABB3_BACK_RIGHT_BOTTOM],
// 								  eightCornerPoints[AABB3_BACK_RIGHT_TOP], eightCornerPoints[AABB3_BACK_LEFT_TOP], color, sideUVs);

				BlockIterator negXNeighbor = iter.GetNegXBlockIterator();

				Rgba8 backLeftTopVertColor;
				Rgba8 backLeftBottomVertColor;
				Rgba8 backRightTopVertColor;
				Rgba8 backRightBottomVertColor;

				if(IsValidIterator(negXNeighbor))
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
// 				Block* neighborBlock = iter.GetPosYBlock();
// 
// 				uint8_t outdoorLightInfluence	= neighborBlock->GetOutdoorLightInfluence();
// 				uint8_t indoorLightInfluence	= neighborBlock->GetIndoorLightInfluence();
// 
// 				unsigned char r = GetColorChannelForLightInfluence(outdoorLightInfluence);
// 				unsigned char g = GetColorChannelForLightInfluence(indoorLightInfluence);
// 				unsigned char b = yFaceColor.r;
// 
// 				Rgba8 color(r, g, b, 255);
// 
// 				if(!g_enableLighting)
// 				{
// 					color = yFaceColor;
// 				}
// 
// 				AddVertsForQuad3D(m_chunkVerts, m_indices, eightCornerPoints[AABB3_FORWARD_LEFT_BOTTOM], eightCornerPoints[AABB3_BACK_LEFT_BOTTOM],
// 								  eightCornerPoints[AABB3_BACK_LEFT_TOP], eightCornerPoints[AABB3_FORWARD_LEFT_TOP], color, sideUVs
				
				BlockIterator posYNeighbor = iter.GetPosYBlockIterator();

				Rgba8 backLeftTopVertColor;
				Rgba8 backLeftBottomVertColor;
				Rgba8 frontLeftBottomVertColor;
				Rgba8 frontLeftTopVertColor;

				if(IsValidIterator(posYNeighbor))
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
// 				Block* neighborBlock = iter.GetNegYBlock();
// 
// 				uint8_t outdoorLightInfluence	= neighborBlock->GetOutdoorLightInfluence();
// 				uint8_t indoorLightInfluence	= neighborBlock->GetIndoorLightInfluence();
// 
// 				unsigned char r = GetColorChannelForLightInfluence(outdoorLightInfluence);
// 				unsigned char g = GetColorChannelForLightInfluence(indoorLightInfluence);
// 				unsigned char b = yFaceColor.r;
// 
// 				Rgba8 color(r, g, b, 255);
// 
// 				if(!g_enableLighting)
// 				{
// 					color = yFaceColor;
// 				}
// 
// 				AddVertsForQuad3D(m_chunkVerts, m_indices, eightCornerPoints[AABB3_BACK_RIGHT_BOTTOM], eightCornerPoints[AABB3_FORWARD_RIGHT_BOTTOM],
// 								  eightCornerPoints[AABB3_FORWARD_RIGHT_TOP], eightCornerPoints[AABB3_BACK_RIGHT_TOP], color, sideUVs);

				BlockIterator negYNeighbor = iter.GetNegYBlockIterator();

				Rgba8 backRightTopVertColor;
				Rgba8 backRightBottomVertColor;
				Rgba8 frontRightBottomVertColor;
				Rgba8 frontRightTopVertColor;

				if(IsValidIterator(negYNeighbor))
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
// 				IntVec3 localCoord = IndexToLocalCoords(blockIndex);
// 				uint8_t outdoorLightInfluence = 0;
// 				uint8_t indoorLightInfluence = 0;
// 
// 				if(localCoord.z < CHUNK_MAX_Z)
// 				{
// 					Block* neighborBlock = iter.GetPosZBlock();
// 					outdoorLightInfluence	= neighborBlock->GetOutdoorLightInfluence();
// 					indoorLightInfluence	= neighborBlock->GetIndoorLightInfluence();
// 				}
// 
// 				unsigned char r = GetColorChannelForLightInfluence(outdoorLightInfluence);
// 				unsigned char g = GetColorChannelForLightInfluence(indoorLightInfluence);
// 				unsigned char b = zFaceColor.r;
// 
// 				Rgba8 color(r, g, b, 255);
// 
// 				if(!g_enableLighting)
// 				{
// 					color = zFaceColor;
// 				}
//				
//				AddVertsForQuad3D(m_chunkVerts, m_indices, eightCornerPoints[AABB3_BACK_RIGHT_TOP], eightCornerPoints[AABB3_FORWARD_RIGHT_TOP], eightCornerPoints[AABB3_FORWARD_LEFT_TOP], 
//									eightCornerPoints[AABB3_BACK_LEFT_TOP], color, topUVs);

				IntVec3 localCoord = IndexToLocalCoords(blockIndex);

				Rgba8 topLeftVertColor;
				Rgba8 topRightVertColor;
				Rgba8 bottomLeftVertColor;
				Rgba8 bottomRightVertColor;


				if(localCoord.z < CHUNK_MAX_Z)
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
// 				IntVec3 localCoord = IndexToLocalCoords(blockIndex);
// 				uint8_t outdoorLightInfluence = 0;
// 				uint8_t indoorLightInfluence = 0;
// 
// 				if(localCoord.z > 0)
// 				{
// 					Block* neighborBlock = iter.GetNegZBlock();
// 					outdoorLightInfluence	= neighborBlock->GetOutdoorLightInfluence();
// 					indoorLightInfluence	= neighborBlock->GetIndoorLightInfluence();
// 				}
// 
// 				unsigned char r = GetColorChannelForLightInfluence(outdoorLightInfluence);
// 				unsigned char g = GetColorChannelForLightInfluence(indoorLightInfluence);
// 				unsigned char b = zFaceColor.r;
// 
// 				Rgba8 color(r, g, b, 255);
// 
// 				if(!g_enableLighting)
// 				{
// 					color = zFaceColor;
// 				}
// 
// 				AddVertsForQuad3D(m_chunkVerts, m_indices, eightCornerPoints[AABB3_FORWARD_RIGHT_BOTTOM], eightCornerPoints[AABB3_BACK_RIGHT_BOTTOM],
// 								  eightCornerPoints[AABB3_BACK_LEFT_BOTTOM], eightCornerPoints[AABB3_FORWARD_LEFT_BOTTOM], color, bottomUVs);

				IntVec3 localCoord = IndexToLocalCoords(blockIndex);

				Rgba8 forwardRightBottomVertColor;
				Rgba8 backRightBottomVertColor;
				Rgba8 backLeftBottom;
				Rgba8 forwardLeftBottom;

				if(localCoord.z > 0)
				{
					IntVec3 negZCoord = localCoord + IntVec3::DOWN;
					uint32_t negZIndex = LocalCoordsToIndex(negZCoord);

					BlockIterator negZIter = BlockIterator(this, negZIndex);

					Block* lightPosX = negZIter.GetNegXBlock();
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

	m_vertexBuffer = g_theRenderer->CreateVertexBuffer(sizeof(Vertex_PCUTBN), sizeof(Vertex_PCUTBN));
	m_indexBuffer = g_theRenderer->CreateIndexBuffer(sizeof(unsigned int), sizeof(unsigned int));

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

	if(sideA)
	{
		sideAOutdoorInfluence = static_cast<float>(sideA->GetOutdoorLightInfluence());
		sideAIndoorInfluence = static_cast<float>(sideA->GetIndoorLightInfluence());
	}
	
	if(sideB)
	{
		sideBOutdoorInfluence = static_cast<float>(sideB->GetOutdoorLightInfluence());
		sideBIndoorInfluence = static_cast<float>(sideB->GetIndoorLightInfluence());
	}
	
	if(center)
	{
		centerOutdoorInfluence = static_cast<float>(center->GetOutdoorLightInfluence());
		centerIndoorInfluence = static_cast<float>(center->GetIndoorLightInfluence());
	}
	
	if(corner)
	{
		cornerOutdoorInfluence = static_cast<float>(corner->GetOutdoorLightInfluence());
		cornerIndoorInfluence = static_cast<float>(corner->GetIndoorLightInfluence());
	}
	
	float avgOutdoorInfluence = (sideAOutdoorInfluence + sideBOutdoorInfluence + cornerOutdoorInfluence + centerOutdoorInfluence) / 4.f;
	float avgIndoorInfluence = (sideAIndoorInfluence + sideBIndoorInfluence + cornerIndoorInfluence + centerIndoorInfluence) / 4.f;

	uint8_t outdoorInfluence = static_cast<uint8_t>(avgOutdoorInfluence);
	uint8_t indoorInfluence = static_cast<uint8_t>(avgIndoorInfluence);

	unsigned char outdoorColorChannel = GetColorChannelForLightInfluence(outdoorInfluence);
	unsigned char indoorColorChannel = GetColorChannelForLightInfluence(indoorInfluence);
	unsigned char color = faceColor.r;

	return Rgba8(outdoorColorChannel, indoorColorChannel, color, 255);
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
std::string Chunk::GetBlockTypeForNonInlandBiomeBlock(BiomeGenParams const& params)
{   
    std::string blockType = params.m_blockBeforeReplacement;

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
std::string Chunk::GetBlockTypeForDeepOceanBiomeBlock(BiomeGenParams const& params)
{
	if(params.m_blockZ == params.m_height)
	{
		if(params.m_blockZ > SEA_LEVEL_Z)
		{
			if(params.m_temperature < 0.f)
			{
				return "Snow";
			}
			else
			{
				return "Sand";
			}
		}
		else if(params.m_blockZ == SEA_LEVEL_Z && params.m_blockBeforeReplacement == "Water" && params.m_temperature < 0.f)
			return "Ice";
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForOceanBiomeBlock(BiomeGenParams const& params)
{
	if(params.m_blockZ == params.m_height)
	{
		if(params.m_blockZ > SEA_LEVEL_Z)
		{
			return "Sand";
		}
		else if(params.m_blockZ < SEA_LEVEL_Z)
		{
			return "Dirt";
		}
	}

	if(params.m_blockZ < params.m_height)
	{
		if(params.m_blockZ > SEA_LEVEL_Z && params.m_blockZ < params.m_height - params.m_dirtDepth)
		{
			return "Dirt";
		}
	}


	return params.m_blockBeforeReplacement;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForFrozenOceanBiomeBlock(BiomeGenParams const& params)
{
	if(params.m_blockZ == params.m_height)
	{
		if(params.m_blockZ > SEA_LEVEL_Z)
		{
			return "Snow";
		}
		else if(params.m_blockZ == SEA_LEVEL_Z && params.m_blockBeforeReplacement == "Water")
		{
			return "Ice";
		}
	}

	if(params.m_blockZ < params.m_height)
	{
		if(params.m_blockZ > SEA_LEVEL_Z && params.m_blockZ < params.m_height - params.m_dirtDepth)
		{
			return "Snow";
		}
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForInlandBiomeBlock(BiomeGenParams const& params)
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
std::string Chunk::GetBlockTypeForCoastBiomeBlock(BiomeGenParams const& params)
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
std::string Chunk::GetBlockTypeForNearInlandBiomeBlock(BiomeGenParams const& params)
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
std::string Chunk::GetBlockTypeForMidInlandBiomeBlock(BiomeGenParams const& params)
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
std::string Chunk::GetBlockTypeForFarInlandBiomeBlock(BiomeGenParams const& params)
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
std::string Chunk::GetBlockTypeForBeachBiomes(BiomeGenParams const& params)
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
std::string Chunk::GetBlockTypeForSnowyBeach(BiomeGenParams const& params)
{
	if(params.m_blockZ == params.m_height)
	{
		return "Snow";
	}
	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForNormalBeachBiome(BiomeGenParams const& params)
{
	if(params.m_blockZ == params.m_height)
	{
		return "Sand";
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForDesertBiome(BiomeGenParams const& params)
{
	int sandDepth = params.m_dirtDepth * 3;
	if(params.m_blockZ == params.m_height || params.m_blockZ >= params.m_height - sandDepth)
	{
		return "Sand";
	}
	else if(params.m_blockZ < params.m_height - sandDepth && params.m_blockZ > params.m_height - (params.m_dirtDepth + sandDepth))
	{
		return "Dirt";
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForMiddleBiomes(BiomeGenParams const& params)
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
std::string Chunk::GetBlockTypeForSnowyPlains(BiomeGenParams const& params)
{
	
	if(params.m_blockZ == params.m_height)
	{
		return "Snow";
	}
	else if(params.m_blockZ <params.m_height && params.m_blockZ >= params.m_height - params.m_dirtDepth)
	{
		return "Dirt";
	}
	else if(params.m_blockZ == SEA_LEVEL_Z)
	{
		return "Water";
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForSnowyTaiga(BiomeGenParams const& params)
{
	if(params.m_blockZ == static_cast<int>(params.m_height))
	{	
		return "Snow";
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return "Dirt";
	}
	else if(params.m_blockZ == SEA_LEVEL_Z)
	{
		return "Ice";
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForTaiga(BiomeGenParams const& params)
{
	if(params.m_blockZ == static_cast<int>(params.m_height))
	{
		return "GrassLight";
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return "Dirt";
	}
	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForPlains(BiomeGenParams const& params)
{
	if(params.m_blockZ == static_cast<int>(params.m_height))
	{
		return "GrassLight";
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return "Dirt";
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForForest(BiomeGenParams const& params)
{
	if(params.m_blockZ == static_cast<int>(params.m_height))
	{
		return "Grass";
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return "Dirt";
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForJungle(BiomeGenParams const& params)
{
	if(params.m_blockZ == static_cast<int>(params.m_height))
	{
		return "GrassDark";
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return "Dirt";
	}

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForSavanna(BiomeGenParams const& params)
{
	if(params.m_blockZ == static_cast<int>(params.m_height))
	{
		return "GrassYellow";
	}
	else if(params.m_blockZ < static_cast<int>(params.m_height) && params.m_blockZ >= static_cast<int>(params.m_height) - params.m_dirtDepth)
	{
		return "Dirt";
	}
	return params.m_blockBeforeReplacement;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForBadlandBiomes(BiomeGenParams const& params)
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
std::string Chunk::GetBlockTypeForStoneyPeaks(BiomeGenParams const& params)
{
	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
std::string Chunk::GetBlockTypeForSnowyPeaks(BiomeGenParams const& params)
{
	if(params.m_blockZ == params.m_height)
		return "Snow";
	else if(params.m_blockZ == SEA_LEVEL_Z && params.m_blockBeforeReplacement == "Water")
		return "Ice";

	return params.m_blockBeforeReplacement;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
InlandBiomeType Chunk::GetInlandBiomeTypeForContinentalness(float continentalness)
{
	FloatRange coastRange = FloatRange(g_coastCurve.m_start, g_coastCurve.m_end);
	FloatRange nearInlandRange = FloatRange(g_nearInlandCurve.m_start, g_nearInlandCurve.m_end);
	FloatRange midInlandRange = FloatRange(g_midInlandCurve.m_start, g_midInlandCurve.m_end);
	FloatRange farInlandRange = FloatRange(g_farInlandCurve.m_start, g_farInlandCurve.m_end);

	if(coastRange.IsOnRange(continentalness))
	{
		return InlandBiomeType_Coast;
	}
	else if(nearInlandRange.IsOnRange(continentalness))
	{
		return InlandBiomeType_NearInland;
	}
	else if(midInlandRange.IsOnRange(continentalness))
	{
		return InlandBiomeType_MidInland;
	}
	else if(farInlandRange.IsOnRange(continentalness))
	{
		return InlandBiomeType_FarInland;
	}

	return InlandBiomeType_FarInland;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
PeaksAndValleysLevel Chunk::GetPeaksAndValleysLevel(float pvValue)
{
	FloatRange valleyRange = FloatRange(g_weirdnessValleys.m_start, g_weirdnessValleys.m_end);
	FloatRange lowRange = FloatRange(g_weirdnessLow.m_start, g_weirdnessLow.m_end);
	FloatRange midRange = FloatRange(g_weirdnessMid.m_start, g_weirdnessMid.m_end);
	FloatRange highRange = FloatRange(g_weirdnessHigh.m_start, g_weirdnessHigh.m_end);
	FloatRange peakRange = FloatRange(g_weirdnessPeaks.m_start, g_weirdnessPeaks.m_end);

	if(valleyRange.IsOnRange(pvValue))
	{
		return PV_Valley;
	}
	else if(lowRange.IsOnRange(pvValue))
	{
		return PV_Low;
	}
	else if(midRange.IsOnRange(pvValue))
	{
		return PV_Mid;
	}
	else if(highRange.IsOnRange(pvValue))
	{
		return PV_High;
	}
	else if(peakRange.IsOnRange(pvValue))
	{
		return PV_Peak;
	}

	return PV_Mid;
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
ErosionLevel Chunk::GetErosionLevel(float erosionValue)
{
	FloatRange l0Range = FloatRange(g_erosionL0.m_start, g_erosionL0.m_end);
	FloatRange l1Range = FloatRange(g_erosionL1.m_start, g_erosionL1.m_end);
	FloatRange l2Range = FloatRange(g_erosionL2.m_start, g_erosionL2.m_end);
	FloatRange l3Range = FloatRange(g_erosionL3.m_start, g_erosionL3.m_end);
	FloatRange l4Range = FloatRange(g_erosionL4.m_start, g_erosionL4.m_end);
	FloatRange l5Range = FloatRange(g_erosionL5.m_start, g_erosionL5.m_end);
	FloatRange l6Range = FloatRange(g_erosionL6.m_start, g_erosionL6.m_end);

	if(l0Range.IsOnRange(erosionValue))
	{
		return ErosionLevel_L0;
	}
	else if(l1Range.IsOnRange(erosionValue))
	{
		return ErosionLevel_L1;
	}
	else if(l2Range.IsOnRange(erosionValue))
	{
		return ErosionLevel_L2;
	}
	else if(l3Range.IsOnRange(erosionValue))
	{
		return ErosionLevel_L3;
	}
	else if(l4Range.IsOnRange(erosionValue))
	{
		return ErosionLevel_L4;
	}
	else if(l5Range.IsOnRange(erosionValue))
	{
		return ErosionLevel_L5;
	}
	else if(l6Range.IsOnRange(erosionValue))
	{
		return ErosionLevel_L6;
	}

	return ErosionLevel_L3;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Continentalness Chunk::GetContinentalnessLevel(float value)
{
	if(value < g_deepOceanCurve.m_end)
		return Continentalness_DeepOcean;
	if(value < g_oceanCurve.m_end)
		return Continentalness_Ocean;
	if(value < g_coastCurve.m_end)
		return Continentalness_Coast;
	if(value < g_nearInlandCurve.m_end)
		return Continentalness_NearInland;
	if(value < g_midInlandCurve.m_end)
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
	return m_neighbours[neighbourDirection] && (m_neighbours[neighbourDirection]->m_chunkState == ChunkState::ACTIVE);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::RemoveBlock(uint32_t blockIndex)
{
	m_blocks[blockIndex].m_blockDefinitionIndex = BlockDefinition::GetBlockDefinitionIndex("Air");
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
void Chunk::InitializeLighting(std::deque<BlockIterator>& dirtyLightingBlocks)
{
	// Boundary Blocks Loop
	for(int x = 0; x <= CHUNK_MAX_X; x++)
	{
		for(int y = 0; y <= CHUNK_MAX_Y; y++)
		{
			for(int z = 0; z <= CHUNK_MAX_Z; z++)
			{
				uint32_t blockIndex = LocalCoordsToIndex(x, y, z);
				if(m_blocks[blockIndex].IsBlockOpaque())
				{
					continue;
				}

				if((x == 0 && IsValidNeighbour(NEIGHBOUR_NEG_X)) || (x == CHUNK_MAX_X && IsValidNeighbour(NEIGHBOUR_POS_X)) ||
				   (y == 0 && IsValidNeighbour(NEIGHBOUR_NEG_Y)) || (y == CHUNK_MAX_Y && IsValidNeighbour(NEIGHBOUR_POS_Y)))
				{
					m_blocks[blockIndex].SetIsLightDirty(true);
				}
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

			for(int z = CHUNK_MAX_Z; z >= 0; z--)
			{
				uint32_t blockIndex = LocalCoordsToIndex(x, y, z);
				if(m_blocks[blockIndex].IsBlockOpaque())
				{
					break;
				}

				m_blocks[blockIndex].SetOutdoorLightInfluence(OUTDOOR_LIGHT_INFLUENCE_MAX);
				m_blocks[blockIndex].SetIsLightDirty(true);
				MarkNonSkyHorizontalBlockLightDirty(IntVec3(x, y, z));
			}
		}
	}

	// Glowstone Loop
	unsigned char glowstoneIndex = BlockDefinition::GetBlockDefinitionIndex("Glowstone");
	for(uint32_t i = 0; i < BLOCKS_PER_CHUNK; i++)
	{
		if(m_blocks[i].m_blockDefinitionIndex == glowstoneIndex)
		{
			int lightInfluence = BlockDefinition::GetIndoorLighting(glowstoneIndex);
			m_blocks[i].SetIndoorLightInfluence(static_cast<uint8_t>(lightInfluence));
			m_blocks[i].SetIsLightDirty(true);
		}

		if(m_blocks[i].IsLightDirty())
		{
			dirtyLightingBlocks.push_back(BlockIterator(this, i));
		}
	}
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Chunk::MarkNonSkyHorizontalBlockLightDirty(IntVec3 const& localCoords)
{	
	BlockIterator iterator;
	iterator.m_chunk = this;
	iterator.m_blockIndex = LocalCoordsToIndex(localCoords);

	Block* posXBlock = iterator.GetPosXBlock();
	Block* posYBlock = iterator.GetPosYBlock();
	Block* negXBlock = iterator.GetNegXBlock();
	Block* negYBlock = iterator.GetNegYBlock();
	
	if(posXBlock && !posXBlock->IsSky())
	{
		posXBlock->SetIsLightDirty(true);
	}
	if(posYBlock && !posYBlock->IsSky())
	{
		posYBlock->SetIsLightDirty(true);
	}
	if(negXBlock && !negXBlock->IsSky())
	{
		negXBlock->SetIsLightDirty(true);
	}
	if(negYBlock && !negYBlock->IsSky())
	{
		negYBlock->SetIsLightDirty(true);
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
    return m_blocks[blockIndex].m_blockDefinitionIndex == BlockDefinition::GetBlockDefinitionIndex("Water");
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

