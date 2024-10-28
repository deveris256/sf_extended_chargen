#pragma once

class BGSAVMData : public RE::TESForm
{
public:
	RE::BSFixedString editorName;  // 30
	BGSAVMData*       unk38;       // 38 - Points to itself?
	enum Type
	{
		SIMPLE,
		COMPLEX,
		MODULATION
	};
	uint64_t          type;   // 40
	RE::BSFixedString name;   // 48
	RE::BSFixedString name2;  // 50
	struct Entry
	{
		RE::BSFixedString name;
		RE::BSFixedString textureOrAVM;
		struct Color
		{
			uint8_t r, g, b, a;
		};
		Color    color;
		uint32_t unk14;
	};
	Entry*   entryBegin;  // 58
	Entry*   entryEnd;    // 60
	Entry*   unk68;       // 68 - Why are there 2 end pointers?
	uint64_t unk70;       // 70
};
static_assert(sizeof(BGSAVMData) == 0x80);
