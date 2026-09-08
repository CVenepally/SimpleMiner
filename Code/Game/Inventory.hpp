#pragma once
#include <cstdint>
#include "Game/GameCommon.hpp"

struct Slot
{
	unsigned char	m_itemType = 0;
	uint8_t			m_itemCount = 0;

	bool IsEmpty() const { return m_itemCount == 0 && m_itemType == 0; }

};


class Inventory
{
	friend class Player;
	friend class Game;

private:	
	Inventory();
	~Inventory();

	void AddItemToInventory(unsigned char itemType, int itemCount);
	void RemoveItemFromActiveSlot();
	uint8_t GetItemCountInActiveSlot() const;
	void PrintToConsole();

private:

	Slot m_slots[TOTAL_SLOTS_INVENTORY] = {};

	int m_activeSlotIndex = 0;
	
	Slot m_copiedSlot = {};
};