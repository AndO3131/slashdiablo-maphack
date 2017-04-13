#pragma once
#include "../../D2Structs.h"
#include "../Module.h"
#include "../../Config.h"
#include "../../Common.h"
#include "../../BitReader.h"
#include "../Item/ItemDisplay.h"

#define INVENTORY_WIDTH 10
#define INVENTORY_HEIGHT 4
#define STASH_WIDTH 6
#define LOD_STASH_HEIGHT 8
#define CLASSIC_STASH_HEIGHT 4
#define CUBE_WIDTH 3
#define CUBE_HEIGHT 4

// Pixel positions
#define INVENTORY_LEFT_800 417
#define INVENTORY_LEFT_1300 691
#define INVENTORY_TOP_800 315
#define INVENTORY_TOP_1300 365
#define STASH_LEFT_800 153
#define STASH_LEFT_1300 426
#define LOD_STASH_TOP_800 143
#define LOD_STASH_TOP_1300 192
#define CLASSIC_STASH_TOP_800 334
#define CLASSIC_STASH_TOP_1300 383
#define CUBE_LEFT_800 198
#define CUBE_LEFT_1300 470
#define CUBE_TOP_800 199
#define CUBE_TOP_1300 249
#define CELL_SIZE 29

struct ItemPacketData {
	unsigned int itemId;
	unsigned int x;
	unsigned int y;
	ULONGLONG startTicks;
	unsigned int destination;
};

class ItemMover : public Module {
private:
	bool FirstInit;
	int InventoryItemIds[INVENTORY_WIDTH * INVENTORY_HEIGHT];
	int StashItemIds[STASH_WIDTH * LOD_STASH_HEIGHT];
	int CubeItemIds[CUBE_WIDTH * CUBE_HEIGHT];
	unsigned int HealKey;
	unsigned int ManaKey;
	ItemPacketData ActivePacket;
	CRITICAL_SECTION crit;
public:
	ItemMover() : Module("Item Mover"), ActivePacket(), FirstInit(false) {
		InitializeCriticalSection(&crit);
	};

	~ItemMover() { DeleteCriticalSection(&crit); };

	void Lock() { EnterCriticalSection(&crit); };
	void Unlock() { LeaveCriticalSection(&crit); };

	bool LoadInventory(UnitAny *unit, int xpac, int source, int sourceX, int sourceY, bool shiftState, bool ctrlState, int stashUI, int invUI);
	bool FindDestination(int xpac, int destination, unsigned int itemId, BYTE xSize, BYTE ySize);
	void PickUpItem();
	void PutItemInContainer();
	void PutItemOnGround();

	void OnLoad();
	void OnKey(bool up, BYTE key, LPARAM lParam, bool* block);
	void OnRightClick(bool up, int x, int y, bool* block);
	void OnGamePacketRecv(BYTE* packet, bool *block);
	void OnGameExit();
};


void ParseItem(const unsigned char *data, ItemInfo *ii, bool *success);
bool ProcessStat(unsigned int statId, BitReader &reader, ItemProperty &itemProp);
