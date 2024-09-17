#ifdef _MSC_VER
#include <windows.h>

#define EXPORT __declspec(dllexport)

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
      // Code to run when the DLL is loaded
      break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      // Code to run when the DLL is unloaded
      break;
  }
  return TRUE;
}
#else
#define EXPORT __attribute__((visibility("default")))

__attribute__((constructor)) static void initializer(void) {}

__attribute__((destructor)) static void finalizer(void) {}

#endif

EXPORT
void actor(void) {}
