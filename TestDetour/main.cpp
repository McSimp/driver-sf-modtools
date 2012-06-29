#include <windows.h>
#include <cstdio>
#include "csimplescan.h"
#include "csimpledetour.h"
//#include "cdetour.h"
#include <stdio.h>
#include <stdarg.h>
#include <varargs.h>
#include "lua.h"

//#define LUAL_LOADFILE_SIG "\x81\xEC\x1C\x02\x00\x00\x53\x55\x8B\xAC\x24\x2C\x02\x00\x00\x56\x8B\xB4\x24\x2C\x02\x00\x00\x57\x8B\x7E\x08\x2B\x7E\x0C\xC7\x44\x24\x24\x00\x00\x00\x00\xC1\xFF\x03"
//#define LUAL_LOADFILE_MASK "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define RI_DOFILE_SIG "\x51\xA1\x28\x72\xFE\x00\xC7\x04\x24\x00\x00\x00\x00\x85\xC0\x75\x0A\xE8\xDA\xFC\xDA\xFF\xA3\x28\x72\xFE\x00"
#define RI_DOFILE_MASK "xxxxxxxxxxxxxxxxxxxxxxxxxxx"
int (*ri_dofile_Orig)(lua_State *, const char *) = 0;
int ri_dofile_New(lua_State *, const char *);

#define RI_LOADFILE_SIG "\x51\xA1\x28\x72\xFE\x00\xC7\x04\x24\x00\x00\x00\x00\x85\xC0\x75\x0A\xE8\x1A\xFC\xDA\xFF\xA3\x28\x72\xFE\x00"
#define RI_LOADFILE_MASK "xxxxxxxxxxxxxxxxxxxxxxxxxxx"
int (*ri_loadfile_Orig)(lua_State *, const char *) = 0;
int ri_loadfile_New(lua_State *, const char *);

#define LUA_PUSHCCLOSURE_SIG "\x53\x55\x56\x8B\x74\x24\x10\x8B\x46\x10\x8B\x48\x44\x57\x3B\x48\x40\x72\x09\x56\xE8\x00\x00\x00\x00"
#define LUA_PUSHCCLOSURE_MASK "xxxxxxxxxxxxxxxxxxxxx????"
void (*lua_pushcclosure_ss)(lua_State *, lua_CFunction, int) = 0;

#define LUAL_CHECKLSTRING_SIG "\x8B\x44\x24\x0C\x53\x56\x8B\x74\x24\x0C\x57\x8B\x7C\x24\x14\x50\x57\x56\xE8\x00\x00\x00\x00\x8B\xD8\x83\xC4\x0C\x85\xDB\x75\x48"
#define LUAL_CHECKLSTRING_MASK "xxxxxxxxxxxxxxxxxxx????xxxxxxxxx"
const char* (*luaL_checklstring_ss)(lua_State *, int, size_t *) = 0;

#define LUA_SETFIELD_SIG "\x8B\x4C\x24\x08\x83\xEC\x08\x53\x56\x8B\x74\x24\x14\x57\x8B\xD6\xE8\x00\x00\x00\x00\x8B\x54\x24\x20\x8B\xF8\x8B\xC2\x8D\x58\x01\x8A\x08\x40\x84\xC9\x75\xF9\x2B\xC3\x50\x52\x56\xE8\x00\x00\x00\x00\x89\x44\x24\x18\x8B\x46\x08\x83\xE8\x08\x50\x8D\x4C\x24\x1C\x51\x57\x56\xC7\x44\x24\x00\x00\x00\x00\x00\xE8\x00\x00\x00\x00\x83\xC4\x1C\x83\x46\x08\xF8\x5F\x5E\x5B\x83\xC4\x08\xC3"
#define LUA_SETFIELD_MASK "xxxxxxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxxxxxxxxxxxxxx?????x????xxxxxxxxxxxxxx"
void (*lua_setfield_ss)(lua_State *, int, const char *) = 0;

#define LUA_PCALL_SIG "\x8B\x4C\x24\x10\x83\xEC\x08\x56\x8B\x74\x24\x10\x85\xC9\x75\x04\x33\xC9\xEB\x0C\x8B\xD6\xE8\x00\x00\x00\x00\x2B\x46\x20\x8B\xC8\x8B\x44\x24\x14"
#define LUA_PCALL_MASK "xxxxxxxxxxxxxxxxxxxxxxx????xxxxxxxxx"
int (*lua_pcall_ss)(lua_State *, int, int, int) = 0;

#define LUAL_LOADBUFFER_SIG "\x83\xEC\x1C\x8B\x44\x24\x24\x8B\x4C\x24\x28\x89\x04\x24\x8B\x44\x24\x2C\x33\xD2\x89\x4C\x24\x04\x3B\xC2\x75\x05\xB8\x00\x00\x00\x00"
#define LUAL_LOADBUFFER_MASK "xxxxxxxxxxxxxxxxxxxxxxxxxxxxx????"
int (*luaL_loadbuffer_ss)(lua_State *, const char *, size_t, const char *) = 0;

bool injectedFunctions = false;
lua_State* luaPtr;
FILE* logFile;

//SETUP_SIMPLE_DETOUR( DrawError_Detour, CDetour::DrawError_T, CDetour::DrawError_H );
SETUP_SIMPLE_DETOUR(ri_dofile_Detour, ri_dofile_Orig, ri_dofile_New);
SETUP_SIMPLE_DETOUR(ri_loadfile_Detour, ri_loadfile_Orig, ri_loadfile_New);

void LogConsole( const char *szFmt, ... )
{
	va_list args;
	va_start( args, szFmt );

	int buffSize = _vscprintf( szFmt, args ) + 1;

	if ( buffSize <= 1 )
		return;

	char *szBuff = new char[ buffSize ];
	memset( szBuff, 0, buffSize );

	int len = vsprintf_s( szBuff, buffSize, szFmt, args );

	szBuff[ buffSize - 1 ] = 0;

	HANDLE hOutput = GetStdHandle( STD_OUTPUT_HANDLE );

	DWORD numWritten = 0;
	WriteFile( hOutput, szBuff, len, &numWritten, NULL );

	if(logFile != NULL)
	{
		fputs(szBuff, logFile);
		//fputs("\n", logFile);
		fflush(logFile);
	}

	delete [] szBuff;
}

void RegisterCFunction(lua_State* L, lua_CFunction func, const char* name)
{
	(*lua_pushcclosure_ss)(L, func, 0);
	(*lua_setfield_ss)(L, LUA_GLOBALSINDEX, name);

	LogConsole("[Lua C Funcs] %s registered in Lua\n", name);
}

static int lua_log(lua_State* L)
{
	const char *log = (*luaL_checklstring_ss)(L, 1, NULL);
	LogConsole("[Lua Log] %s\n", log);
	return 0;
}

int ri_loadfile_New(lua_State *L, const char *filename)
{
	LogConsole("[ri_loadfile] filename = %s\n", filename);
	return (*ri_loadfile_Orig)(L, filename);
}

int ri_dofile_New(lua_State *L, const char *filename)
{
	LogConsole("[ri_dofile] filename = %s\n", filename);
	luaPtr = L;

	if(!injectedFunctions)
	{
		RegisterCFunction(L, lua_log, "injectedLog");
		injectedFunctions = true;
	}

	return (*ri_dofile_Orig)(L, filename);
}

bool SigScanFunction(CSimpleScan &scan, const char *name, const char *sig, const char *mask, void **func)
{
	if(scan.FindFunction(sig, mask, func))
    {
		LogConsole("[SigScan] %s found at 0x%x\n", name, *func);
		return true;
	}
	else
	{
		LogConsole("[SigScan] %s not found\n", name);
		return false;
	}
}

typedef struct _INIT_STRUCT {
	LPCSTR Message;
} INIT_STRUCT, *PINIT_STRUCT;

extern "C" __declspec(dllexport) void ExecuteLua(PVOID message) {
	PINIT_STRUCT messageStruct = reinterpret_cast<PINIT_STRUCT>(message);
	int result = luaL_loadbuffer_ss(luaPtr, messageStruct->Message, strlen(messageStruct->Message), messageStruct->Message) || lua_pcall_ss(luaPtr, 0, LUA_MULTRET, 0);
	if(result == 0)
	{
		LogConsole("[Run Lua] %s ran successfully\n", messageStruct->Message);
	}
	else
	{
		LogConsole("[Run Lua] %s failed\n", messageStruct->Message);
	}
}

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		AllocConsole();
		logFile = fopen("injectedLog.txt", "w");
		if(logFile == NULL)
		{
			return TRUE;
		}

		LogConsole("====== Driver San Francisco Mod Injector Started ======\n");

		CSimpleScan driverGame("Driver.exe");
		
		bool success =
		SigScanFunction(driverGame, "ri_dofile", RI_DOFILE_SIG, RI_DOFILE_MASK, (void **)&ri_dofile_Orig)
		&& SigScanFunction(driverGame, "ri_loadfile", RI_LOADFILE_SIG, RI_LOADFILE_MASK, (void **)&ri_loadfile_Orig)
		&& SigScanFunction(driverGame, "luaL_checklstring", LUAL_CHECKLSTRING_SIG, LUAL_CHECKLSTRING_MASK, (void **)&luaL_checklstring_ss)
		&& SigScanFunction(driverGame, "lua_setfield", LUA_SETFIELD_SIG, LUA_SETFIELD_MASK, (void **)&lua_setfield_ss)
		&& SigScanFunction(driverGame, "lua_pushcclosure", LUA_PUSHCCLOSURE_SIG, LUA_PUSHCCLOSURE_SIG, (void **)&lua_pushcclosure_ss)
		&& SigScanFunction(driverGame, "lua_pcall", LUA_PCALL_SIG, LUA_PCALL_SIG, (void **)&lua_pcall_ss)
		&& SigScanFunction(driverGame, "luaL_loadbuffer", LUAL_LOADBUFFER_SIG, LUAL_LOADBUFFER_SIG, (void **)&luaL_loadbuffer_ss);
		
		if(!success)
		{
			LogConsole("[SigScan] A function was not found. Halting execution.\n");
			return TRUE;
		}

		ri_loadfile_Detour.Attach();
		LogConsole("[Detours] ri_loadfile detoured\n");

		ri_dofile_Detour.Attach();
		LogConsole("[Detours] ri_dofile detoured\n");
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		LogConsole("====== DLL Detached ======\n");

		ri_loadfile_Detour.Detach();
		ri_dofile_Detour.Detach();

		FreeConsole();

		fclose(logFile);
	}

	return TRUE;
}
