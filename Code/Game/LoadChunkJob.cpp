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
	m_chunkToLoad->m_chunkState = ChunkState::LOADING;
	if(!m_chunkToLoad->TryLoadFromFile())
	{
		m_chunkToLoad->InitializeBlocks();
	}
}
