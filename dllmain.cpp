#include <stdafx.h>

#include "labels.h"
#include "main.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		MH_Initialize();

		labels::init_labels();

		MH_EnableHook(MH_ALL_HOOKS);

		scriptRegister(hInstance, ballgame::script_main);
		keyboardHandlerRegister(ballgame::keyboard_handle);

		break;
	case DLL_PROCESS_DETACH:
		MH_Uninitialize();

		scriptUnregister(hInstance);
		keyboardHandlerUnregister(ballgame::keyboard_handle);

		break;
	}

	return TRUE;
}