/*
Helper to convert compiled bytecode in COFF object to use with GTA3Script/CLEO Scripts in GTA San Andreas
Write by Fabio3rs - https://github.com/Fabio3rs

MIT License

Copyright (c) 2020 Fabio3rs

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
// http://brmodstudio.forumeiros.com
// http://brmodstudio.forumeiros.com
// http://brmodstudio.forumeiros.com
/*
    Compile with:
    g++ newCompilerHelper.cpp CText.cpp --std=c++17

    Tested with MinGW objects on Windows
*/
#define _CRT_SECURE_NO_WARNINGS

#include "CText.h"
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <sstream>
#include <cstdint>
#include <cctype>
#include <optional>
#include <cmath>

//// http://www.zedwood.com/article/cpp-str_replace-function
std::string& str_replace(const std::string &search, const std::string &replace, std::string &subject)
{
	std::string buffer;

	const int sealeng = search.length();
	const int strleng = subject.length();

	if (sealeng == 0)
		return subject; //no change

	for (int i = 0, j = 0; i<strleng; j = 0)
	{
		while (i + j<strleng && j<sealeng && subject[i + j] == search[j])
			j++;

		if (j == sealeng) //found 'search'
		{
			buffer.append(replace);
			i += sealeng;
		}
		else
		{
			buffer.append(&subject[i++], 1);
		}
	}

	subject = buffer;
	return subject;
}

static inline void ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch)
    {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch)
    {
		return !std::isspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s)
{
	ltrim(s);
	rtrim(s);
}

std::string compileCMD = "F:/MinGW/bin/g++.exe -c %0 -fno-exceptions -IF:\\MinGW\\include -IF:\\MinGW\\i686-w64-mingw32\\include -IF:\\MinGW\\i686-w64-mingw32\\include\\c++ -IF:\\MinGW\\i686-w64-mingw32\\include\\c++\\i686-w64-mingw32 -IF:\\MinGW\\lib\\gcc\\i686-w64-mingw32\\4.8.3\\include -nostdlib -std=c++11 -shared -masm=intel -o %1";
std::string compileCMD2 = "F:/MinGW/bin/g++.exe -c new.cpp -fno-exceptions -IF:\\MinGW\\include -IF:\\MinGW\\i686-w64-mingw32\\include -IF:\\MinGW\\i686-w64-mingw32\\include\\c++ -IF:\\MinGW\\i686-w64-mingw32\\include\\c++\\i686-w64-mingw32 -IF:\\MinGW\\lib\\gcc\\i686-w64-mingw32\\4.8.3\\include -nostdlib -std=c++11 -shared -masm=intel -o testnew.o";

bool debugOn = false;
bool outputToGTA3SCFormat = false;
bool exportList = true, exportOtherFunctions = false;

int rellocOffset = 0;

std::vector<std::string> exports;

void executeCommand(const std::string &command)
{
	system(command.c_str());
}

#pragma pack(push, 1)
union COFFNameUnion
{
	char	str[8];
	struct
	{
		uint32_t zeroes;
		uint32_t offset;
	};
};

// COFF file structures, thanks to http://wiki.osdev.org/COFF
struct COFFHeader {
	unsigned short			f_magic;	/* Magic number */
	unsigned short		 	f_nscns;	/* Number of Sections */
	int32_t 					f_timdat;	/* Time & date stamp */
	int32_t 					f_symptr;	/* File pointer to Symbol Table */
	int32_t 					f_nsyms;	/* Number of Symbols */
	unsigned short 			f_opthdr;	/* sizeof(Optional Header) */
	unsigned short			f_flags;	/* Flags */
};

struct COFFOptionalHeader {
	unsigned short			magic;          /* Magic Number                    */
	unsigned short			vstamp;         /* Version stamp                   */
	uint32_t			tsize;          /* Text size in bytes              */
	uint32_t			dsize;          /* Initialised data size           */
	uint32_t			bsize;          /* Uninitialised data size         */
	uint32_t			entry;          /* Entry point                     */
	uint32_t 			text_start;     /* Base of Text used for this file */
	uint32_t 			data_start;     /* Base of Data used for this file */
};

enum COFFSectionHeaderType { STYP_TEXT = 0x20, STYP_DATA = 0x40, STYP_BSS = 0x80};
int32_t COFFSectionHeaderTypes = STYP_TEXT | STYP_DATA | STYP_BSS;

struct COFFSectionHeader {
	COFFNameUnion			s_name;	/* Section Name */
	int32_t					s_paddr;	/* Physical Address */
	int32_t					s_vaddr;	/* Virtual Address */
	int32_t					s_size;		/* Section Size in Bytes */
	int32_t					s_scnptr;	/* File offset to the Section data */
	int32_t					s_relptr;	/* File offset to the Relocation table for this Section */
	int32_t					s_lnnoptr;	/* File offset to the Line Number table for this Section */
	unsigned short			s_nreloc;	/* Number of Relocation table entries */
	unsigned short			s_nlnno;	/* Number of Line Number table entries */
	int32_t					s_flags;	/* Flags for this section */
};

struct COFFRealocationsEntries {
	int32_t					r_vaddr;	/* Reference Address */
	int32_t					r_symndx;	/* Symbol index */
	unsigned short			r_type;		/* Type of relocation */
	//unsigned short			offset;		/* offset */
};

struct COFFLineNumberEntries {
	union
	{
		int32_t				l_symndx;	/* Symbol Index */
		int32_t				l_paddr;	/* Physical Address */
	} l_addr;
	unsigned short			l_lnno;		/* Line Number */
};

struct COFFSymbolTable
{
	COFFNameUnion			name;
	int32_t					n_value;	/* Value of Symbol */
	short					n_scnum;	/* Section Number */
	unsigned short			n_type;		/* Symbol Type */
	char					n_sclass;	/* Storage Class */
	char					n_numaux;	/* Auxiliary Count */
};

struct COFFSectionHeaderCPP
{
	COFFSectionHeader									rawSectionHeader;
	std::unique_ptr<COFFRealocationsEntries[]>			rawRealocationsEntries;
	std::unique_ptr<COFFLineNumberEntries[]>			rawLineNumberEntries;
	std::unique_ptr<char[]>								sectionData;
};

struct writtenSymbols
{
	COFFSymbolTable *symbol;
	int				address;
	std::streampos	filePos;
};

#pragma pack(pop)

class COFFFile
{
public:
	COFFHeader											header;
	COFFOptionalHeader									optionalHeader;
	std::unique_ptr<COFFSectionHeaderCPP[]>				sectionsHeader;
	std::unique_ptr<COFFSymbolTable[]>					symbolTable;
	std::map<int32_t, std::string>							stringData;

	std::vector<writtenSymbols>							wSymbols;

	bool												open;

	COFFFile()
	{
		open = false;
	}

	int getNumSections() const
	{
		return header.f_nscns;
	}

	int getNumSymbols() const
	{
		return header.f_nsyms;
	}

	std::string getFullName(const COFFNameUnion &n) const
	{
		std::string result = "Fail to get name";

		if (n.zeroes != 0)
		{
			int num = 8;

			if (n.str[7] == 0)
				num = strlen(n.str);

			result = "";
			result.insert(0, n.str, num);
		}
		else
		{
			auto off = stringData.find(n.offset);

			if (off != stringData.end())
				result = off->second;
		}

		return result;
	}

	std::string getSectionName(int id) const
	{
		if (getNumSections() <= id)
			return "Fail to get name";

		return getFullName(sectionsHeader[id].rawSectionHeader.s_name);
	}

	std::string getSymbolName(int id) const
	{
		if (getNumSymbols() <= id)
			return "Fail to get name";

		return getFullName(symbolTable[id].name);
	}

	COFFSymbolTable &getSymbolByName(const std::string &name)
	{
		for (int i = 0; i < getNumSymbols(); i++)
		{
			if (getFullName(symbolTable[i].name) == name)
			{
				return symbolTable[i];
			}
		}

		throw std::runtime_error(std::string(__FILE__) + " " + std::to_string(__LINE__));
	}
};

COFFFile openCOFFFile(const std::string &file)
{
	COFFFile COFFfile;
	std::fstream test(file, std::ios::in | std::ios::binary);

	if (!test.is_open())
	{
		std::cout << "Fail to open the file" << std::endl;
		return COFFfile;
	}

	COFFfile.open = true;

	test.seekg(0, std::ios::beg);
	test.read(reinterpret_cast<char*>(&COFFfile.header), sizeof(COFFfile.header));

	//std::cout << header.f_opthdr << std::endl;
	//std::cout << header.f_nscns << std::endl;

	if (COFFfile.header.f_opthdr != 0)
	{
		test.read(reinterpret_cast<char*>(&COFFfile.optionalHeader), sizeof(COFFfile.optionalHeader));
	}

	COFFfile.sectionsHeader = std::make_unique<COFFSectionHeaderCPP[]>(COFFfile.header.f_nscns);
	COFFfile.symbolTable = std::make_unique<COFFSymbolTable[]>(COFFfile.header.f_nsyms);

	{
		auto sectionsHeaderPointer = COFFfile.sectionsHeader.get();

		for (int i = 0; i < COFFfile.header.f_nscns; i++)
		{
			auto &sectionHeader = sectionsHeaderPointer[i];
			test.read(reinterpret_cast<char*>(&(sectionHeader.rawSectionHeader)), sizeof(COFFSectionHeader));
		}

		for (int i = 0; i < COFFfile.header.f_nscns; i++)
		{
			auto &sectionHeader = sectionsHeaderPointer[i];

			/*std::cout << "Section name " << sectionHeader.rawSectionHeader.s_name << " Section size " << sectionHeader.rawSectionHeader.s_size
				<< "    lines num " << sectionHeader.rawSectionHeader.s_nlnno
				<< " relocations num " << sectionHeader.rawSectionHeader.s_nreloc
				<< "  type " << (sectionHeader.rawSectionHeader.s_flags & COFFSectionHeaderTypes) << std::endl;*/

			if (sectionHeader.rawSectionHeader.s_scnptr > 0 && sectionHeader.rawSectionHeader.s_size > 0)
			{
				sectionHeader.sectionData = std::make_unique<char[]>(sectionHeader.rawSectionHeader.s_size);

				test.seekg(sectionHeader.rawSectionHeader.s_scnptr, std::ios::beg);
				test.read(sectionHeader.sectionData.get(), sectionHeader.rawSectionHeader.s_size);
			}

			if (sectionHeader.rawSectionHeader.s_nreloc > 0)
			{
				sectionHeader.rawRealocationsEntries = std::make_unique<COFFRealocationsEntries[]>(sectionHeader.rawSectionHeader.s_nreloc);

				test.seekg(sectionHeader.rawSectionHeader.s_relptr, std::ios::beg);
				test.read(reinterpret_cast<char*>(sectionHeader.rawRealocationsEntries.get()), sizeof(COFFRealocationsEntries) * sectionHeader.rawSectionHeader.s_nreloc);
			}

			if (sectionHeader.rawSectionHeader.s_nlnno > 0)
			{
				sectionHeader.rawLineNumberEntries = std::make_unique<COFFLineNumberEntries[]>(sectionHeader.rawSectionHeader.s_nlnno);

				test.seekg(sectionHeader.rawSectionHeader.s_lnnoptr, std::ios::beg);
				test.read(reinterpret_cast<char*>(sectionHeader.rawLineNumberEntries.get()), sizeof(COFFLineNumberEntries) * sectionHeader.rawSectionHeader.s_nlnno);
			}
		}
	}

	// Read symbols table
	test.seekg(COFFfile.header.f_symptr, std::ios::beg);
	test.read(reinterpret_cast<char*>(COFFfile.symbolTable.get()), sizeof(COFFSymbolTable) * COFFfile.header.f_nsyms);

	int32_t mem = 0;

	test.read(reinterpret_cast<char*>(&mem), sizeof(mem));

	std::unique_ptr<char[]> strings = std::make_unique<char[]>(mem);
	const int memTest = (mem - 4);

	{
		memset(strings.get(), 0, 4);
		test.read(strings.get() + 4, mem - 4);

		char *s = strings.get();

		for (int i = 0; i < memTest; i++)
		{
			if (i >= memTest)
			{
				break;
			}

			COFFfile.stringData[i] = &s[i];

			int len = strlen(&s[i]);
			i += len;

			while (i < memTest)
			{
				if (s[i] == 0)
				{
					i++;
				}
				else
				{
					i--;
					break;
				}
			}
		}
	}

	for (int i = 0; i < COFFfile.header.f_nsyms; i++)
	{
		/*COFFSymbolTable &symbtable = COFFfile.symbolTable[i];

		if (symbtable.name.zeroes != 0 && symbtable.name.str[0] > 0 && isprint(symbtable.name.str[0]))
		{
			std::cout << symbtable.name.str << std::endl;
		}
		else
		{
			if (symbtable.name.offset && symbtable.name.offset < memTest)
			{
				std::cout << "zeroes " << symbtable.name.zeroes << std::endl;
				std::cout << "offset " << symbtable.name.offset << std::endl;

				std::cout << "	" << COFFfile.stringData[symbtable.name.offset] << std::endl;
			}
		}*/
	}



	return COFFfile;
}

void makeCall(std::vector<uint8_t> &data, int absoluteTargetAddress)
{
	int32_t calc = data.size();
	calc += 5;
	calc = absoluteTargetAddress - calc;

	data.push_back(0xE8);
	uint8_t *calcPtr = reinterpret_cast<uint8_t*>(&calc);
	data.insert(data.end(), calcPtr, calcPtr + sizeof(calc));
}

void makeExternalCall(std::vector<uint8_t> &data, uint32_t absoluteTargetAddress)
{
	uint8_t asmData[] = {
		0x68, 0x00, 0x00, 0x00, 0x00,
		0xC3
	};

	*reinterpret_cast<uint32_t*>(&asmData[1]) = absoluteTargetAddress;

	data.insert(data.end(), asmData, asmData + (sizeof(asmData) / sizeof(uint8_t)));
}

void makeExternalCallPushAddress(std::vector<uint8_t> &data, uint32_t absoluteTargetAddress)
{
	uint8_t asmData[] = {
		0x00, 0x00, 0x00, 0x00
	};

	*reinterpret_cast<uint32_t*>(&asmData[0]) = absoluteTargetAddress;

	data.insert(data.end(), asmData, asmData + (sizeof(asmData) / sizeof(uint8_t)));
}

writtenSymbols getSymbolForReAlloc(const COFFFile &file, int symbolId)
{
	writtenSymbols result{0, 0, 0};

	if (file.getNumSymbols() > symbolId)
	{
		auto &symbol = file.symbolTable[symbolId];

		for (auto &ws : file.wSymbols)
		{
			if (std::addressof(symbol) == ws.symbol)
				return ws;
		}
	}

	return result;
}

struct scriptSymbols
{
	std::string name;
	int pos;
	int COFFid;

	scriptSymbols()
	{
		pos = 0;
		COFFid = -1;
	}
};

std::vector<scriptSymbols> symbols;

scriptSymbols &getSymbolByName(const std::string &name)
{
	for (int i = 0; i < symbols.size(); i++)
	{
		if (symbols[i].name == name)
		{
			return symbols[i];
		}
	}

	throw std::runtime_error(std::string(__FILE__) + " " + std::to_string(__LINE__));
}

std::optional<scriptSymbols> symbolForIndex(int index)
{
	for (auto &s : symbols)
	{
		if (s.pos == index)
		{
			return s;
		}
	}

	return std::nullopt;
}

std::string outputStartHex()
{
	if (outputToGTA3SCFormat)
	{
		return "DUMP";
	}

	return "HEX";
}

std::string outputEndHex()
{
	if (outputToGTA3SCFormat)
	{
		return "ENDDUMP";
	}

	return "END";
}

std::string labelFormat(const std::string &label)
{
	if (outputToGTA3SCFormat)
	{
		return label + ":";
	}

	return ":" + label;
}

bool mustExportThisSymbol(const std::string &symbolName)
{
	if (symbolName.size() == 0)
		return false;

	if (symbolName == "_initCPPCode")
	{
		return true;
	}

	std::string s = symbolName;

	if (s[0] == '_')
		s[0] = ' ';

	trim(s);

	//std::cout << s << s.size() << std::endl;

	auto it = std::find(exports.begin(), exports.end(), s);

	if (it != exports.end())
	{
		//std::cout << "it true" << std::endl;
		return true;
	}

	return false;
}

void vectorDataToFile(COFFFile &coff, std::fstream &fout, std::vector<uint8_t> &data)
{
	int breakLineCount = 0;

	// TODO: find .drectve section

	bool DUMP = false, firstDUMP = false;

	for (int i = 0; i < data.size(); i++, breakLineCount++)
	{
		auto &ch = data[i];

		auto sn = symbolForIndex(i);
		if (sn)
		{
			auto symbol = sn.value();

			if (mustExportThisSymbol(symbol.name) && (symbol.COFFid == -1 || (symbol.COFFid >= 0 && coff.symbolTable[symbol.COFFid].n_type == 32)))
			{
				if (DUMP)
				{
					fout << std::endl;
					fout << outputEndHex() << "\n";
				}
				fout << std::endl;
				fout << labelFormat(symbol.name) << "\n";
				fout << outputStartHex() << "\n";
				breakLineCount = 0;
				DUMP = true;
				firstDUMP = true;
			}
		}

		if (!firstDUMP)
		{
			fout << outputStartHex() << "\n";
			firstDUMP = true;
			DUMP = true;
		}

		fout << std::hex << (ch < 0x10 ? "0" : "") << (int)(ch) << " ";

		if (breakLineCount >= 60)
		{
			fout << std::endl;
			breakLineCount = 0;
		}
	}
}

void pushSymbolForSection(const COFFFile &file, int section, int value, int bytes)
{
	for (int l = 0; l < file.getNumSymbols(); l++)
	{
		auto &symbol = file.symbolTable[l];

		//std::cout << symbol.n_type << std::endl;
		if ((symbol.n_scnum - 1) == section && /*symbol.n_type > 0 &&*/ symbol.n_value == value)
		{
			scriptSymbols ns;

			ns.name = file.getFullName(symbol.name);
			ns.pos = bytes;
			ns.COFFid = l;

			//std::cout << ns.name << std::endl;
			//std::cout << bytes << std::endl;
			//std::cout << value << std::endl;
			//std::cout << l << std::endl;

			symbols.push_back(std::move(ns));
		}
	}
}

void listSymbols(const COFFFile &file)
{
	for (int l = 0; l < file.getNumSymbols(); l++)
	{
		auto &symbol = file.symbolTable[l];

		{
			std::cout << file.getFullName(symbol.name) << std::endl;
			std::cout << l << std::endl;
		}
	}
}

struct reallocationTable
{
	int32_t offset;
	int32_t symbol;
	int32_t type;

	reallocationTable()
	{
		offset = symbol = type = 0;
	}

	reallocationTable(const COFFRealocationsEntries &entry)
	{
		offset = entry.r_vaddr;
		symbol = entry.r_symndx;
		type = entry.r_type;
	}
};

struct reallocationTableToFile
{
	int32_t offset;
	int32_t symbol;

	reallocationTableToFile()
	{
		offset = symbol = 0;
	}

	reallocationTableToFile(const reallocationTable &entry)
	{
		offset = entry.offset;
		symbol = entry.symbol;
	}
};

std::vector<reallocationTable> rellocations;

enum externalSymbolsType {textFunction, dataFunction};
struct externalObjectList
{
	uint32_t address;
	int type;

	externalObjectList()
	{
		address = 0;
		type = 0;
	}
};

std::map<std::string, externalObjectList> externalObjects;

const std::map<std::string, externalObjectList> &getExternalObjectsList()
{
	return externalObjects;
}

void parseDrective(const char *p, int size)
{
	if (p && size > 0)
	{
		std::string str(p, p + size);

		std::replace(str.begin(), str.end(), '	', '\n');
		std::replace(str.begin(), str.end(), ' ', '\n');
		str_replace("-export:\"", "\n", str);
		std::replace(str.begin(), str.end(), '"', '\n');
		trim(str);

		std::stringstream s(str);

		std::string temp;

		while (std::getline(s, temp))
		{
			trim(temp);

			if (temp.size() == 0)
				continue;

			//std::cout << temp << temp.size() << std::endl;
			exports.push_back(temp);
		}
	}
}

void loadSymbolList()
{
	std::cout << "Tentando carregar SymbolList.txt..." << std::endl;

	std::string test;
	{
		std::fstream symbolList("SymbolList.txt", std::ios::in | std::ios::binary);

		if (symbolList.fail())
		{
			std::cout << "Falha ao abrir o arquivo SymbolList.txt" << std::endl << std::endl;
			return;
		}

		symbolList.seekg(0, std::ios::end);
		size_t size = symbolList.tellg();
		symbolList.seekg(0, std::ios::beg);

		test.insert(0, size, '\n');
		symbolList.read(test.data(), size);
	}

	std::stringstream contents(std::move(test));

	char line[1024];

	while (contents.getline(line, sizeof(line)))
	{
		char funame[128], sect[64];
		uint32_t addr;
		int r = sscanf(line, "%s %s %x", funame, sect, &addr);

		if (r == 3)
		{
			auto &nSymbol = externalObjects[funame];
			nSymbol.address = addr;

			nSymbol.type = externalSymbolsType::textFunction;

			if (std::string(sect).find("data") != std::string::npos)
			{
				nSymbol.type = externalSymbolsType::dataFunction;
			}
		}
	}
}

template<class T>
void printbits(T c)
{
	for (int i = 0; i < (sizeof(T) * 8); i++)
	{
		const uint64_t b = pow(2, i);
		std::cout << ((c & b) != 0);
	}
}

void toCLEOSCM(COFFFile &COFFfile, const std::string &outFinalFile)
{
	rellocations.clear();
	symbols.clear();
	exports.clear();

	if (COFFfile.open)
	{
		std::fstream fout(outFinalFile, std::ios::out | std::ios::trunc);

		if (fout.is_open())
		{
			std::vector<uint8_t> outputData;

			{
				externalObjectList nobj;

				nobj.address = 0x00588BE0;
				nobj.type = 0;

				externalObjects["_showTextBox"] = nobj;
			}

			{
				{
					scriptSymbols ns;
					ns.name = "getCodeTopAddress";
					ns.pos = outputData.size();

					symbols.push_back(ns);
				}

				uint8_t ab[] = { 0xE8u, 0x01u, 0x00u, 0x00u, 0x00u, 0xC3u, 0x8Bu, 0x04u, 0x24u, 0x83u, 0xE8u, 0x05u, 0xC3u };
				outputData.insert(outputData.end(), ab, ab + (sizeof(ab) / sizeof(uint8_t)));
			}

			for (int i = 0; i < COFFfile.getNumSections(); i++)
			{
				auto &section = COFFfile.sectionsHeader[i];

				if (COFFfile.getSectionName(i) == ".drectve")
				{
					//std::cout << (section.rawSectionHeader.s_flags & (STYP_TEXT | STYP_DATA | STYP_BSS)) << std::endl << std::endl;
					parseDrective(section.sectionData.get(), section.rawSectionHeader.s_size);
					continue;
				}

				if (section.rawSectionHeader.s_nreloc > 0)
				{
					auto reloc = section.rawRealocationsEntries.get();

					for (int i = 0; i < section.rawSectionHeader.s_nreloc; i++)
					{
						reallocationTable r = reloc[i];

						r.offset += outputData.size();

						//std::cout << reloc[i].offset << std::endl;

						rellocations.push_back(r);
					}
				}

				if (section.rawSectionHeader.s_flags & STYP_TEXT)
				{
					auto data = section.sectionData.get();

					for (int j = 0; j < section.rawSectionHeader.s_size; j++)
					{
						uint8_t ubyte = static_cast<uint8_t>(data[j]);

						pushSymbolForSection(COFFfile, i, j, outputData.size());

						outputData.push_back(ubyte);
					}
				}

				if (section.rawSectionHeader.s_flags & STYP_DATA)
				{
					auto data = section.sectionData.get();
					for (int j = 0; j < section.rawSectionHeader.s_size; j++)
					{
						uint8_t ubyte = static_cast<uint8_t>(data[j]);

						pushSymbolForSection(COFFfile, i, j, outputData.size());

						outputData.push_back(ubyte);
					}
				}

				if (section.rawSectionHeader.s_flags & STYP_BSS)
				{
					for (int j = 0; j < section.rawSectionHeader.s_size; j++)
					{
						pushSymbolForSection(COFFfile, i, j, outputData.size());
						outputData.push_back(0x00);
					}
				}
			}

			for (int i = 0; i < COFFfile.getNumSymbols(); i++)
			{
				auto &symbol = COFFfile.symbolTable[i];
				if (symbol.n_scnum == 0) {
					auto objFullName = COFFfile.getFullName(symbol.name);
					/*std::cout << objFullName << std::endl;
					std::cout << "n_type " << symbol.n_type << std::endl;
					std::cout << "n_value " << symbol.n_value << std::endl;
					std::cout << "n_scnum " << symbol.n_scnum << std::endl;*/

					auto &eobj = getExternalObjectsList();

					auto it = eobj.find(objFullName);

					if (it != eobj.end())
					{
						{
							scriptSymbols ns;
							ns.name = objFullName;
							ns.pos = outputData.size();
							ns.COFFid = i;

							symbols.push_back(std::move(ns));
						}

						switch (it->second.type)
						{
						case 0:
							makeExternalCall(outputData, it->second.address);
							break;

						case 1:
							makeExternalCallPushAddress(outputData, it->second.address);
							break;

						case 2:
							break;

						default:
							break;
						}
					}
					else
					{
						std::cout << "Undefined external: " << COFFfile.getSymbolName(i) << std::endl;
					}
				}
			}

			int reallocationFunctionPos = 0;
			{
				scriptSymbols ns;
				ns.name = "reallocationFunction";
				ns.pos = outputData.size();

				reallocationFunctionPos = ns.pos;

				symbols.push_back(std::move(ns));

				/*
				#include <cstdint>

				struct reallocationTable
				{
				int32_t offset;
				int32_t symbol;
				int32_t type;
				};

				extern "C" void volatile reallocationFunction(char *codeTop)
				{
				char *tablePos = codeTop;
				tablePos += 0x10000;

				reallocationTable *table = reinterpret_cast<reallocationTable*>(tablePos);
				int tableSize = 1000;

				for (int i = 0; i < tableSize; i++)
				{
				auto &tb = table[i];
				int32_t **value = reinterpret_cast<int32_t**>(&codeTop[tb.offset]);
				*value = reinterpret_cast<int32_t*>(&codeTop[tb.symbol]);
				}
				}

				*/

				int startPos = outputData.size();

				uint8_t reallocationFunCode[] = {
					/*	    11 00000000*/ 0x55,		                         	//push   ebp 
					/*	    14 00000001*/ 0x89, 0xE5,		                       	//mov   ebp, esp 
					/*	    16 00000003*/ 0x83, 0xEC, 0x20,		                     	//sub   esp, 32 
					/*	    17 00000006*/ 0x8B, 0x45, 0x08,		                     	//mov   eax, DWORD [ebp+8] 
					/*	    18 00000009*/ 0x89, 0x45, 0xF8,		                     	//mov   DWORD [ebp-8], eax 
					/*	    19 0000000C*/ 0x81, 0x45, 0xF8, 0x00, 0x00, 0x01, 0x00,		             	//add   DWORD [ebp-8], 65536 
					/*	    20 00000013*/ 0x8B, 0x45, 0xF8,		                     	//mov   eax, DWORD [ebp-8] 
					/*	    21 00000016*/ 0x89, 0x45, 0xF4,		                     	//mov   DWORD [ebp-12], eax 
					/*	    22 00000019*/ 0xC7, 0x45, 0xF0, 0xE8, 0x03, 0x00, 0x00,		             	//mov   DWORD [ebp-16], 1000 
					/*	    23 00000020*/ 0xC7, 0x45, 0xFC, 0x00, 0x00, 0x00, 0x00,		             	//mov   DWORD [ebp-4], 0 
					/*	    24 00000027*/ 0xEB, 0x37, 		                       	//jmp   L2 
					/*	    26 00000029*/ 0x8B, 0x45, 0xFC,		                     	//mov   eax, DWORD [ebp-4] 
					/*	    27 0000002C*/ 0x8D, 0x14, 0xC5, 0x00, 0x00, 0x00, 0x00,		             	//lea   edx, [0+eax*8] 
					/*	    28 00000033*/ 0x8B, 0x45, 0xF4,		                     	//mov   eax, DWORD [ebp-12] 
					/*	    29 00000036*/ 0x01, 0xD0,		                       	//add   eax, edx 
					/*	    30 00000038*/ 0x89, 0x45, 0xEC,		                     	//mov   DWORD [ebp-20], eax 
					/*	    31 0000003B*/ 0x8B, 0x45, 0xEC,		                     	//mov   eax, DWORD [ebp-20] 
					/*	    32 0000003E*/ 0x8B, 0x00,		                       	//mov   eax, DWORD [eax] 
					/*	    33 00000040*/ 0x89, 0xC2,		                       	//mov   edx, eax 
					/*	    34 00000042*/ 0x8B, 0x45, 0x08,		                     	//mov   eax, DWORD [ebp+8] 
					/*	    35 00000045*/ 0x01, 0xD0,		                       	//add   eax, edx 
					/*	    36 00000047*/ 0x89, 0x45, 0xE8,		                     	//mov   DWORD [ebp-24], eax 
					/*	    37 0000004A*/ 0x8B, 0x45, 0xEC,		                     	//mov   eax, DWORD [ebp-20] 
					/*	    38 0000004D*/ 0x8B, 0x40, 0x04,		                     	//mov   eax, DWORD [eax+4] 
					/*	    39 00000050*/ 0x89, 0xC2,		                       	//mov   edx, eax 
					/*	    40 00000052*/ 0x8B, 0x45, 0x08,		                     	//mov   eax, DWORD [ebp+8] 
					/*	    41 00000055*/ 0x01, 0xC2,		                       	//add   edx, eax 
					/*	    42 00000057*/ 0x8B, 0x45, 0xE8,		                     	//mov   eax, DWORD [ebp-24] 
					/*	    43 0000005A*/ 0x89, 0x10,		                       	//mov   DWORD [eax], edx 
					/*	    44 0000005C*/ 0x83, 0x45, 0xFC, 01,		                   	//add   DWORD [ebp-4], 1 
					/*	    46 00000060*/ 0x8B, 0x45, 0xFC,		                     	//mov   eax, DWORD [ebp-4] 
					/*	    47 00000063*/ 0x3B, 0x45, 0xF0,		                     	//cmp   eax, DWORD [ebp-16] 
					/*	    48 00000066*/ 0x7C, 0xC1,		                       	//jl   L3 
					/*	    49 00000068*/ 0xC9,		                         	//leave
					/*	    52 00000069*/ 0xC3		                         	//ret
				};

				outputData.insert(outputData.end(), reallocationFunCode, reallocationFunCode + (sizeof(reallocationFunCode) / sizeof(uint8_t)));

				int offsetPosInOutputData = startPos + 0xC + 3;
				int numPosInOutputData = startPos + 0x19 + 3;



				for (auto it = rellocations.begin(); it != rellocations.end(); )
				{
					if (it->type == 20)
					{
						int32_t off = it->offset;
						int32_t symbol = it->symbol;

						for (int i = 0; i < symbols.size(); i++)
						{
							auto &s = symbols[i];

							if (s.COFFid >= 0 && symbol == s.COFFid)
							{
								int32_t target = s.pos;

								//std::cout << COFFfile.getSymbolName(s.COFFid) << std::endl;
								//std::cout << outputData[target] << std::endl;
								//std::cout << off << std::endl;

								target = target - (off + 4);
								//std::cout << target << std::endl;

								*reinterpret_cast<int32_t*>(std::addressof(outputData[it->offset])) = target;

								break;
							}
						}

						it = rellocations.erase(it);
					}
					else
					{
						auto &value = *reinterpret_cast<int32_t*>(std::addressof(outputData[it->offset]));

						if (value > 0 && it->type == 6)
						{
							bool found = false;

							for (int i = 0; i < COFFfile.getNumSymbols(); i++)
							{
								if (found)
									break;

								auto &symbol = COFFfile.symbolTable[i];

								if (symbol.n_value == value)
								{
									for (int k = 0; k < symbols.size(); k++)
									{
										auto &s = symbols[k];

										if (s.COFFid >= 0 && i == s.COFFid)
										{
											std::cout << "it symbol " << it->symbol << std::endl;
											std::cout << "s.pos " << s.pos << std::endl;

											/*std::cout << "COFFid " << i << std::endl;
											
											std::cout << "it offset " << it->offset << std::endl;
											std::cout << "name " << COFFfile.getSymbolName(i) << std::endl;
											std::cout << "value " << value << std::endl;
											std::cout << "symbol.n_numaux " << (int)symbol.n_numaux << std::endl;
											std::cout << "symbol.n_sclass " << (int)symbol.n_sclass << std::endl;
											std::cout << "symbol.n_scnum " << (int)symbol.n_scnum << std::endl;
											std::cout << "symbol.n_type " << (int)symbol.n_type << std::endl;
											std::cout << "symbol.n_value " << symbol.n_value << std::endl;
											std::cout << "it section " << it->section << std::endl;*/
											it->symbol = i;
											it->type = 777;
											value = s.pos;
											//value = 0;

											found = true;
											break;
										}
									}
								}
							}

							if (!found)
							{
								std::cout << "Symbol detect error " << it->offset << std::endl;
							}

							//value += 0x00400000;
						}
						else
						{
							//std::cout << "Other type? " << it->offset << "   " << it->symbol << "  " << it->type << "        " << *reinterpret_cast<int32_t*>(std::addressof(outputData[it->offset])) << std::endl;
						}

						++it;
					}
				}



				const int reallocationNum = rellocations.size();
				const int reallocationOffset = outputData.size();

				*reinterpret_cast<int32_t*>(&outputData[offsetPosInOutputData]) = reallocationOffset;
				*reinterpret_cast<int32_t*>(&outputData[numPosInOutputData]) = reallocationNum;

				{
					for (auto &r : rellocations)
					{
						for (int i = 0; i < symbols.size(); i++)
						{
							auto &s = symbols[i];

							if (s.COFFid >= 0 && s.COFFid == (r.symbol))
							{
								auto &symbol = COFFfile.symbolTable[s.COFFid];

								if (r.type == 6)
								{
									//std::cout << COFFfile.getSymbolName(s.COFFid) << std::endl;
									r.symbol = s.pos + *reinterpret_cast<int32_t*>(&outputData[r.offset]);

									*reinterpret_cast<int32_t*>(&outputData[r.offset]) = r.symbol;
									/*std::cout << "r.offset " << r.offset << std::endl;
									std::cout << "r.symbol " << r.symbol << std::endl;
									std::cout << "r.type " << r.type << std::endl;
									std::cout << "*(int*)&outputData[r.offset] " << *(int*)&outputData[r.offset] << std::endl;*/
								}
								else
								{
									r.symbol = *reinterpret_cast<int32_t*>(&outputData[r.offset]);
								}
							}
						}
					}

					uint8_t relocationTop[] = { 0x55, 0x89, 0xE5 };
					uint8_t relocationMiddle[] = {
						/*	    16 00000003*/ 0x83, 0xEC, 0x10,		                     	//sub   esp, 16 
						/*	    17 00000006*/ 0xC7, 0x45, 0xFC, 0x30, 0x18, 0x20, 0x41,		//mov   DWORD [ebp-4], 1092622384 
						/*	    18 0000000D*/ 0xC7, 0x45, 0xF8, 0x50, 0x31, 0x20, 0x51,		//mov   DWORD [ebp-8], 1361064272 
						/*	    19 00000014*/ 0x8B, 0x55, 0xFC,		                     	//mov   edx, DWORD [ebp-4] 
						/*	    20 00000017*/ 0x8B, 0x45, 0x08,		                     	//mov   eax, DWORD [ebp+8] 
						/*	    21 0000001A*/ 0x01, 0xD0,								  	//add   eax, edx 
						/*	    22 0000001C*/ 0x89, 0x45, 0xF4,		                     	//mov   DWORD [ebp-12], eax 
						/*	    23 0000001F*/ 0x8B, 0x55, 0xF8,		                     	//mov   edx, DWORD [ebp-8] 
						/*	    24 00000022*/ 0x8B, 0x45, 0x08,		                     	//mov   eax, DWORD [ebp+8] 
						/*	    25 00000025*/ 0x01, 0xC2,								  	//add   edx, eax 
						/*	    26 00000027*/ 0x8B, 0x45, 0xF4,		                     	//mov   eax, DWORD [ebp-12] 
						/*	    27 0000002A*/ 0x89, 0x10								  	//mov   DWORD [eax], edx  
					};
					uint8_t relocationBtn[] = { 0xC9, 0xC3 };


					/*outputData.insert(outputData.end(), relocationTop, relocationTop + (sizeof(relocationTop) / sizeof(uint8_t)));

					for (auto &r : rellocations)
					{
						*reinterpret_cast<int32_t*>(std::addressof(relocationMiddle[3 + 3])) = r.offset;
						*reinterpret_cast<int32_t*>(std::addressof(relocationMiddle[0xA + 3])) = r.symbol;

						outputData.insert(outputData.end(), relocationMiddle, relocationMiddle + (sizeof(relocationMiddle) / sizeof(uint8_t)));
					}

					outputData.insert(outputData.end(), relocationBtn, relocationBtn + (sizeof(relocationBtn) / sizeof(uint8_t)));*/
				}

				if (reallocationNum > 0)
				{
					std::vector<reallocationTableToFile> rl;
					rl.reserve(rellocations.size());

					for (auto &r : rellocations)
					{
						rl.push_back(r);
					}

					uint8_t *r = reinterpret_cast<uint8_t*>(&rl[0]);
					
					scriptSymbols ns;
					ns.name = "relocationTable";
					ns.pos = outputData.size();
					symbols.push_back(std::move(ns));

					outputData.insert(outputData.end(), r, r + (reallocationNum * sizeof(reallocationTableToFile)));
				}
				else
				{
					outputData.insert(outputData.end(), 12, 0u);
				}
			}

			{
				outputData.insert(outputData.end(), 8, 0x90);
				scriptSymbols ns;
				ns.name = "_initCPPCode";
				ns.pos = outputData.size();

				symbols.push_back(std::move(ns));
				outputData.push_back(0x60); // pushad
				outputData.push_back(0x9C); // pushfd
											//outputData.push_back(0xCC);
				makeCall(outputData, 0);

				//outputData.push_back(0xCC);
				outputData.push_back(0x50);
				makeCall(outputData, reallocationFunctionPos);

				uint8_t addesp4[] = { 0x83, 0xC4, 0x04 };
				//outputData.push_back(0xCC);
				outputData.insert(outputData.end(), addesp4, addesp4 + (sizeof(addesp4) / sizeof(uint8_t)));

				try
				{
					auto &symbol = getSymbolByName(".ctors");

					int num = *reinterpret_cast<int32_t*>(&outputData[symbol.pos]);

					//std::cout << ".ctors: " << symbol.name << " " << symbol.COFFid << " " << symbol.pos << " " << num << std::endl;

					bool callCreated = false;

					for (int i = 0; i < symbols.size(); i++)
					{
						auto &s = symbols[i];
						if (s.COFFid >= 0)
						{
							auto &symbol = COFFfile.symbolTable[s.COFFid];

							if (symbol.n_value == num)
							{
								makeCall(outputData, s.pos);
								callCreated = true;
								break;
							}
						}
					}

					if (!callCreated)
					{
						std::cout << "Ctor found but not created the call, trying second method...\n";

						for (int i = 0; i < symbols.size(); i++)
						{
							auto &s = symbols[i];
							if (s.COFFid >= 0)
							{
								if (s.pos == num)
								{
									makeCall(outputData, s.pos);
									callCreated = true;
									break;
								}
							}
						}

						if (callCreated)
						{
							std::cout << "Call to ctor created\n";
						}
						else
						{
							std::cout << "Fail to search ctor function symbol\n";
						}
					}
				}
				catch (const std::exception &e)
				{
					std::cout << "Erro ao detectar o construtor\n" << e.what() << std::endl;

					for (int i = 0; i < COFFfile.getNumSymbols(); i++)
					{
						std::cout << COFFfile.getSymbolName(i) << std::endl;
					}

					std::cout << "Talvez algo tenha dado errado, por favor avise no tópico: http://brmodstudio.forumeiros.com/t6959-" << std::endl;
					std::cout << "Talvez algo tenha dado errado, por favor avise no tópico: http://brmodstudio.forumeiros.com/t6959-" << std::endl;
					std::cout << "Talvez algo tenha dado errado, por favor avise no tópico: http://brmodstudio.forumeiros.com/t6959-" << std::endl;
				}

				outputData.push_back(0x9D); // popfd
				outputData.push_back(0x61); // popad
				outputData.push_back(0xC3); // InitCPPCode ret
			}

			if (rellocOffset > 0)
			{
				for (auto &r : rellocations)
				{
					*reinterpret_cast<int32_t*>(&outputData[r.offset]) = r.symbol + rellocOffset;
				}
			}

			{
				std::fstream rawFile(outFinalFile + ".raw", std::ios::out | std::ios::trunc | std::ios::binary);

				rawFile.write(reinterpret_cast<const char*>(outputData.data()), outputData.size());
			}

			std::cout << "Bytes de código " << outputData.size() << std::endl;
			vectorDataToFile(COFFfile, fout, outputData);
		}
	}
}

int main(int argc, char *argv[])
{
    /*
    TODO: read command line in argc/argv.
    List/command to export some simbols to output file
    Print usage
    */

	loadSymbolList();

	try {
		CText Config("Config.txt");

		compileCMD = Config[""]["compileCMD"];
		compileCMD2 = Config[""]["compileCMD2"];
	}
	catch (const std::exception &e)
    {
        std::cout << e.what() << std::endl;
		return 0;
	}

	bool what = true, doExit = false;
	int wopt = 0;
	int estagMenu = 0;

	std::cout << "**** ESTE PROGRAMA ESTÁ EM FASE DE CRIAÇÃO E TESTES - USE POR SUA CONTA E RISCO ****" << std::endl;
	std::cout << "**** ESTE PROGRAMA ESTÁ EM FASE DE CRIAÇÃO E TESTES - USE POR SUA CONTA E RISCO ****" << std::endl;
	std::cout << "**** ESTE PROGRAMA ESTÁ EM FASE DE CRIAÇÃO E TESTES - USE POR SUA CONTA E RISCO ****" << std::endl;
	std::cout << "*******************************************************************************************************************" << std::endl;
	std::cout << "Em caso de duvida entre em contato pelo seguinte topico:" << std::endl;
	std::cout << "http://brmodstudio.forumeiros.com/t4721-avancado-facilitador-de-conversao-de-c-para-assembly-para-usar-em-scripts-cleo-scm" << std::endl << std::endl;
	std::cout << "http://brmodstudio.forumeiros.com/t6996-" << std::endl << std::endl;
	std::cout << "Programa por Fabio Rossini Sluzala" << std::endl;
	std::cout << "Obrigado a todas as pessoas que contribuiram para que essa ideia se tornasse possivel" << std::endl;
	std::cout << "Brazilian Modding Studio" << std::endl;
	std::cout << "*******************************************************************************************************************" << std::endl;
	std::cout << std::endl;

	std::string infile = "new.cpp", outCompilefile = "new.o", outFinalFile = "out.txt";

	std::string optTemp;

	do
	{
		switch (estagMenu)
		{
		case 0:
			what = true;
			do
			{
				std::cout << "Usando modo " << (outputToGTA3SCFormat? "GTA3Script" : "Sanny Builder") << "\n";

				std::cout << "Opções:\n";
				std::cout << "	1. Compilar source C/C++ e gerar código CLEO/SCM\n";
				std::cout << "	2. Gerar código para CLEO/SCM de um objeto pronto\n";
				std::cout << "	3. Usar sintaxe de Sanny Builder\n";
				std::cout << "	4. Usar sintaxe de GTA3Script\n";
				std::cout << "	5. Sair\n";

				optTemp = "";
				std::cin >> optTemp;

				try
				{
					wopt = std::stoi(optTemp);

					switch (wopt)
					{
					case 1:
						++estagMenu;
						what = false;
						break;

					case 2:
						++estagMenu;
						what = false;
						break;

					case 3:
						outputToGTA3SCFormat = false;
						break;

					case 4:
						outputToGTA3SCFormat = true;
						break;

					case 5:
						what = false;
						return 0;
						break;

					default:
						std::cout << "Opção inválida, selecione uma opção de 1 a 3\n\n";
						break;
					}
				}
				catch (const std::exception &e)
				{
					std::cout << e.what() << std::endl;
				}
			} while (what);
			break;

		case 1:
		{
			what = true;
			do
			{
				switch (wopt)
				{
				case 1:
				{
					std::cout << "Digite o nome do arquivo a ser compilado:\n";

					optTemp = "";
					std::cin >> optTemp;

					if (optTemp.size() > 0)
					{
						infile = optTemp;
						++estagMenu;
						what = false;
					}
				}
					break;

				case 2:
				{
					std::cout << "Digite o nome do objeto (arquivo do tipo COFF) a ser convertido para uso em Scripts CLEO/SCM:\n";

					optTemp = "";
					std::cin >> optTemp;

					if (optTemp.size() > 0)
					{
						outCompilefile = optTemp;
						estagMenu += 2;
						what = false;
					}
				}
					break;

				default:
					std::cout << "Erro, opcao invalida\n";
					estagMenu = 0;
					what = false;
					break;
				}
			} while (what);
			break;
		}

		case 2:
		{
			what = true;
			do
			{
				std::cout << "Digite o nome do arquivo intermediario (COFF object file):\n";

				optTemp = "";
				std::cin >> optTemp;

				if (optTemp.size() > 0)
				{
					outCompilefile = optTemp;
					++estagMenu;
					what = false;
				}
			} while (what);
		}

		case 3:
		{
			what = true;
			do
			{
				std::cout << "Digite o nome do arquivo de saída final:\n";

				optTemp = "";
				std::cin >> optTemp;

				if (optTemp.size() > 0)
				{
					outFinalFile = optTemp;
					++estagMenu;
					what = false;
				}
			} while (what);

			if (wopt == 1)
			{
				std::string commandToRun = multiRegister(compileCMD, infile, outCompilefile);

				std::cout << "Rodando o seguinte comando...\n";
				std::cout << commandToRun << std::endl;

				executeCommand(commandToRun);
			}

			COFFFile COFFfile = openCOFFFile(outCompilefile);
			toCLEOSCM(COFFfile, outFinalFile);

			std::cout << "Comandos terminados, voltando ao inicio do menu...\n\n\n";

			estagMenu = 0;
		}
			break;
		}
	} while (!doExit);
    
    return 0;
}
