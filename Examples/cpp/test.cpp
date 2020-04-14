/****************************************************************************
* http://bms.mixmods.com.br/t4721-
*
*
*/
/*
 *  ASI project to generate assembly callbacks to use with CLEO scripts
 *  
 *  Modified version to use inside a CLEO script with this tool: https://github.com/Fabio3rs/COFF-to-GTAScript-Helper
 *
 *  Copyright (C) 2018 Fabio3rs <>
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 * 
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 *
 */
#include <cstring> // Protótipos das funções de string de C, como por exemplo strcmp, entre outras.
#include <cstdint>
#include "dynamic_hooker.hpp"
#include "script.h"

extern "C" void *memcpy(void *destination, const void *source, size_t num)
{
	const uint8_t *src = (const uint8_t*)source;
	uint8_t *dest = (uint8_t*)destination;
	
	for (size_t i = 0; i < num; i++)
	{
		dest[i] = src[i];
	}
}

auto CRunningScript__Process = injector::thiscall<char(CRunningScript *)>::call<0x00469F00>;

struct cppToCleoCallback
{
	uintptr_t							address;
	CRunningScript						fakeScript;
	CRunningScript						*owner;
	uint8_t								*startIp;
	int									nparams;


	injectcode::dynamic_hooker::newCode	myCode;

	void run(injector::reg_pack &c)
	{
		if (owner->baseIP == nullptr)
			return;

		fakeScript = CRunningScript();

		fakeScript.baseIP = owner->baseIP;
		fakeScript.curIP = startIp;

		fakeScript.IsExternalThread = true;
		fakeScript.isActive = true;
		fakeScript.tls[0].pParam = std::addressof(c);

		memcpy(&(fakeScript.tls[1]), (void*)(c.esp), nparams * 4);

		CRunningScript__Process(&fakeScript);
	}
};

extern "C" __declspec(dllexport) int getCallBackSize()
{
	return sizeof(cppToCleoCallback);
}

extern "C" void runCleoCallback(injector::reg_pack &c, uintptr_t address)
{
	cppToCleoCallback *cb = (cppToCleoCallback*)address;

	if (cb)
	{
		cb->run(c);
	}
}

extern "C" __declspec(dllexport) injectcode::dynamic_hooker::newCode* generateFunction(cppToCleoCallback *nCallback, CRunningScript *owner, int atLabel, int nparams, int popBytes)
{
	if (atLabel < 0)
	{
		atLabel = -atLabel;
	}
	
	nCallback->address = (uintptr_t)nCallback;
	//nCallback->baseIp = owner->baseIP;
	nCallback->owner = owner;
	nCallback->nparams = nparams;

	nCallback->startIp = std::addressof(owner->baseIP[atLabel]);

	memset(std::addressof(nCallback->myCode), 0, sizeof(nCallback->myCode));

	injectcode::dynamic_hooker::genDynamicCall(nCallback->myCode, nCallback->address, runCleoCallback, popBytes);

	return std::addressof(nCallback->myCode);
}

