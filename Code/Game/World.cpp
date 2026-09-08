#include "Game/World.hpp"
#include "Game/Game.hpp"
#include "Game/Chunk.hpp"
#include "Game/Block.hpp"
#include "Game/BlockDefinition.hpp"
#include "Game/BlockIterator.hpp"
#include "Game/GenerateChunkJob.hpp"
#include "Game/LoadChunkJob.hpp"
#include "Game/SaveChunkJob.hpp"
#include "Game/Player.hpp"

#include "Engine/Core/DebugRender.hpp"
#include "Engine/Core/Time.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Shader.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "ThirdParty/Noise/SmoothNoise.hpp"

#include <unordered_set>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
double g_cancelJobsTime				= 0.;
double g_pushPendingJobsTime		= 0.;
double g_generateChunkJobsTime		= 0.;
double g_deactivateAndSaveJobsTime	= 0.;
double g_completedJobsTime			= 0.;
double g_activateJobsTime			= 0.;
double g_buildMeshTime				= 0.;
double g_lightingProcess			= 0.;
double g_lightingInitTotal			= 0.;
double g_lightingInitAvg			= 0.;
double g_updateTime					= 0.;
double g_renderTime					= 0.;

//------------------------------------------------------------------------------------------------------------------
World::World(Clock* gameClock)
{
// 	g_continentalnessCurve.AddCurve(g_deepOceanCurve, g_deepOceanCurveStartTime);
// 	g_continentalnessCurve.AddCurve(g_oceanCurve, g_oceanCurveStartTime);
// 	g_continentalnessCurve.AddCurve(g_coastCurve, g_coastCurveStartTime);
// 	g_continentalnessCurve.AddCurve(g_nearInlandCurve, g_nearInlandStartTime);
// 	g_continentalnessCurve.AddCurve(g_midInlandCurve, g_midInlandStartTime);
// 	g_continentalnessCurve.AddCurve(g_farInlandCurve, g_farInlandStartTime);

	g_continentalnessCurve = PieceWiseCurves();
	g_continentalnessCurve.AddCurve_new(g_deepOceanCurve);
	g_continentalnessCurve.AddCurve_new(g_oceanCurve);
	g_continentalnessCurve.AddCurve_new(g_coastCurve);
	g_continentalnessCurve.AddCurve_new(g_nearInlandCurve);
	g_continentalnessCurve.AddCurve_new(g_midInlandCurve);
	g_continentalnessCurve.AddCurve_new(g_farInlandCurve);
	g_continentalnessCurve.AddCurve_new(g_maxContinentalnessCurve);
	
	g_erosionCurve = PieceWiseCurves();
	g_erosionCurve.AddCurve_new(g_erosionL0);
	g_erosionCurve.AddCurve_new(g_erosionL1);
	g_erosionCurve.AddCurve_new(g_erosionL2);
	g_erosionCurve.AddCurve_new(g_erosionL3);
	g_erosionCurve.AddCurve_new(g_erosionL4);
	g_erosionCurve.AddCurve_new(g_erosionL5);
	g_erosionCurve.AddCurve_new(g_erosionL6);

	g_weirdnessCurve = PieceWiseCurves();
	g_weirdnessCurve.AddCurve_new(g_weirdnessValleys);
	g_weirdnessCurve.AddCurve_new(g_weirdnessLow    );
	g_weirdnessCurve.AddCurve_new(g_weirdnessMid    );
	g_weirdnessCurve.AddCurve_new(g_weirdnessHigh   );
	g_weirdnessCurve.AddCurve_new(g_weirdnessPeaks  );
	g_weirdnessCurve.AddCurve_new(g_weirdnessMax);

	m_worldShader = g_theRenderer->CreateOrGetShader("Data/Shaders/WorldShader", InputLayoutType::VERTEX_PCUTBN);
	m_worldCB = g_theRenderer->CreateConstantBuffer(sizeof(WorldConstants));

	m_worldClock = new Clock(*gameClock, "World Clock");
	m_worldClock->SetTimeScale(GAME_SECONDS_PER_REAL_TIME_SECONDS);

	double physicsTimeStep = static_cast<double>(1.f / 120.f);

	m_physicsTimer = Timer(physicsTimeStep, &Clock::GetSystemClock());
	m_physicsTimer.Start();
}

//------------------------------------------------------------------------------------------------------------------
World::~World()
{
	delete m_worldClock;
	m_worldClock = nullptr;

	delete m_worldCB;
	m_worldCB = nullptr;

	DeactivateAndSaveChunks_Job(true);

	g_jobSystem->WaitForJobsToFinish();

	{
		std::scoped_lock<std::mutex> lock(m_activeChunksMutex);
		m_activeChunks.clear();
	}
}

//------------------------------------------------------------------------------------------------------------------
void World::Update()
{
	DebugJobInfo();

	g_activateJobsTime = 0.;
	g_lightingInitTotal = 0.;
	g_lightingInitAvg = 0.;
	g_lightingProcess = 0.;

	double startTime = GetCurrentTimeSeconds();

	int activeChunksSize = 0;
	int pendingMeshSize = 0;
	{
		std::scoped_lock<std::mutex> lock1(m_activeChunksMutex);
		std::scoped_lock<std::mutex> lock2(m_pendingMeshBuildingMutex);
		activeChunksSize = static_cast<int>(m_activeChunks.size());
		pendingMeshSize = static_cast<int>(m_pendingMeshBuilding.size());
	}

	TryCancelJobs();
	PushPendingJobs();
	GenerateChunks_Job();
	DeactivateAndSaveChunks_Job();
	GetCompletedJobs();
	ActivateChunk();
	if(g_enableLighting)
	{
		ProcessDirtyLighting();
	}
	
	BuildMesh_MainThread();

	UpdateEntities();

	if(m_physicsTimer.DecrementPeriodIfElapsed())
	{
		PhysicsUpdate();
	}

	if(g_inputSystem->IsKeyDown('Y'))
	{
		m_worldClock->SetTimeScale(GAME_SECONDS_PER_REAL_TIME_SECONDS * TIME_OF_DAY_MULTIPLIER);
	}
	else
	{
		m_worldClock->SetTimeScale(GAME_SECONDS_PER_REAL_TIME_SECONDS);
	}

	SetSkyColor();

	g_updateTime = (GetCurrentTimeSeconds() - startTime) * 1000.;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::UpdateEntities()
{
	for(Entity* entity : m_allEntities)
	{
		if(entity)
		{
			entity->Update();
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::PhysicsUpdate()
{
	float deltaSeconds = static_cast<float>(m_physicsTimer.m_period);
//	float deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();

	for(Entity* entity : m_allEntities)
	{
		if(entity)
		{
			entity->PhysicsUpdate(deltaSeconds);
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::DebugJobInfo()
{
	if(Game::ShouldDisplayJobInfo())
	{
		IntVec2 dims = g_theWindow->GetClientDimensions();
		AABB2 textBox = AABB2(IntVec2(0, 0), dims);

		int numPendingLoad = 0;
		int numLoading = 0;
		int numPendingGenerate = 0;
		int numGenerating = 0;
		int numPendingSave = 0;
		int numSaving = 0;

		{
			std::scoped_lock<std::mutex> lock(m_pendingActivationMutex);
			for(auto const& [COORD, chunk] : m_pendingActivation)
			{
				if(chunk)
				{
					if(chunk->m_chunkState == ChunkState::PENDING_GENERATE)
					{
						numPendingGenerate += 1;
					}
					else if(chunk->m_chunkState == ChunkState::GENERATING)
					{
						numGenerating += 1;
					}
					else if(chunk->m_chunkState == ChunkState::PENDING_LOAD)
					{
						numPendingLoad += 1;
					}
					else if(chunk->m_chunkState == ChunkState::LOADING)
					{
						numLoading += 1;
					}
				}
			}
		}

		DebugAddScreenText("Chunks", textBox, 14.f, Vec2(0.003f, 0.37f), 0.f, Rgba8::YELLOW);
		DebugAddScreenText(
			Stringf(
				"Pending Save:      %d\n"
				"Saving:            %d\n"
				"Pending Load:      %d\n"
				"Loading:           %d\n"
				"Pending Generate:  %d\n"
				"Generating:        %d\n"
				"Pending Build:     %d\n"
				"Active Chunks:     %d",
				numPendingSave,
				numSaving,
				numPendingLoad,
				numLoading,
				numPendingGenerate,
				numGenerating,
				static_cast<int>(m_pendingMeshBuilding.size()),
				static_cast<int>(m_activeChunks.size())
			),
			textBox, 14.f, Vec2(0.003f, 0.29f), 0.f, Rgba8::WHITE
		);

		DebugAddScreenText("Job System", textBox, 14.f, Vec2(0.003f, 0.24f), 0.f, Rgba8::YELLOW);
		DebugAddScreenText(
			Stringf(
				"Pending Jobs:    %d\n"
				"Executing Jobs:  %d\n"
				"Completed Jobs:  %d",
				g_jobSystem->GetNumPendingJobs(),
				g_jobSystem->GetNumExecutingJobs(),
				g_jobSystem->GetNumCompletedJobs()
			),
			textBox, 14.f, Vec2(0.003f, 0.205f), 0.f, Rgba8::WHITE
		);

		std::string times = Stringf(
			"   Cancel Jobs Time:         %0.2f\n"
			"   Push Pending Jobs Time:   %0.2f\n"
			"   Generate Chunk Jobs Time: %0.2f\n"
			"   Deactivate/Save Jobs:     %0.2f\n"
			"   Completed Jobs Time:      %0.2f\n"
			"       Activate Jobs Time:   %0.2f\n"
			"           Init Lighting Time: %0.2f\n"
			"       Init Lighting Avg:    %0.2f\n"
			"   Process Lighting Time:    %0.2f\n"
			"   Build Mesh Time:          %0.2f\n",
			g_cancelJobsTime,
			g_pushPendingJobsTime,
			g_generateChunkJobsTime,
			g_deactivateAndSaveJobsTime,
			g_completedJobsTime,
			g_activateJobsTime,
			g_lightingInitTotal,
			g_lightingInitAvg,
			g_lightingProcess,
			g_buildMeshTime
		);

		DebugAddScreenText("Time: ", textBox, 14.f, Vec2(0.003f, 0.17f), 0.f, Rgba8::YELLOW);
		DebugAddScreenText(Stringf("Update Time: %0.2f\n%s\n\nRender Time: %0.2f", g_updateTime, times.c_str(), g_renderTime),
						   textBox, 14.f, Vec2(0.003f, 0.01f), 0.f, Rgba8::WHITE);
	}
}

//------------------------------------------------------------------------------------------------------------------
void World::Render()
{
	double startTime = GetCurrentTimeSeconds();

	g_theRenderer->ClearScreen(m_skyColor);

	BindWorldConstant();
	
	g_theRenderer->BindShader(m_worldShader);
	{
		std::scoped_lock<std::mutex> lock(m_activeChunksMutex);
		for(auto const& [key, chunk] : m_activeChunks)
		{
			if(!chunk)
			{
				continue;
			}

			chunk->Render();
			if(Game::IsDebugMode())
			{
				DebugAddWorldWireframeBox(chunk->m_chunkBounds.m_mins, chunk->m_chunkBounds.m_maxs, 0.f);
			}
		}
	}
	

	if(Game::IsDebugMode())
	{
		std::string worldInfo = GetWorldDebugInfo();

		IntVec2 dims = g_theWindow->GetClientDimensions();
		AABB2 textBox = AABB2(IntVec2(0, 0), dims);

		DebugAddScreenText(worldInfo, textBox, 12.f, Vec2(0.f, 0.94f), 0.f, Rgba8::WHITE);
	}

	g_renderTime = (GetCurrentTimeSeconds() - startTime) * 1000.;
}

//------------------------------------------------------------------------------------------------------------------
std::string World::GetWorldDebugInfo() const
{
// 	std::scoped_lock<std::mutex> lock(m_activeChunksMutex);

	int chunkCount = static_cast<int>(m_activeChunks.size());
	int totalChunkVerts = 0;
	int totalChunkVertIndexes = 0;

	for(auto const& [key, chunk] : m_activeChunks)
	{
		totalChunkVerts += static_cast<int>(chunk->m_chunkVerts.size());
		totalChunkVertIndexes += static_cast<int>(chunk->m_indices.size());
	}

	return Stringf("Chunks: %d\nTotal Vertices: %d\nTotal Indexes: %d", chunkCount, totalChunkVerts, totalChunkVertIndexes);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::MarkChunkDirty(Chunk* chunk)
{	
	if(!chunk || chunk->m_chunkState == ChunkState::ACTIVE_DIRTY)
	{
		return; 
	}

	chunk->m_chunkState = ChunkState::ACTIVE_DIRTY;

	{
		std::scoped_lock<std::mutex> lock(m_pendingMeshBuildingMutex);
		m_pendingMeshBuilding[chunk->m_chunkCoords] = chunk;
	}		
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool World::GenerateClosestChunkMesh()
{
	DebuggerPrintf("[Mesh Build] Building Mesh\n");

	IntVec3 playerPos = Game::GetPlayerPosition().GetAsIntVec3();
	IntVec2 playerChunkCoords = GetChunkCoords(playerPos);

	Chunk* closestChunk = nullptr;
	int closestDistance = INT_MAX;

	{
		std::scoped_lock<std::mutex> lock(m_pendingMeshBuildingMutex);

		for(auto iter = m_pendingMeshBuilding.begin(); iter != m_pendingMeshBuilding.end(); ++iter)
		{
			Chunk* chunk = iter->second;

			int distance = GetVectorDistanceSquared2D(iter->first, playerChunkCoords);
			if(distance < closestDistance && chunk->m_chunkState == ChunkState::ACTIVE_DIRTY)
			{
				if(chunk->AreAllNeighborsValid())
				{
					closestChunk = chunk;
					closestDistance = distance;
				}
			}
		}
	}

	if(closestChunk)
	{

		{
			std::scoped_lock<std::mutex> lock(m_pendingMeshBuildingMutex);
			closestChunk->BuildChunkMesh();
			m_pendingMeshBuilding.erase(closestChunk->m_chunkCoords);
		}
		return true;
	}

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::DigBlock(BlockIterator const& blockIter)
{
	Chunk*		chunk		= blockIter.m_chunk;
	uint32_t	blockIndex	= blockIter.m_blockIndex;
	
	if(chunk && IsChunkActive(chunk))
	{
		chunk->RemoveBlock(blockIndex);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool World::PlaceBlock(BlockIterator const& blockIter, unsigned char type)
{
	Chunk* chunk = blockIter.m_chunk;
	uint32_t	blockIndex = blockIter.m_blockIndex;

	if(chunk && IsChunkActive(chunk))
	{
		chunk->PlaceBlockOfType(blockIndex, type);
		return true;
	}

	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool World::IsChunkActive(Chunk* chunk)
{
	std::scoped_lock<std::mutex> lock(m_activeChunksMutex);
	auto iter = m_activeChunks.find(chunk->m_chunkCoords);

	return iter != m_activeChunks.end();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Chunk* World::GetChunk(IntVec2 chunkCoord)
{
	std::scoped_lock<std::mutex> lock(m_activeChunksMutex);
	auto iter = m_activeChunks.find(chunkCoord);
	if(iter != m_activeChunks.end())
	{
		return iter->second;
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::FindNeighboursForChunk(Chunk* chunk)
{
	IntVec2 neighbourCoords[NEIGHBOUR_COUNT] = {};
	neighbourCoords[NEIGHBOUR_POS_X] = IntVec2(chunk->m_chunkCoords.x + 1, chunk->m_chunkCoords.y);
	neighbourCoords[NEIGHBOUR_POS_Y] = IntVec2(chunk->m_chunkCoords.x, chunk->m_chunkCoords.y + 1);
	neighbourCoords[NEIGHBOUR_NEG_X] = IntVec2(chunk->m_chunkCoords.x - 1, chunk->m_chunkCoords.y);
	neighbourCoords[NEIGHBOUR_NEG_Y] = IntVec2(chunk->m_chunkCoords.x, chunk->m_chunkCoords.y - 1);

	for(int i = 0; i < NEIGHBOUR_COUNT; ++i)
	{
		Chunk* neighbour = nullptr;
		{
			std::scoped_lock<std::mutex> lock(m_activeChunksMutex);
			auto iter = m_activeChunks.find(neighbourCoords[i]);
			if(iter != m_activeChunks.end() && iter->second != nullptr)
			{
				neighbour = iter->second;
			}
		}

		if(neighbour)
		{
			chunk->AddNeighbour(neighbour, static_cast<Neighbours>(i));

			switch(static_cast<Neighbours>(i))
			{
			case NEIGHBOUR_POS_X:
				neighbour->AddNeighbour(chunk, NEIGHBOUR_NEG_X);
				break;
			case NEIGHBOUR_POS_Y:
				neighbour->AddNeighbour(chunk, NEIGHBOUR_NEG_Y);
				break;
			case NEIGHBOUR_NEG_X:
				neighbour->AddNeighbour(chunk, NEIGHBOUR_POS_X);
				break;
			case NEIGHBOUR_NEG_Y:
				neighbour->AddNeighbour(chunk, NEIGHBOUR_POS_Y);
				break;
			default:
				break;
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::GenerateChunks_Job()
{	
	double startTime	= GetCurrentTimeSeconds();
	int chunksToBuild = 0;
	int activeChunksSize = 0;
	int pendingActivationSize = 0;

	{
		std::scoped_lock<std::mutex> lock1(m_activeChunksMutex);
		activeChunksSize = static_cast<int>(m_activeChunks.size());

		if(activeChunksSize == MAX_ACTIVE_CHUNKS)
		{
			return;
		}
// 		chunksToBuild = MAX_ACTIVE_CHUNKS - activeChunksSize;

	}
	
	{
		std::scoped_lock<std::mutex> lock2(m_pendingActivationMutex);
		pendingActivationSize = static_cast<int>(m_pendingActivation.size());
		chunksToBuild = MAX_ACTIVE_CHUNKS - (activeChunksSize + pendingActivationSize);
	}

	if(chunksToBuild == 0)
	{
		return;
	}

	IntVec3 playerPos = Game::GetPlayerPosition().GetAsIntVec3();
	IntVec2 playerChunkCoords = GetChunkCoords(playerPos);

	int step = 0;

	while(step < CHUNK_ACTIVATION_RADIUS_X && chunksToBuild > 0)
	{
		for(int x = 0; x < step; x++)
		{
			int chunkCoordForwardX = playerChunkCoords.x + x;
			int chunkCoordBackwardX = playerChunkCoords.x - x;

			for(int y = 0; y < step; y++)
			{
				int     chunkCoordLeftY = playerChunkCoords.y + y;
				int     chunkCoordRightY = playerChunkCoords.y - y;
				IntVec2 chunkCoordA = IntVec2(chunkCoordForwardX, chunkCoordLeftY);
				IntVec2 chunkCoordB = IntVec2(chunkCoordForwardX, chunkCoordRightY);
				IntVec2 chunkCoordC = IntVec2(chunkCoordBackwardX, chunkCoordLeftY);
				IntVec2 chunkCoordD = IntVec2(chunkCoordBackwardX, chunkCoordRightY);

				IntVec2 chunks[4] = {chunkCoordA, chunkCoordB, chunkCoordC, chunkCoordD};

				for(auto const& coord : chunks)
				{
					bool shouldCreateChunk = false;

					{
						std::scoped_lock<std::mutex> lock1(m_activeChunksMutex);
						std::scoped_lock<std::mutex> lock2(m_pendingActivationMutex);

						shouldCreateChunk = m_activeChunks.find(coord) == m_activeChunks.end() &&
							m_pendingActivation.find(coord) == m_pendingActivation.end() &&
							IsChunkWithinActivationRange(coord, playerChunkCoords);
// 						shouldCreateChunk = m_activeChunks.find(coord) == m_activeChunks.end() && IsChunkWithinActivationRange(coord, playerChunkCoords);
					}

					if(shouldCreateChunk)
					{
						Chunk* newChunk = new Chunk(coord);
						chunksToBuild -= 1;

						{
							std::scoped_lock<std::mutex> lock(m_pendingActivationMutex);
							m_pendingActivation[coord] = newChunk;
						}

						if(newChunk->m_chunkState == ChunkState::PENDING_GENERATE)
						{
							Job* newJob = new GenerateChunkJob(newChunk);

							{
								std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
								if(m_numGenerateJobsInJobSystem < MAX_GENERATE_JOBS)
								{
									g_jobSystem->AddJob(newJob);
									m_queuedActivateJobsInJobSystemQueue.push_back(newJob);
									m_numGenerateJobsInJobSystem += 1;
								}
								else
								{
									m_generateJobsQueue.push_back(newJob);
								}
							}

							DebuggerPrintf("Added Generate Job\n");
						}
						else if(newChunk->m_chunkState == ChunkState::PENDING_LOAD)
						{
							Job* newJob = new LoadChunkJob(newChunk);

							{
								std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
								if(m_numLoadJobsInJobSystem < MAX_LOAD_JOBS)
								{
									g_jobSystem->AddJob(newJob);
									m_queuedActivateJobsInJobSystemQueue.push_back(newJob);
									m_numLoadJobsInJobSystem += 1;
								}
								else
								{
									m_loadJobsQueue.push_back(newJob);
								}
							}

							DebuggerPrintf("Added Load Job\n");
						}
					}
				}
			}
		}
		step += 1;
	}

	g_generateChunkJobsTime = (GetCurrentTimeSeconds() - startTime) * 1000.;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::DeactivateAndSaveChunks_Job(bool isShuttingDown)
{
	double startTime = GetCurrentTimeSeconds();

	IntVec3 playerPos = Game::GetPlayerPosition().GetAsIntVec3();
	IntVec2 playerChunkCoords = GetChunkCoords(playerPos);

	std::vector<Chunk*> chunksToDeactivate;

	{
		std::scoped_lock<std::mutex> lock(m_activeChunksMutex);

		for(auto iter = m_activeChunks.begin(); iter != m_activeChunks.end(); ++iter)
		{
			if(isShuttingDown || IsChunkOutsideDeactivationRange(iter->first, playerChunkCoords))
			{
				chunksToDeactivate.push_back(iter->second);
				{
					std::scoped_lock<std::mutex> pendingLock(m_pendingMeshBuildingMutex);
					auto pendingIter = m_pendingMeshBuilding.find(iter->first);
					
					if(pendingIter != m_pendingMeshBuilding.end())
					{
						m_pendingMeshBuilding.erase(pendingIter);
					}
				}
				
			}
		}
	}

	for(Chunk* chunk : chunksToDeactivate)
	{
		chunk->RemoveNeighbors();

		if(chunk->m_isModified)
		{
			chunk->m_chunkState = ChunkState::PENDING_SAVE;

			Job* newJob = new SaveChunkJob(chunk);

			{
				std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
				if(!isShuttingDown)
				{
					if(m_numSaveJobsInJobSystem < MAX_SAVE_JOBS)
					{
						g_jobSystem->AddJob(newJob);
						m_numSaveJobsInJobSystem += 1;
					}
					else
					{
						m_saveJobsQueue.push_back(newJob);
					}
				}
				else
				{
					g_jobSystem->AddJob(newJob);
					m_numSaveJobsInJobSystem += 1;
				}
			}

			{
				std::scoped_lock<std::mutex> lock(m_pendingDeactivationMutex);
				m_pendingDeactivation[chunk->m_chunkCoords] = chunk;
			}

			{
				std::scoped_lock<std::mutex> lock(m_activeChunksMutex);
				m_activeChunks.erase(chunk->m_chunkCoords);
			}
		}
		else
		{
			{
				std::scoped_lock<std::mutex> lock(m_activeChunksMutex);
				m_activeChunks.erase(chunk->m_chunkCoords);
			}
			delete chunk;
		}
	}
	g_deactivateAndSaveJobsTime = (GetCurrentTimeSeconds() - startTime) * 1000.;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::BuildMesh_MainThread()
{
	double startTime = GetCurrentTimeSeconds();

	int meshesBuilt = 0;

	while(meshesBuilt < MAX_CHUNK_MESHES_TO_BUILD)
	{
		bool isEmpty = false;
		{
			std::scoped_lock<std::mutex> lock(m_pendingMeshBuildingMutex);
			isEmpty = m_pendingMeshBuilding.empty();
		}

		if(isEmpty)
		{
			break;
		}

		if(GenerateClosestChunkMesh())
		{
			++meshesBuilt;
		}
		else
		{
			break;
		}
	}
	g_buildMeshTime = (GetCurrentTimeSeconds() - startTime) * 1000.;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::PushPendingJobs()
{
	double startTime = GetCurrentTimeSeconds();

	IntVec3 playerPos = Game::GetPlayerPosition().GetAsIntVec3();
	IntVec2 playerChunkCoords = GetChunkCoords(playerPos);

	// Push generate jobs
	while(true)
	{
		GenerateChunkJob* generateJob = nullptr;
		bool hasJobs = false;
		bool canPush = false;

		{
			std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
			hasJobs = !m_generateJobsQueue.empty();
			canPush = m_numGenerateJobsInJobSystem < MAX_GENERATE_JOBS;

			if(hasJobs && canPush)
			{
				generateJob = static_cast<GenerateChunkJob*>(m_generateJobsQueue.front());
				m_generateJobsQueue.pop_front();
			}
		}

		if(!hasJobs || !canPush)
		{
			break;
		}

		if(!IsChunkWithinActivationRange(generateJob->m_chunkToGenerate->m_chunkCoords, playerChunkCoords))
		{
// 			{
// 				std::scoped_lock<std::mutex> lock(m_pendingActivationMutex);
// 				m_pendingActivation.erase(generateJob->m_chunkToGenerate->m_chunkCoords);
// 			}
			delete generateJob->m_chunkToGenerate;
			delete generateJob;
			continue;
		}

		g_jobSystem->AddJob(generateJob);

		{
			std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
			m_queuedActivateJobsInJobSystemQueue.push_back(generateJob);
			m_numGenerateJobsInJobSystem += 1;
		}
	}

	// Push load jobs
	while(true)
	{
		LoadChunkJob* loadJob = nullptr;
		bool hasJobs = false;
		bool canPush = false;

		{
			std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
			hasJobs = !m_loadJobsQueue.empty();
			canPush = m_numLoadJobsInJobSystem < MAX_LOAD_JOBS;

			if(hasJobs && canPush)
			{
				loadJob = static_cast<LoadChunkJob*>(m_loadJobsQueue.front());
				m_loadJobsQueue.pop_front();
			}
		}

		if(!hasJobs || !canPush)
		{
			break;
		}

		if(!IsChunkWithinActivationRange(loadJob->m_chunkToLoad->m_chunkCoords, playerChunkCoords))
		{
// 			{
// 				std::scoped_lock<std::mutex> lock(m_pendingActivationMutex);
// 				m_pendingActivation.erase(loadJob->m_chunkToLoad->m_chunkCoords);
// 			}
			delete loadJob->m_chunkToLoad;
			delete loadJob;
			continue;
		}

		g_jobSystem->AddJob(loadJob);

		{
			std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
			m_queuedActivateJobsInJobSystemQueue.push_back(loadJob);
			m_numLoadJobsInJobSystem += 1;
		}
	}

	// Push save jobs
	while(true)
	{
		SaveChunkJob* saveJob = nullptr;
		bool hasJobs = false;
		bool canPush = false;

		{
			std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
			hasJobs = !m_saveJobsQueue.empty();
			canPush = m_numSaveJobsInJobSystem < MAX_SAVE_JOBS;

			if(hasJobs && canPush)
			{
				saveJob = static_cast<SaveChunkJob*>(m_saveJobsQueue.front());
				m_saveJobsQueue.pop_front();
			}
		}

		if(!hasJobs || !canPush)
		{
			break;
		}

		g_jobSystem->AddJob(saveJob);

		{
			std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
			m_numSaveJobsInJobSystem += 1;
		}
	}

	g_pushPendingJobsTime = (GetCurrentTimeSeconds() - startTime) * 1000.;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::TryCancelJobs()
{
	double startTime = GetCurrentTimeSeconds();

	IntVec3 playerPos = Game::GetPlayerPosition().GetAsIntVec3();
	IntVec2 playerChunkCoords = GetChunkCoords(playerPos);

	std::vector<Job*> jobsToProcess;
	{
		std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
		jobsToProcess = m_queuedActivateJobsInJobSystemQueue;
	}

	for(Job* job : jobsToProcess)
	{
		GenerateChunkJob* genJob = dynamic_cast<GenerateChunkJob*>(job);
		if(genJob)
		{
			IntVec2 chunkCoords = genJob->m_chunkToGenerate->m_chunkCoords;
			if(!IsChunkWithinActivationRange(chunkCoords, playerChunkCoords))
			{
				bool didCancel = g_jobSystem->CancelPendingJob(job);
				if(didCancel)
				{
// 					{
// 						std::scoped_lock<std::mutex> lock(m_pendingActivationMutex);
// 						m_pendingActivation.erase(chunkCoords);
// 					}
// 
					{
						std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
						auto iter = std::find(m_queuedActivateJobsInJobSystemQueue.begin(),
											  m_queuedActivateJobsInJobSystemQueue.end(), job);
						if(iter != m_queuedActivateJobsInJobSystemQueue.end())
						{
							m_queuedActivateJobsInJobSystemQueue.erase(iter);
						}
						m_numGenerateJobsInJobSystem -= 1;
					}

					delete genJob->m_chunkToGenerate;
					delete genJob;

					continue;
				}
			}
		}

		LoadChunkJob* loadJob = dynamic_cast<LoadChunkJob*>(job);
		if(loadJob)
		{
			IntVec2 chunkCoords = loadJob->m_chunkToLoad->m_chunkCoords;
			if(!IsChunkWithinActivationRange(chunkCoords, playerChunkCoords))
			{
				bool didCancel = g_jobSystem->CancelPendingJob(job);
				if(didCancel)
				{
// 					{
// 						std::scoped_lock<std::mutex> lock(m_pendingActivationMutex);
// 						m_pendingActivation.erase(chunkCoords);
// 					}

					{
						std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
						auto iter = std::find(m_queuedActivateJobsInJobSystemQueue.begin(),
											  m_queuedActivateJobsInJobSystemQueue.end(), job);
						if(iter != m_queuedActivateJobsInJobSystemQueue.end())
						{
							m_queuedActivateJobsInJobSystemQueue.erase(iter);
						}
						m_numLoadJobsInJobSystem -= 1;
					}

					delete loadJob->m_chunkToLoad;
					delete loadJob;

					continue;
				}
			}
		}
	}

	g_cancelJobsTime = (GetCurrentTimeSeconds() - startTime) * 1000.;

}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::GetCompletedJobs()
{
	double startTime = GetCurrentTimeSeconds();
	
	std::vector<Job*> completedJobs = g_jobSystem->RetrieveCompletedJobs();
	g_jobSystem->RetrieveEarliestCompletedJob();

	int activatedChunks = 0;

	for(Job* job : completedJobs)
	{
		GenerateChunkJob* genJob = dynamic_cast<GenerateChunkJob*>(job);
		if(genJob)
		{
			Chunk* chunk = genJob->m_chunkToGenerate;
// 			ActivateChunk(chunk);

			{
				std::scoped_lock<std::mutex> lock(m_pendingActivationMutex);
				m_pendingActivation[chunk->m_chunkCoords] = chunk;
			}

			DeleteJob(genJob);
//			activatedChunks++;
			{
				std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
				m_numGenerateJobsInJobSystem -= 1;
			}
			continue;
		}

		LoadChunkJob* loadJob = dynamic_cast<LoadChunkJob*>(job);
		if(loadJob)
		{
			Chunk* chunk = loadJob->m_chunkToLoad;
// 			ActivateChunk(chunk);
			{
				std::scoped_lock<std::mutex> lock(m_pendingActivationMutex);
				m_pendingActivation[chunk->m_chunkCoords] = chunk;
			}

			DeleteJob(loadJob);
//			activatedChunks++;

			{
				std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
				m_numLoadJobsInJobSystem -= 1;
			}
			continue;
		}

		SaveChunkJob* saveJob = dynamic_cast<SaveChunkJob*>(job);
		if(saveJob)
		{
			Chunk* chunk = saveJob->m_chunkToSave;

			{
				std::scoped_lock<std::mutex> lock(m_pendingDeactivationMutex);
				m_pendingDeactivation.erase(chunk->m_chunkCoords);
			}

			DeleteJob(saveJob);

			{
				std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);
				m_numSaveJobsInJobSystem -= 1;
			}
			continue;
		}
	}
	
	g_completedJobsTime = (GetCurrentTimeSeconds() - startTime) * 1000.;
	g_lightingInitAvg = g_lightingInitTotal / activatedChunks;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
// void World::ActivateChunk(Chunk* chunk)
// {
// 	double startTime = GetCurrentTimeSeconds();
// 
// 	if(chunk && (chunk->m_chunkState == ChunkState::GENERATED || chunk->m_chunkState == ChunkState::LOADED))
// 	{
// 		{
// 			std::scoped_lock<std::mutex> lock(m_pendingActivationMutex);
// 			m_pendingActivation.erase(chunk->m_chunkCoords);
// 		}
// 
// 		{
// 			std::scoped_lock<std::mutex> lock(m_activeChunksMutex);
// 			m_activeChunks[chunk->m_chunkCoords] = chunk;
// 		}
// 
// 		MarkChunkDirty(chunk);
// 
// 		FindNeighboursForChunk(chunk);
// 		
// 		double lightingStartTime = GetCurrentTimeSeconds();
// 		if(g_enableLighting)
// 		{
// 			chunk->InitializeLighting(m_dirtyLighting);
// 		}
// 		g_lightingInitTotal += (GetCurrentTimeSeconds() - lightingStartTime) * 1000.;
// 	
// 	}
// 
// 	g_activateJobsTime += (GetCurrentTimeSeconds() - startTime) * 1000.;
// }

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::ActivateChunk()
{
	double startTime = GetCurrentTimeSeconds();

	Chunk* chunkToActivate = nullptr;

	IntVec2 playerPos = Game::GetPlayerPosition().GetAsIntVec3().GetXY2D();

	{
		std::scoped_lock<std::mutex> lock(m_pendingActivationMutex);

		auto bestIt = m_pendingActivation.end();
		int bestDistanceSq = INT_MAX;

		for(auto iter = m_pendingActivation.begin(); iter != m_pendingActivation.end(); ++iter)
		{
			Chunk* chunk = iter->second;
			if(!chunk)
			{
				continue;
			}

			if(chunk->m_chunkState != ChunkState::GENERATED && chunk->m_chunkState != ChunkState::LOADED)
			{
				continue;
			}

			IntVec2 chunkCenter = GetChunkCenter(iter->first);
			int distanceSq = (chunkCenter - playerPos).GetLengthSquared();
			if(distanceSq < bestDistanceSq)
			{
				bestIt = iter;
				bestDistanceSq = distanceSq;
			}
		}

		if(bestIt != m_pendingActivation.end())
		{
			chunkToActivate = bestIt->second;
			m_pendingActivation.erase(bestIt);
		}

// 		for(auto const& iter : m_pendingActivation)
// 		{
// 			if(iter.second->m_chunkState == ChunkState::GENERATED || iter.second->m_chunkState == ChunkState::LOADED)
// 			{
// 				chunkToActivate = iter.second;
// 				m_pendingActivation.erase(chunkToActivate->m_chunkCoords);
// 				break;
// 			}
// 		}
	}		

	if(chunkToActivate)
	{
		{
			std::scoped_lock<std::mutex> lock(m_activeChunksMutex);
			m_activeChunks[chunkToActivate->m_chunkCoords] = chunkToActivate;
		}

		MarkChunkDirty(chunkToActivate);

		FindNeighboursForChunk(chunkToActivate);

		double lightingStartTime = GetCurrentTimeSeconds();
		if(g_enableLighting)
		{
			chunkToActivate->InitializeLighting(m_dirtyLighting);
		}
		g_lightingInitTotal += (GetCurrentTimeSeconds() - lightingStartTime) * 1000.;

	}

	g_activateJobsTime += (GetCurrentTimeSeconds() - startTime) * 1000.;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::DeleteJob(Job* job)
{
	{
		std::scoped_lock<std::mutex> lock(m_jobQueuesMutex);

		for(auto iter = m_queuedActivateJobsInJobSystemQueue.begin();
			iter != m_queuedActivateJobsInJobSystemQueue.end(); ++iter)
		{
			if(job && job == *iter)
			{
				m_queuedActivateJobsInJobSystemQueue.erase(iter);
				break;
			}
		}
	}

	delete job;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::ProcessDirtyLighting()
{
	double startTime = GetCurrentTimeSeconds();
	while(!m_dirtyLighting.empty())
	{
		BlockIterator clearedBlockIter = ProcessNextDirtyLightBlock();
	}
	g_lightingProcess = (GetCurrentTimeSeconds() - startTime) * 1000.;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator World::ProcessNextDirtyLightBlock()
{
	BlockIterator dirtyBlockIterator = m_dirtyLighting.front();
	m_dirtyLighting.pop_front();
	Block& dirtyBlock = dirtyBlockIterator.m_chunk->m_blocks[dirtyBlockIterator.m_blockIndex];
	dirtyBlock.SetIsLightDirty(false);

	uint8_t correctIndoor = 0;
	uint8_t correctOutdoor = 0;

	if(dirtyBlock.IsSky())
	{
		correctOutdoor = OUTDOOR_LIGHT_INFLUENCE_MAX;
	}

	uint8_t emittedLight = static_cast<uint8_t>(BlockDefinition::GetIndoorLighting(dirtyBlock.m_blockDefinitionIndex));
	correctIndoor = emittedLight;

	if(!dirtyBlock.IsBlockOpaque())
	{
		uint8_t neighborIndoor[6] = {};
		uint8_t neighborOutdoor[6] = {};
		GetNeighborLightInfluences(dirtyBlockIterator, neighborIndoor, neighborOutdoor);

		uint8_t maxNeighborIndoor = GetMaxLightInfluence(neighborIndoor, 6);
		uint8_t maxNeighborOutdoor = GetMaxLightInfluence(neighborOutdoor, 6);

		if(maxNeighborIndoor > 0)
		{
			correctIndoor = GetMax(correctIndoor, static_cast<uint8_t>(maxNeighborIndoor - 1));
		}

		if(maxNeighborOutdoor > 0)
		{
			correctOutdoor = GetMax(correctOutdoor, static_cast<uint8_t>(maxNeighborOutdoor - 1));
		}
	}

	uint8_t currentIndoor = dirtyBlock.GetIndoorLightInfluence();
	uint8_t currentOutdoor = dirtyBlock.GetOutdoorLightInfluence();

	if(currentIndoor != correctIndoor || currentOutdoor != correctOutdoor)
	{
		dirtyBlock.SetIndoorLightInfluence(correctIndoor);
		dirtyBlock.SetOutdoorLightInfluence(correctOutdoor);

		MarkChunkDirty(dirtyBlockIterator.m_chunk);
		MarkNeighborBlockLightDirty(dirtyBlockIterator);
	}

	return dirtyBlockIterator;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::MarkLightingDirty(BlockIterator const& blockIter)
{
	Block& block = blockIter.m_chunk->m_blocks[blockIter.m_blockIndex];	
	if(block.IsLightDirty())
	{
		return;
	}

	block.SetIsLightDirty(true);
	m_dirtyLighting.push_back(blockIter);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Entity* World::SpawnEntityOfType(EntityType type, Vec3 const& position, EulerAngles const& orientation)
{
	Entity* newEntity = nullptr;

	switch(type)
	{
		case ENTITIY_TYPE_PLAYER:
		{
			float height = g_gameConfigBlackboard.GetValue("playerHeight", 1.f);
			float maxSpeed = g_gameConfigBlackboard.GetValue("playerMaxSpeed", 0.f);
			float width = g_gameConfigBlackboard.GetValue("playerWidth", 1.f);
			float eyeHeight = g_gameConfigBlackboard.GetValue("playerEyeHeight", 1.f);

			newEntity = new Player(position, orientation, maxSpeed, height, width, eyeHeight);
			break;
		}
		case ENTITY_TYPE_NPC:
			newEntity = nullptr;
			break;

		default:
			newEntity = nullptr;
			break;
	}
	AddEntityToWorld(newEntity);
	return newEntity;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::AddEntityToWorld(Entity* entity)
{
	if(entity == nullptr)
	{
		return;
	}

	for(Entity* currentEntity : m_allEntities)
	{
		if(currentEntity == nullptr)
		{
			m_allEntities.push_back(entity);
			return;
		}
	}

	m_allEntities.push_back(entity);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::GetNeighborLightInfluences(BlockIterator& blockIter, uint8_t* out_indoorInfluences, uint8_t* out_outdoorInfluences)
{
	Block* blocks[6] = {
		blockIter.GetPosXBlock(),
		blockIter.GetNegXBlock(),
		blockIter.GetPosYBlock(),
		blockIter.GetNegYBlock(),
		blockIter.GetPosZBlock(),
		blockIter.GetNegZBlock()
	};

	for(int i = 0; i < 6; ++i)
	{
		if(blocks[i])
		{
			out_indoorInfluences[i] = blocks[i]->GetIndoorLightInfluence();
			out_outdoorInfluences[i] = blocks[i]->GetOutdoorLightInfluence();
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t World::GetMaxLightInfluence(uint8_t* lightInfluences, int numLights)
{
	uint8_t maxInfluence = 0;
	for(int i = 0; i < numLights; ++i)
	{
		if(lightInfluences[i] > maxInfluence)
		{
			maxInfluence = lightInfluences[i];
		}
	}
	return maxInfluence;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::SetSkyColor()
{
	float worldTime = static_cast<float>(m_worldClock->GetTotalSeconds());
	float dayFraction = worldTime / TOTAL_GAME_SECONDS_PER_GAME_DAY;
	dayFraction = dayFraction - floorf(dayFraction);
// 
// 	IntVec2 dims = g_theWindow->GetClientDimensions();
// 	AABB2 textBox = AABB2(IntVec2(0, 0), dims);
// //	DebugAddScreenText(Stringf("Day Fraction: %0.2f", dayFraction), textBox, 14.f, Vec2(0.f, 0.90f), 0.f, Rgba8::YELLOW);

	if(dayFraction >= BASE_DUSK_FRACTION || dayFraction <= BASE_DAWN_FRACTION)
	{
		m_skyColor = DUSK_TO_DAWN_COLOR;
	}
	else if(dayFraction > BASE_DAWN_FRACTION && dayFraction <= BASE_NOON_FRACTION)
	{
		float rangeMappedDayFraction = RangeMapClamped(dayFraction, BASE_DAWN_FRACTION, BASE_NOON_FRACTION, 0.f, 1.f);
		m_skyColor = Rgba8::StaticColorLerp(DUSK_TO_DAWN_COLOR, NOON_COLOR, rangeMappedDayFraction);
	}
	else if(dayFraction > BASE_NOON_FRACTION && dayFraction <= BASE_DUSK_FRACTION)
	{
		float rangeMappedDayFraction = RangeMapClamped(dayFraction, BASE_NOON_FRACTION, BASE_DUSK_FRACTION, 0.f, 1.f);
		m_skyColor = Rgba8::StaticColorLerp(NOON_COLOR, DUSK_TO_DAWN_COLOR, rangeMappedDayFraction);
	}

	if(dayFraction < 0.5f)
	{
		float rangeMappedDayFraction = RangeMap(dayFraction, 0.f, 0.5f, 0.f, 1.f);
		m_outdoorColor = Rgba8::StaticColorLerp(OUT_DOOR_MIDNIGHT, OUT_DOOR_NOON, rangeMappedDayFraction);
	}
	else
	{
		float rangeMappedDayFraction = RangeMap(dayFraction, 0.5f, 1.f, 0.f, 1.f);
		m_outdoorColor = Rgba8::StaticColorLerp(OUT_DOOR_NOON, OUT_DOOR_MIDNIGHT, rangeMappedDayFraction);
	}

	if(dayFraction > BASE_DUSK_FRACTION)
	{
		float lightningPerlin = Compute1dPerlinNoise(worldTime, 200.f, 9);

		float lightningStrength = RangeMapClamped(lightningPerlin, 0.6f, 0.9f, 0.f, 1.f);

		m_outdoorColor = Rgba8::StaticColorLerp(m_outdoorColor, Rgba8::WHITE, lightningStrength);
		m_skyColor = Rgba8::StaticColorLerp(m_skyColor, Rgba8::WHITE, lightningStrength);
	}
	
	float glowPerlin = Compute1dPerlinNoise(worldTime, 500.f, 9);

	float glowStrength = RangeMapClamped(glowPerlin, -1.f, 1.f, 0.8f, 1.f);

	m_indoorLightColor = m_baseIndoorLightColor * glowStrength;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::UndirtyAllBlocksInChunk()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::MarkLightingDirtyIfNotOpaque(BlockIterator const& blockIter)
{
	Block& block = blockIter.m_chunk->m_blocks[blockIter.m_blockIndex];

	if(!block.IsBlockOpaque())
	{
		MarkLightingDirty(blockIter);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::MarkNeighborBlockLightDirty(BlockIterator& blockIter)
{
	IntVec3 localCoords = IndexToLocalCoords(blockIter.m_blockIndex);
	Chunk* currentChunk = blockIter.m_chunk;
	Chunk* posXChunk = nullptr;
	Chunk* posYChunk = nullptr;
	Chunk* negXChunk = nullptr;
	Chunk* negYChunk = nullptr;

	if(localCoords.x == 0)
	{
		negXChunk = blockIter.GetNegXChunk();
		MarkChunkDirty(negXChunk);
	}
	else if(localCoords.x == CHUNK_MAX_X)
	{
		posXChunk = blockIter.GetPosXChunk();
		MarkChunkDirty(posXChunk);
	}

	if(localCoords.y == 0)
	{
		negYChunk = blockIter.GetNegYChunk();
		MarkChunkDirty(negYChunk);
	}
	else if(localCoords.y == CHUNK_MAX_Y)
	{
		posYChunk = blockIter.GetPosYChunk();
		MarkChunkDirty(posYChunk);
	}

	Block* posXBlock = blockIter.GetPosXBlock();
	Block* negXBlock = blockIter.GetNegXBlock();
	Block* posYBlock = blockIter.GetPosYBlock();
	Block* negYBlock = blockIter.GetNegYBlock();
	Block* posZBlock = blockIter.GetPosZBlock();
	Block* negZBlock = blockIter.GetNegZBlock();

	if(posXBlock && !posXBlock->IsLightDirty())
	{
		if(posXChunk)
		{
			MarkLightingDirty(BlockIterator(posXChunk, LocalCoordsToIndex(0, localCoords.y, localCoords.z)));
		}
		else
		{
			MarkLightingDirty(BlockIterator(currentChunk, LocalCoordsToIndex(localCoords.x + 1, localCoords.y, localCoords.z)));
		}
	}

	if(negXBlock && !negXBlock->IsLightDirty())
	{
		if(negXChunk)
		{
			MarkLightingDirty(BlockIterator(negXChunk, LocalCoordsToIndex(CHUNK_MAX_X, localCoords.y, localCoords.z)));
		}
		else
		{
			MarkLightingDirty(BlockIterator(currentChunk, LocalCoordsToIndex(localCoords.x - 1, localCoords.y, localCoords.z)));
		}
	}

	if(posYBlock && !posYBlock->IsLightDirty())
	{
		if(posYChunk)
		{
			MarkLightingDirty(BlockIterator(posYChunk, LocalCoordsToIndex(localCoords.x, 0, localCoords.z)));
		}
		else
		{
			MarkLightingDirty(BlockIterator(currentChunk, LocalCoordsToIndex(localCoords.x, localCoords.y + 1, localCoords.z)));
		}
	}

	if(negYBlock && !negYBlock->IsLightDirty())
	{
		if(negYChunk)
		{
			MarkLightingDirty(BlockIterator(negYChunk, LocalCoordsToIndex(localCoords.x, CHUNK_MAX_Y, localCoords.z)));
		}
		else
		{
			MarkLightingDirty(BlockIterator(currentChunk, LocalCoordsToIndex(localCoords.x, localCoords.y - 1, localCoords.z)));
		}
	}

	if(posZBlock && !posZBlock->IsLightDirty())
	{
		MarkLightingDirty(BlockIterator(currentChunk, LocalCoordsToIndex(localCoords.x, localCoords.y, localCoords.z + 1)));
	}

	if(negZBlock && !negZBlock->IsLightDirty())
	{
		MarkLightingDirty(BlockIterator(currentChunk, LocalCoordsToIndex(localCoords.x, localCoords.y, localCoords.z - 1)));
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void World::BindWorldConstant()
{
	WorldConstants worldConsts;
	worldConsts.m_cameraPosition = Vec4(Game::GetPlayerPosition());
	worldConsts.m_distanceRange = g_fogRange;

	m_skyColor.GetAsFloat(worldConsts.m_skyColor);
	m_indoorLightColor.GetAsFloat(worldConsts.m_indoorLightColor);
	m_outdoorColor.GetAsFloat(worldConsts.m_outdoorLightColor);

	g_theRenderer->CopyCPUToGPU(&worldConsts, sizeof(WorldConstants), m_worldCB);
	g_theRenderer->BindConstantBuffer(g_worldConstRegister, m_worldCB);
}
