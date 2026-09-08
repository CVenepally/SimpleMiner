#include <cstdint>

#pragma pack(push, 1)

struct ChunkSaveHeader
{
	uint8_t m_gchk[8] = {'G', 'C', 'H', 'K', 1, 4, 4, 7};
};

struct ChunkSaveRun
{
	uint8_t m_type;
	uint8_t m_length;
};

#pragma pack(pop)