#pragma once
#include "Game/GameCommon.hpp"
#include <cstdint>
#include <vector>

//------------------------------------------------------------------------------------------------------------------
class Chunk;
struct IntVec3;
class Block;

//------------------------------------------------------------------------------------------------------------------
class BlockIterator
{
public:
					BlockIterator() = default;
    explicit		BlockIterator(Chunk* chunk, uint32_t blockIndex = 0);
    virtual			~BlockIterator() = default;

    Directions		GetBlockingDirections(uint32_t blockIndex) const;
    bool			IsBlockedByNeighborChunkBlock(IntVec3 const& localCoord, Directions direction) const;

    void			GetNeighboringBlockIterators(std::vector<BlockIterator>& neighbors);

	BlockIterator	GetBlockIteratorForBlockGlobalCoord(IntVec3 const& blockCoord);

	Block* GetPosXBlock();
	Block* GetNegXBlock();
	Block* GetPosYBlock();
	Block* GetNegYBlock();
	Block* GetPosZBlock();
	Block* GetNegZBlock();

	Block* GetPosXPosYBlock();
	Block* GetPosXNegYBlock();
	Block* GetNegXPosYBlock();
	Block* GetNegXNegYBlock();
	
	// YZ diagonal neighbors
	Block* GetPosYPosZBlock();
	Block* GetPosYNegZBlock();
	Block* GetNegYPosZBlock();
	Block* GetNegYNegZBlock();

	Block* GetPosZPosXBlock();
	Block* GetNegZPosXBlock();
	Block* GetNegZNegXBlock();
	Block* GetPosZNegXBlock();

	Chunk* GetPosXChunk();
	Chunk* GetNegXChunk();
	Chunk* GetPosYChunk();
	Chunk* GetNegYChunk();

	BlockIterator GetPosXBlockIterator();
	BlockIterator GetNegXBlockIterator();
	BlockIterator GetPosYBlockIterator();
	BlockIterator GetNegYBlockIterator();
	BlockIterator GetPosZBlockIterator();
	BlockIterator GetNegZBlockIterator();

	BlockIterator GetPosXPosYBlockIterator();
	BlockIterator GetPosXNegYBlockIterator();
	BlockIterator GetNegXPosYBlockIterator();
	BlockIterator GetNegXNegYBlockIterator();

	Block& GetBlockRef();
	unsigned char GetBlockType();

	bool operator==(const BlockIterator& other) const
	{
		return m_chunk == other.m_chunk && m_blockIndex == other.m_blockIndex;
	}

public:
    Chunk*      m_chunk         = nullptr;
    uint32_t    m_blockIndex    = 0;
};

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
struct BlockIteratorHash
{
	size_t operator()(const BlockIterator& b) const
	{
		return std::hash<Chunk*>()(b.m_chunk) ^ (std::hash<uint32_t>()(b.m_blockIndex) << 1);
	}
};