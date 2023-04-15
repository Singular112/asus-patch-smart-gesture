// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the ASUSPATCHSMARTGESTURE_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// ASUSPATCHSMARTGESTURE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef ASUSPATCHSMARTGESTURE_EXPORTS
#define ASUSPATCHSMARTGESTURE_API __declspec(dllexport)
#else
#define ASUSPATCHSMARTGESTURE_API __declspec(dllimport)
#endif

// This class is exported from the asus-patch-smart-gesture.dll
class ASUSPATCHSMARTGESTURE_API Casuspatchsmartgesture {
public:
	Casuspatchsmartgesture(void);
	// TODO: add your methods here.
};

extern ASUSPATCHSMARTGESTURE_API int nasuspatchsmartgesture;

ASUSPATCHSMARTGESTURE_API int fnasuspatchsmartgesture(void);

BOOL ApplyPatch(BYTE eType, uint64_t dwAddress, const void* pTarget);

bool initialize();

extern volatile bool g_running;
extern std::thread g_main_thread;
