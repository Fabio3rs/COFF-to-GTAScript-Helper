/*
 *  Dynamic_hooker inspired on LINK/2012 Injector
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
#pragma once
#ifndef _LUA_HOOKER_DYNAMIC_HOOKER_H_
#define _LUA_HOOKER_DYNAMIC_HOOKER_H_
#include <cstdint>
#include <injector\calling.hpp>
#include <algorithm>
#include <string>
#include <vector>
#define PACKED __attribute__ ((packed))

namespace injector
{
	#pragma pack(push, 1)
    struct reg_pack
    {
        // The ordering is very important, don't change
        // The first field is the last to be pushed and first to be poped

        // PUSHFD / POPFD
        uint32_t ef;

        // PUSHAD/POPAD -- must be the lastest fields (because of esp)
        union
        {
            uint32_t arr[8];
            struct { uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; };
        };

		void *retn;
        
        enum reg_name {
            reg_edi, reg_esi, reg_ebp, reg_esp, reg_ebx, reg_edx, reg_ecx, reg_eax 
        };
        
        enum ef_flag {
            carry_flag = 0, parity_flag = 2, adjust_flag = 4, zero_flag = 6, sign_flag = 7,
            direction_flag = 10, overflow_flag = 11
        };

        uint32_t& operator[](size_t i)
        { return this->arr[i]; }
        const uint32_t& operator[](size_t i) const
        { return this->arr[i]; }

        template<uint32_t bit>   // bit starts from 0, use ef_flag enum
        bool flag()
        {
            return (this->ef & (1 << bit)) != 0;
        }

        bool jnb()
        {
            return flag<carry_flag>() == false;
        }
    } PACKED;
	#pragma pack(pop)
}

namespace injectcode{
	typedef void(__cdecl *callback_t)(injector::reg_pack&, uintptr_t address);
	
	inline void callwrapper(uintptr_t retAddr, callback_t cb, uintptr_t address, injector::reg_pack *r)
	{
		cb(*r, address);
	}

	class dynamic_hooker{

	public:
	#pragma pack(push, 1)
		struct newCode
		{
			uint8_t pushad; // pushad

			// add[esp + 12], 4 
			uint8_t add;
			uint32_t data;

			uint8_t pushfd; // pushfd

			uint8_t pushesp; // push esp

			uint8_t push;
			uintptr_t pushaddr;
			uint8_t push2;
			uintptr_t useraddr;
			uint8_t push3;
			uintptr_t retaddr;

			// call callwrapper
			uint8_t call;
			uint32_t addr;

			// add esp, 16
			uint16_t addesp;
			uint8_t addespnum;

			// sub[esp + 12 + 4], 4
			uint8_t sub;
			uint32_t data1;


			uint8_t popfd; // popfd
			uint8_t popad; // popad


			// retn x
			uint8_t retn;
			uint16_t retnsiz;
		} PACKED;

	#pragma pack(pop)

	public:
		static void genDynamicCall(newCode &myCode, uintptr_t address, callback_t c, uintptr_t popBytes)
		{
			myCode = newCode{
				0x60,				// pushad
				0x80, 0x040C2444u,  // add[esp + 12], 4 
				0x9C,				 // pushfd

				0x54,			   // push esp

				0x68, address,      // push address
				0x68, uintptr_t(c), // push c
				0x68, 0x00,         // push <retnaddr>

				0xE8, 0x00,			// call callwrapper
				0xC483, 0x10,		// add esp, 16
				0x80, 0x0410246Cu, // sub[esp + 12 + 4], 4
				0x9D,				// popfd
				0x61,				// popad
				0xC2, 0x00			// retn x
			};

			myCode.retnsiz = popBytes;
			injector::MakeCALL(&(myCode.call), callwrapper, false);
		}

		inline dynamic_hooker()
		{

		}

		inline ~dynamic_hooker()
		{

		}
	} PACKED;


}

#endif


