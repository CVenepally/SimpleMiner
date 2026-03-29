#pragma once
#include "Engine/JobSystem/Job.hpp"

class Chunk;

class SaveChunkJob : public Job
{
public:
					SaveChunkJob(Chunk* chunk);
	virtual			~SaveChunkJob();

	virtual void	Execute() override;

public:
	
	Chunk* m_chunkToSave;

};