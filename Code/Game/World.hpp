#pragma once
#include "Game/GameCommon.hpp"
#include "Game/BlockIterator.hpp"

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Timer.hpp"

#include <map>
#include <deque>
#include <vector>
#include <mutex>

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Chunk;
class Job;
class Shader;
class BlockIterator;
class Clock;
class ConstantBuffer;
class Entity;

typedef std::map<IntVec2, Chunk*>	ChunkMap;
typedef	std::deque<Job*>			JobQueue;
typedef std::deque<BlockIterator>	BlockIterQueue;	
typedef std::vector<Entity*>		EntityList;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class World
{
	friend class Game;

public:
	World(Clock* gameClock);
	~World();

	void Update();
	void UpdateEntities();
	void PhysicsUpdate();
	void DebugJobInfo();
	void Render();

	void DigBlock(BlockIterator const& blockIter);
	bool PlaceBlock(BlockIterator const& blockIter, unsigned char type);

	bool IsChunkActive(Chunk* chunk);

	Chunk* GetChunk(IntVec2 chunkCoord);

	std::string GetWorldDebugInfo() const;

	void MarkChunkDirty(Chunk* chunk);
	void MarkLightingDirty(BlockIterator const& blockIter);

	Entity* SpawnEntityOfType(EntityType type, Vec3 const& position, EulerAngles const& orientation);
	void	AddEntityToWorld(Entity* entity);

private:
	void GenerateChunks_Job();
	void DeactivateAndSaveChunks_Job(bool isShuttingDown = false);
	void BuildMesh_MainThread();
	void PushPendingJobs();
	void TryCancelJobs();
	void GetCompletedJobs();
	void BindWorldConstant();

	bool GenerateClosestChunkMesh();
	void FindNeighboursForChunk(Chunk* chunk);
// 	void ActivateChunk(Chunk* chunk);
	void ActivateChunk();
	void DeleteJob(Job* job);

	void			ProcessDirtyLighting();
	BlockIterator	ProcessNextDirtyLightBlock();
	void			UndirtyAllBlocksInChunk();
	void			MarkLightingDirtyIfNotOpaque(BlockIterator const& blockIter);
	void			MarkNeighborBlockLightDirty(BlockIterator& blockIter);

	void			GetNeighborLightInfluences(BlockIterator& blockIter, uint8_t* out_indoorInfluences, uint8_t* out_outdoorInfluences);
	
	uint8_t			GetMaxLightInfluence(uint8_t* lightInfluences, int numLights);

	void			SetSkyColor();

private:

	Shader*				m_worldShader = nullptr;
	ConstantBuffer*		m_worldCB = nullptr;

	EntityList			m_allEntities;

	Timer				m_physicsTimer;

	BlockIterQueue		m_dirtyLighting;

	ChunkMap			m_activeChunks;
	mutable std::mutex	m_activeChunksMutex;
	
	ChunkMap			m_pendingActivation;
	mutable std::mutex	m_pendingActivationMutex;
	
	ChunkMap			m_pendingMeshBuilding;
	mutable std::mutex	m_pendingMeshBuildingMutex;
	
	ChunkMap			m_pendingDeactivation;
	mutable std::mutex	m_pendingDeactivationMutex;

	JobQueue			m_generateJobsQueue;
	JobQueue			m_loadJobsQueue;
	JobQueue			m_saveJobsQueue;
	std::vector<Job*>	m_queuedActivateJobsInJobSystemQueue;

	Clock*				m_worldClock = nullptr;

	int					m_numGenerateJobsInJobSystem = 0;
	int					m_numLoadJobsInJobSystem = 0;
	int					m_numSaveJobsInJobSystem = 0;

	Rgba8				m_skyColor = Rgba8::BLACK;
	Rgba8				m_indoorLightColor = Rgba8(255, 230, 204);
	Rgba8				m_baseIndoorLightColor = Rgba8(255, 230, 204);
	Rgba8				m_outdoorColor = Rgba8::WHITE;

	mutable std::mutex m_jobQueuesMutex;
};