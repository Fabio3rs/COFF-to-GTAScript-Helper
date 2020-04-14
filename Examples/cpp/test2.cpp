/****************************************************************************
* http://bms.mixmods.com.br/t4721-
*
*
*/
/*
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
//#include "includes/top.h" // NÃO MAIS NECESSÁRIO
#include <cstring> // Protótipos das funções de string de C, como por exemplo strcmp, entre outras.
#include <cstdint>

struct CVehicle;

extern "C" char __cdecl showTextBox(const char *a1, char a2, char a3, char a4);

class testClass
{
 int a;
 const char *strA;
 
public:
 const char *getStrA() const
 {
 return strA;
 }
 
 // Suporte a construtores
 testClass() : strA("ABCDEF")
 {
 a = 10;
 }
};

testClass potassio;

struct externalCallbackStructure
{
 CVehicle *veh;
 int32_t status;
};

void callback(const externalCallbackStructure *test)
{
 
}

extern "C" int __declspec(dllexport) codeMain(int i)
{
 //testClass asdawdaw;
 
 asm("nop\n");
 showTextBox(potassio.getStrA(), 0, 0, 0);
 asm("nop\n");
 
 return 0;
}


///************************************   FINAL DO ARQUIVO CPP   ********************************************************
//#include "includes/btn.h" // NÃO MAIS NECESSÁRIO