#include "Game/LoadChunkJob.hpp"
#include "Game/Chunk.hpp"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
LoadChunkJob::LoadChunkJob(Chunk* chunkToLoad)
	: Job()
	, m_chunkToLoad(chunkToLoad)
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
LoadChunkJob::~LoadChunkJob()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void LoadChunkJob::Execute()
{
	DebuggerPrintf("[Load Chunk Job] Working...\n");
	m_chunkToLoad->m_chunkState = ChunkState::LOADING;
	m_chunkToLoad->TryLoadFromFile();
	m_chunkToLoad->m_chunkState = ChunkState::LOADED;
	DebuggerPrintf("[Load Chunk Job] Work Finished!\n");
}
