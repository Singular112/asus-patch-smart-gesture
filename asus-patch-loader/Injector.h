#include "stdafx.h"

typedef std::basic_string<TCHAR> string_t;

/* minimum buf_size = 29 */
DWORD GetLastErrorDescription(DWORD _In_ last_error, LPTSTR _Out_ error_description, size_t _In_ buf_size);

namespace Injector
{
	bool IsModulePresent(DWORD pid, LPCTSTR moduleName, bool& out_presented);


	bool GetProcID(LPCTSTR proc_name, LPCTSTR parent_proc_name, DWORD& out_id);


	bool InjectDLL(LPCTSTR dll_path, LPCTSTR proc_name, LPCTSTR parent_proc_name = NULL);


	string_t GetLastError();
}
