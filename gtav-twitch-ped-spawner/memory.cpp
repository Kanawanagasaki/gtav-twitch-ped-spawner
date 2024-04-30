#include "memory.h"

#include <windows.h>

namespace Memory
{
	static uintptr_t baseAddr;
	static uintptr_t endAddr;

	// https://github.com/ThirteenAG/Hooking.Patterns/blob/master/Hooking.Patterns.cpp#L113-L117
	template<typename TReturn, typename TOffset>
	TReturn* getRVA(uintptr_t baseAddr, TOffset rva)
	{
		return (TReturn*)(baseAddr + rva);
	}

	static void TransformPattern(std::string_view pattern, std::basic_string<uint8_t>& data, std::basic_string<uint8_t>& mask)
	{
		uint8_t tempDigit = 0;
		bool tempFlag = false;

		auto tol = [](char ch) -> uint8_t
			{
				if (ch >= 'A' && ch <= 'F') return uint8_t(ch - 'A' + 10);
				if (ch >= 'a' && ch <= 'f') return uint8_t(ch - 'a' + 10);
				return uint8_t(ch - '0');
			};

		for (auto ch : pattern)
		{
			if (ch == ' ')
			{
				continue;
			}
			else if (ch == '?')
			{
				data.push_back(0);
				mask.push_back(0);
			}
			else if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
			{
				uint8_t thisDigit = tol(ch);

				if (!tempFlag)
				{
					tempDigit = thisDigit << 4;
					tempFlag = true;
				}
				else
				{
					tempDigit |= thisDigit;
					tempFlag = false;

					data.push_back(tempDigit);
					mask.push_back(0xFF);
				}
			}
		}
	}
}

void Memory::Init()
{
	baseAddr = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));

	// https://github.com/ThirteenAG/Hooking.Patterns/blob/master/Hooking.Patterns.cpp#L123-L142
	static auto getSection = [](const PIMAGE_NT_HEADERS nt_headers, unsigned section) -> PIMAGE_SECTION_HEADER
		{
			return reinterpret_cast<PIMAGE_SECTION_HEADER>(
				(UCHAR*)nt_headers->OptionalHeader.DataDirectory +
				nt_headers->OptionalHeader.NumberOfRvaAndSizes * sizeof(IMAGE_DATA_DIRECTORY) +
				section * sizeof(IMAGE_SECTION_HEADER));
		};

	PIMAGE_DOS_HEADER dosHeader = getRVA<IMAGE_DOS_HEADER>(baseAddr, 0);
	PIMAGE_NT_HEADERS ntHeader = getRVA<IMAGE_NT_HEADERS>(baseAddr, dosHeader->e_lfanew);
	for (int i = 0; i < ntHeader->FileHeader.NumberOfSections; i++)
	{
		auto sec = getSection(ntHeader, i);
		auto secSize = sec->SizeOfRawData != 0 ? sec->SizeOfRawData : sec->Misc.VirtualSize;
		if (sec->Characteristics & IMAGE_SCN_MEM_EXECUTE)
			endAddr = baseAddr + sec->VirtualAddress + secSize;
		if ((i == ntHeader->FileHeader.NumberOfSections - 1) && endAddr == 0)
			endAddr = baseAddr + sec->PointerToRawData + secSize;
	}
}

Memory::Handle Memory::FindPattern(const std::string& szPattern)
{
	std::string szCopy = szPattern;
	for (size_t pos = szCopy.find("??"); pos != std::string::npos; pos = szCopy.find("??", pos + 1))
		szCopy.replace(pos, 2, "?");

	std::basic_string<uint8_t> bytesVec;
	std::basic_string<uint8_t> maskVec;
	TransformPattern(szCopy, bytesVec, maskVec);

	const uint8_t* pattern = bytesVec.data();
	const uint8_t* mask = maskVec.data();
	const size_t maskSize = maskVec.size();
	const size_t lastWild = maskVec.find_last_not_of(uint8_t(0xFF));

	ptrdiff_t Last[256];

	std::fill(std::begin(Last), std::end(Last), lastWild == std::string::npos ? -1 : static_cast<ptrdiff_t>(lastWild));

	for (ptrdiff_t i = 0; i < static_cast<ptrdiff_t>(maskSize); ++i)
	{
		if (Last[pattern[i]] < i)
		{
			Last[pattern[i]] = i;
		}
	}

	for (uintptr_t i = baseAddr, end = endAddr - maskSize; i <= end;)
	{
		uint8_t* ptr = reinterpret_cast<uint8_t*>(i);
		ptrdiff_t j = maskSize - 1;

		while ((j >= 0) && pattern[j] == (ptr[j] & mask[j])) j--;

		if (j < 0)
			return Handle(uintptr_t(ptr));
		else i += max(ptrdiff_t(1), j - Last[ptr[j]]);
	}

	return Handle();
}
