#include "Game/SaveChunkJob.hpp"
#include "Game/Chunk.hpp"
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SaveChunkJob::SaveChunkJob(Chunk* chunk)
	: m_chunkToSave(chunk)
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
SaveChunkJob::~SaveChunkJob()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void SaveChunkJob::Execute()
{
	m_chunkToSave->m_chunkState = ChunkState::SAVING;
	m_chunkToSave->SaveToFile();
	m_chunkToSave->m_chunkState = ChunkState::SAVED;
}
