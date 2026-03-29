#include "Game/GenerateChunkJob.hpp"
#include "Game/Chunk.hpp"

#include "Engine/Core/StringUtils.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
GenerateChunkJob::GenerateChunkJob(Chunk* chunkToGenerate)
	: Job()
	, m_chunkToGenerate(chunkToGenerate)
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
GenerateChunkJob::~GenerateChunkJob()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void GenerateChunkJob::Execute()
{
	DebuggerPrintf("[Generate Chunk Job] Working...\n");
	m_chunkToGenerate->m_chunkState = ChunkState::GENERATING;
	m_chunkToGenerate->InitializeBlocks();
	m_chunkToGenerate->m_chunkState = ChunkState::GENERATED;
	DebuggerPrintf("[Generate Chunk Job] Work Finished!\n");
}
