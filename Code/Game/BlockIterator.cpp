#include "Game/BlockIterator.hpp"
#include "Game/Game.hpp"
#include "Game/Block.hpp"
#include "Game/World.hpp"
#include "Game/Chunk.hpp"

//------------------------------------------------------------------------------------------------------------------
BlockIterator::BlockIterator(Chunk* chunk, uint32_t blockIndex)
    : m_chunk(chunk)
    , m_blockIndex(blockIndex)
{
}

//------------------------------------------------------------------------------------------------------------------
Directions BlockIterator::GetBlockingDirections(uint32_t blockIndex) const
{
    Directions blockingDirection = DIRECTION_NONE;

    IntVec3 localCoord = IndexToLocalCoords(blockIndex);

    IntVec3 posXBlockCoord = localCoord + IntVec3(1, 0, 0);
    IntVec3 posYBlockCoord = localCoord + IntVec3(0, 1, 0);
    IntVec3 posZBlockCoord = localCoord + IntVec3(0, 0, 1);
    IntVec3 negXBlockCoord = localCoord + IntVec3(-1, 0, 0);
    IntVec3 negYBlockCoord = localCoord + IntVec3(0, -1, 0);
    IntVec3 negZBlockCoord = localCoord + IntVec3(0, 0, -1);

    uint32_t posXIndex = LocalCoordsToIndex(posXBlockCoord);
    uint32_t posYIndex = LocalCoordsToIndex(posYBlockCoord);
    uint32_t posZIndex = LocalCoordsToIndex(posZBlockCoord);
    uint32_t negXIndex = LocalCoordsToIndex(negXBlockCoord);
    uint32_t negYIndex = LocalCoordsToIndex(negYBlockCoord);
    uint32_t negZIndex = LocalCoordsToIndex(negZBlockCoord);

    if ((IsValidLocalCoord(posXBlockCoord) && m_chunk->IsBlockOpaque(posXIndex)) || (!IsValidLocalCoord(posXBlockCoord) && g_enableCrossChunkHiddenSurfaceRemoval && IsBlockedByNeighborChunkBlock(localCoord, DIRECTION_POS_X)))
    {
        blockingDirection |= DIRECTION_POS_X;
    }
    
    if ((IsValidLocalCoord(posYBlockCoord) && m_chunk->IsBlockOpaque(posYIndex)) || (!IsValidLocalCoord(posYBlockCoord) && g_enableCrossChunkHiddenSurfaceRemoval && IsBlockedByNeighborChunkBlock(localCoord, DIRECTION_POS_Y)))
    {
        blockingDirection |= DIRECTION_POS_Y;
	}
	
    if (IsValidLocalCoord(posZBlockCoord) && m_chunk->IsBlockOpaque(posZIndex))
    {
        blockingDirection |= DIRECTION_POS_Z;
    }

    if ((IsValidLocalCoord(negXBlockCoord) && m_chunk->IsBlockOpaque(negXIndex)) || (!IsValidLocalCoord(negXBlockCoord) && g_enableCrossChunkHiddenSurfaceRemoval && IsBlockedByNeighborChunkBlock(localCoord, DIRECTION_NEG_X)))
    {
        blockingDirection |= DIRECTION_NEG_X;
	}

    if ((IsValidLocalCoord(negYBlockCoord) && m_chunk->IsBlockOpaque(negYIndex)) || (!IsValidLocalCoord(negYBlockCoord) && g_enableCrossChunkHiddenSurfaceRemoval && IsBlockedByNeighborChunkBlock(localCoord, DIRECTION_NEG_Y)))
    {
        blockingDirection |= DIRECTION_NEG_Y;
	}

    if (IsValidLocalCoord(negZBlockCoord) && m_chunk->IsBlockOpaque(negZIndex))
    {
        blockingDirection |= DIRECTION_NEG_Z;
    }

    return blockingDirection;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
bool BlockIterator::IsBlockedByNeighborChunkBlock(IntVec3 const& localCoord, Directions direction) const
{
	if(localCoord.x == CHUNK_MAX_X && direction & DIRECTION_POS_X)
	{
		IntVec3 neighborCoord(0, localCoord.y, localCoord.z);
		uint32_t posXNeighborIndex = LocalCoordsToIndex(neighborCoord);
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_POS_X);
		return neighbor->IsBlockOpaque(posXNeighborIndex);
	}
	else if(localCoord.x == 0 && direction & DIRECTION_NEG_X)
	{
		IntVec3 neighborCoord(CHUNK_MAX_X, localCoord.y, localCoord.z);
		uint32_t negXNeighborIndex = LocalCoordsToIndex(neighborCoord);
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_NEG_X);
		return neighbor->IsBlockOpaque(negXNeighborIndex);
	}
	else if(localCoord.y == CHUNK_MAX_Y && direction & DIRECTION_POS_Y)
	{
		IntVec3 neighborCoord(localCoord.x, 0, localCoord.z);
		uint32_t posYNeighborIndex = LocalCoordsToIndex(neighborCoord);
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_POS_Y);
		return neighbor->IsBlockOpaque(posYNeighborIndex);
	}
	else if(localCoord.y == 0 && direction & DIRECTION_NEG_Y)
	{
		IntVec3 neighborCoord(localCoord.x, CHUNK_MAX_Y, localCoord.z);
		uint32_t negYNeighborIndex = LocalCoordsToIndex(neighborCoord);
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_NEG_Y);
		return neighbor->IsBlockOpaque(negYNeighborIndex);
	}
	return false;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void BlockIterator::GetNeighboringBlockIterators(std::vector<BlockIterator>& neighbors)
{
	neighbors.clear();
	neighbors.reserve(6);
	IntVec3 globalCoords = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);

	IntVec3 posXNeighborCoords = globalCoords + IntVec3::FORWARD;
	IntVec3 posYNeighborCoords = globalCoords + IntVec3::LEFT;
	IntVec3 posZNeighborCoords = globalCoords + IntVec3::UP;
	IntVec3 negXNeighborCoords = globalCoords + IntVec3::BACKWARD;
	IntVec3 negYNeighborCoords = globalCoords + IntVec3::RIGHT;
	IntVec3 negZNeighborCoords = globalCoords + IntVec3::DOWN;

	BlockIterator posXIter = GetBlockIteratorForBlockGlobalCoord(posXNeighborCoords);
	BlockIterator posYIter = GetBlockIteratorForBlockGlobalCoord(posYNeighborCoords);
	BlockIterator posZIter = GetBlockIteratorForBlockGlobalCoord(posZNeighborCoords);
	BlockIterator negXIter = GetBlockIteratorForBlockGlobalCoord(negXNeighborCoords);
	BlockIterator negYIter = GetBlockIteratorForBlockGlobalCoord(negYNeighborCoords);
	BlockIterator negZIter = GetBlockIteratorForBlockGlobalCoord(negZNeighborCoords);
	
	if(IsValidIterator(posXIter))
	{
		neighbors.push_back(posXIter);
	}
	if(IsValidIterator(posYIter))
	{
		neighbors.push_back(posYIter);
	}
	if(IsValidIterator(posZIter))
	{
		neighbors.push_back(posZIter);
	}
	if(IsValidIterator(negXIter))
	{
		neighbors.push_back(negXIter);
	}
	if(IsValidIterator(negYIter))
	{
		neighbors.push_back(negYIter);
	}
	if(IsValidIterator(negZIter))
	{
		neighbors.push_back(negZIter);
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator BlockIterator::GetBlockIteratorForBlockGlobalCoord(IntVec3 const& blockCoord)
{
	IntVec2     chunkCoords = GetChunkCoords(blockCoord);
	Chunk*      blockChunk = Game::GetWorld()->GetChunk(chunkCoords);
	uint32_t    blockIndex = GlobalCoordsToIndex(blockCoord);
	
    return BlockIterator(blockChunk, blockIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetPosXBlock()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.x == CHUNK_MAX_X)
	{
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_POS_X);
		if(!neighbor)
		{
			return nullptr;
		}

		IntVec3 neighborCoord(0, localCoord.y, localCoord.z);
		uint32_t neighborIndex = LocalCoordsToIndex(neighborCoord);
		return &neighbor->GetBlock(neighborIndex);
	}

	uint32_t posXIndex = LocalCoordsToIndex(localCoord + IntVec3(1, 0, 0));
	return &m_chunk->GetBlock(posXIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetNegXBlock()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.x == 0)
	{
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_NEG_X);

		if(!neighbor)
		{
			return nullptr;
		}

		IntVec3 neighborCoord(CHUNK_MAX_X, localCoord.y, localCoord.z);
		uint32_t neighborIndex = LocalCoordsToIndex(neighborCoord);
		return &neighbor->GetBlock(neighborIndex);
	}

	uint32_t negXIndex = LocalCoordsToIndex(localCoord + IntVec3(-1, 0, 0));
	return &m_chunk->GetBlock(negXIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetPosYBlock()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.y == CHUNK_MAX_Y)
	{
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_POS_Y);
		if(!neighbor)
		{
			return nullptr;
		}

		IntVec3 neighborCoord(localCoord.x, 0, localCoord.z);
		uint32_t neighborIndex = LocalCoordsToIndex(neighborCoord);
		return &neighbor->GetBlock(neighborIndex);
	}

	uint32_t posYIndex = LocalCoordsToIndex(localCoord + IntVec3(0, 1, 0));
	return &m_chunk->GetBlock(posYIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetNegYBlock()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.y == 0)
	{
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_NEG_Y);
		if(!neighbor)
		{
			return nullptr;
		}

		IntVec3 neighborCoord(localCoord.x, CHUNK_MAX_Y, localCoord.z);
		uint32_t neighborIndex = LocalCoordsToIndex(neighborCoord);
		return &neighbor->GetBlock(neighborIndex);
	}

	uint32_t negYIndex = LocalCoordsToIndex(localCoord + IntVec3(0, -1, 0));
	return &m_chunk->GetBlock(negYIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetPosZBlock()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.z == CHUNK_MAX_Z)
	{
		return nullptr;
	}

	uint32_t posZIndex = LocalCoordsToIndex(localCoord + IntVec3(0, 0, 1));
	return &m_chunk->GetBlock(posZIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetNegZBlock()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.z == 0)
	{
		return nullptr;
	}

	uint32_t negZIndex = LocalCoordsToIndex(localCoord + IntVec3(0, 0, -1));
	return &m_chunk->GetBlock(negZIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetPosXPosYBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::FORWARD + IntVec3::LEFT;
	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetPosXNegYBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::FORWARD + IntVec3::RIGHT;
	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetNegXPosYBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::BACKWARD + IntVec3::LEFT;
	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetNegXNegYBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::BACKWARD + IntVec3::RIGHT;
	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}
//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetPosZPosXBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::UP + IntVec3::FORWARD;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetNegZPosXBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::DOWN + IntVec3::BACKWARD;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetNegZNegXBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::DOWN + IntVec3::BACKWARD;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetPosZNegXBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::UP + IntVec3::BACKWARD;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetPosYPosZBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::LEFT + IntVec3::UP;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetPosYNegZBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::LEFT + IntVec3::DOWN;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetNegYPosZBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::RIGHT + IntVec3::UP;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block* BlockIterator::GetNegYNegZBlock()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::RIGHT + IntVec3::DOWN;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return &neighborIter.GetBlockRef();
	}
	return nullptr;
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Chunk* BlockIterator::GetPosXChunk()
{
	return m_chunk->GetNeighborChunk(NEIGHBOUR_POS_X);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Chunk* BlockIterator::GetNegXChunk()
{
	return m_chunk->GetNeighborChunk(NEIGHBOUR_NEG_X);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Chunk* BlockIterator::GetPosYChunk()
{
	return m_chunk->GetNeighborChunk(NEIGHBOUR_POS_Y);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Chunk* BlockIterator::GetNegYChunk()
{
	return m_chunk->GetNeighborChunk(NEIGHBOUR_NEG_Y);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator BlockIterator::GetPosXBlockIterator()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.x == CHUNK_MAX_X)
	{
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_POS_X);
		if(!neighbor)
		{
			return BlockIterator(nullptr, 0);
		}

		IntVec3 neighborCoord(0, localCoord.y, localCoord.z);
		uint32_t neighborIndex = LocalCoordsToIndex(neighborCoord);
		return BlockIterator(neighbor, neighborIndex);
	}

	uint32_t posXIndex = LocalCoordsToIndex(localCoord + IntVec3(1, 0, 0));
	return BlockIterator(m_chunk, posXIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator BlockIterator::GetNegXBlockIterator()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.x == 0)
	{
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_NEG_X);
		if(!neighbor)
		{
			return BlockIterator(nullptr, 0);
		}

		IntVec3 neighborCoord(CHUNK_MAX_X, localCoord.y, localCoord.z);
		uint32_t neighborIndex = LocalCoordsToIndex(neighborCoord);
		return BlockIterator(neighbor, neighborIndex);
	}

	uint32_t negXIndex = LocalCoordsToIndex(localCoord + IntVec3(-1, 0, 0));
	return BlockIterator(m_chunk, negXIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator BlockIterator::GetPosYBlockIterator()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.y == CHUNK_MAX_Y)
	{
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_POS_Y);
		if(!neighbor)
		{
			return BlockIterator(nullptr, 0);
		}

		IntVec3 neighborCoord(localCoord.x, 0, localCoord.z);
		uint32_t neighborIndex = LocalCoordsToIndex(neighborCoord);
		return BlockIterator(neighbor, neighborIndex);
	}

	uint32_t posYIndex = LocalCoordsToIndex(localCoord + IntVec3(0, 1, 0));
	return BlockIterator(m_chunk, posYIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator BlockIterator::GetNegYBlockIterator()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.y == 0)
	{
		Chunk* neighbor = m_chunk->GetNeighborChunk(NEIGHBOUR_NEG_Y);
		if(!neighbor)
		{
			return BlockIterator(nullptr, 0);
		}

		IntVec3 neighborCoord(localCoord.x, CHUNK_MAX_Y, localCoord.z);
		uint32_t neighborIndex = LocalCoordsToIndex(neighborCoord);
		return BlockIterator(neighbor, neighborIndex);
	}

	uint32_t negYIndex = LocalCoordsToIndex(localCoord + IntVec3(0, -1, 0));
	return BlockIterator(m_chunk, negYIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator BlockIterator::GetPosZBlockIterator()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.z == CHUNK_MAX_Z)
	{
		return BlockIterator(nullptr, 0);
	}

	uint32_t posZIndex = LocalCoordsToIndex(localCoord + IntVec3(0, 0, 1));
	return BlockIterator(m_chunk, posZIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator BlockIterator::GetNegZBlockIterator()
{
	IntVec3 localCoord = IndexToLocalCoords(m_blockIndex);

	if(localCoord.z == 0)
	{
		return BlockIterator(nullptr, 0);
	}

	uint32_t negZIndex = LocalCoordsToIndex(localCoord + IntVec3(0, 0, -1));
	return BlockIterator(m_chunk, negZIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator BlockIterator::GetPosXPosYBlockIterator()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::FORWARD + IntVec3::LEFT;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return neighborIter;
	}

	return BlockIterator(nullptr, 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator BlockIterator::GetPosXNegYBlockIterator()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::FORWARD + IntVec3::RIGHT;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return neighborIter;
	}

	return BlockIterator(nullptr, 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator BlockIterator::GetNegXPosYBlockIterator()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::BACKWARD + IntVec3::LEFT;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return neighborIter;
	}

	return BlockIterator(nullptr, 0);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
BlockIterator BlockIterator::GetNegXNegYBlockIterator()
{
	IntVec3 globalCoord = GetGlobalCoords(m_chunk->GetChunkCoordinates(), m_blockIndex);
	IntVec3 neighborCoord = globalCoord + IntVec3::BACKWARD + IntVec3::RIGHT;

	BlockIterator neighborIter = GetBlockIteratorForBlockGlobalCoord(neighborCoord);
	if(IsValidIterator(neighborIter))
	{
		return neighborIter;
	}

	return BlockIterator(nullptr, 0);
}


//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Block& BlockIterator::GetBlockRef()
{
	return m_chunk->GetBlock(m_blockIndex);
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
unsigned char BlockIterator::GetBlockType()
{
	return m_chunk->GetBlock(m_blockIndex).m_blockDefinitionIndex;
}
