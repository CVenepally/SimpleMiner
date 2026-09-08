#include "Game/Inventory.hpp"
#include "Game/BlockDefinition.hpp"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Inventory::Inventory()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
Inventory::~Inventory()
{}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Inventory::AddItemToInventory(unsigned char itemType, int itemCount)
{
	for(Slot& slot : m_slots)
	{
		if(slot.IsEmpty())
		{
			slot.m_itemType = itemType;
			slot.m_itemCount = static_cast<uint8_t>(itemCount);
			break;
		}

		if(slot.m_itemType == itemType)
		{
			int availableSpace = SLOTS_MAX_STACK_SIZE - slot.m_itemCount;

			if(availableSpace == 0)
			{
				continue;
			}

			if(availableSpace >= static_cast<uint8_t>(itemCount))
			{
				slot.m_itemCount += static_cast<uint8_t>(itemCount);
				break;
			}
			slot.m_itemCount = SLOTS_MAX_STACK_SIZE;
			itemCount = itemCount - availableSpace;
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Inventory::RemoveItemFromActiveSlot()
{
	Slot& activeSlot = m_slots[m_activeSlotIndex];
	
	if(activeSlot.m_itemCount > 0)
	{
		activeSlot.m_itemCount -= 1;
		if(activeSlot.m_itemCount == 0)
		{
			activeSlot.m_itemType = 0;
		}
	}
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
uint8_t Inventory::GetItemCountInActiveSlot() const
{
	return m_slots[m_activeSlotIndex].m_itemCount;
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------
void Inventory::PrintToConsole()
{
	bool didDisplayAnything = false;

	int slotCount = 0;

	for(Slot& slot : m_slots)
	{
		slotCount += 1;

		if(slot.m_itemCount > 0)
		{
			didDisplayAnything = true;
			std::string itemString = Stringf("Slot: %d %s: Count: %d", slotCount, BlockDefinition::s_blockDefinitions[slot.m_itemType]->GetBlockName().c_str(), slot.m_itemCount);
			g_devConsole->AddLine(DevConsole::INFO_MAJOR, itemString);
		}
	}

	if(!didDisplayAnything)
	{
		g_devConsole->AddLine(DevConsole::INFO_MAJOR, "No Items");
	}
}
