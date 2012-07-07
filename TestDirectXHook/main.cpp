#include <windows.h>
#include <stdio.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "csimpledetour.h"

#pragma comment (lib, "d3d9.lib")

FILE* logFile;
HMODULE dllHandle;

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

typedef HRESULT (APIENTRY* EndScene) (IDirect3DDevice9*);
EndScene EndScene_orig = 0;
HRESULT APIENTRY EndScene_hook(IDirect3DDevice9*);
SETUP_SIMPLE_DETOUR(EndScene_Detour, EndScene_orig, EndScene_hook);

bool fontLoaded = false;
ID3DXFont *m_font;

HRESULT APIENTRY EndScene_hook(IDirect3DDevice9* pInterface)
{
	__asm pushad
	//LogConsole("[DirectX] EndScene\n");

	if(!fontLoaded)
	{
		D3DXCreateFont(pInterface, 20, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &m_font);
		fontLoaded = true;
	}

	D3DCOLOR fontColor = D3DCOLOR_ARGB(255,255,0,0);    

	// Create a rectangle to indicate where on the screen it should be drawn
	RECT rct;
	rct.left=2;
	rct.right=700;
	rct.top=50;
	rct.bottom=rct.top+20;
 
	// Draw some text 
	m_font->DrawText(NULL, L"I'VE BEEN INJECTED MUAHAHA", -1, &rct, 0, fontColor );

	__asm popad
	
	return EndScene_orig(pInterface);
}

DWORD WINAPI directXShiznit(__in  LPVOID lpParameter)
{
	IDirect3D9* d3d = NULL;
	IDirect3DDevice9* d3ddev = NULL;

	HWND tmpWnd = CreateWindowA("BUTTON", "Temporary Window", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, dllHandle, NULL);
	if(tmpWnd == NULL)
	{
		LogConsole("[DirectX] Failed to create temp window\n");
		return 0;
	}

	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if(d3d == NULL)
	{
		DestroyWindow(tmpWnd);
		LogConsole("[DirectX] Failed to create temp Direct3D interface\n");
		return 0;
	}

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp)); 
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = tmpWnd;

	HRESULT result = d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		tmpWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddev);
	if(result != D3D_OK)
	{
		d3d->Release();
		DestroyWindow(tmpWnd);
		LogConsole("[DirectX] Failed to create temp Direct3D device\n");
		return 0;
	}

	// We have the device, so walk the vtable to get the addrs of all the dx functions in d3d9.dll
	DWORD* dVtable = (DWORD*)d3ddev;
	dVtable = (DWORD*)dVtable[0]; // == *d3ddev
	LogConsole("[DirectX] dvtable: %x\n", dVtable);
	
	for(int i = 0; i < 43; i++)
	{
		LogConsole("[DirectX] vtable[%i]: %x, pointer at %x\n", i, dVtable[i], &dVtable[i]);
	}
	
	// Set EndScene_orig to the original EndScene
	EndScene_orig = (EndScene)dVtable[42];

	// Now detour EndScene
	EndScene_Detour.Attach();
	LogConsole("[Detours] EndScene detour attached\n");

	d3ddev->Release();
	d3d->Release();
	DestroyWindow(tmpWnd);
		
	return 1;
}

BOOL WINAPI DllMain( HMODULE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	if(fdwReason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hinstDLL);
		dllHandle = hinstDLL;

		AllocConsole();
		logFile = fopen("injectedLog.txt", "w");
		if(logFile == NULL)
		{
			return TRUE;
		}

		LogConsole("====== DirectX Hook test started ======\n");

		CreateThread(0, 0, directXShiznit, 0, 0, 0);
		//int result = directXShiznit();
		//LogConsole("Result = %i\n", result);
		//LogConsole("%x\n", g_deviceFunctionAddresses);
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		LogConsole("====== DLL Detached ======\n");

		FreeConsole();

		fclose(logFile);
	}

	return TRUE;
}

