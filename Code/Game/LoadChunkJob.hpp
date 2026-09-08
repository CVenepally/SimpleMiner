#pragma once
#include "Engine/JobSystem/Job.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class Chunk;

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
class LoadChunkJob : public Job
{
public:
					LoadChunkJob(Chunk* chunkToLoad);
	virtual			~LoadChunkJob();

	virtual void	Execute() override;

public:

	Chunk*			m_chunkToLoad = nullptr;

};