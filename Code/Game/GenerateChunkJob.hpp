#pragma once
#include "Engine/JobSystem/Job.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Chunk;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class GenerateChunkJob : public Job
{
public:
					GenerateChunkJob(Chunk* chunkToGenerate);
	virtual			~GenerateChunkJob();

	virtual void	Execute() override;

public:
	
	Chunk*			m_chunkToGenerate = nullptr;

};