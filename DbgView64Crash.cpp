// DbgView64Crash.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

BOOL GetDbgview64(_Out_ PULONG pProcessId)
{
	BOOL fOk = FALSE;

	if (HANDLE hEvent = OpenEventW(SYNCHRONIZE, FALSE, L"Local\\DBWIN_DATA_READY"))
	{
		union {
			PVOID buf;
			PSYSTEM_HANDLE_INFORMATION_EX pshi;
			POBJECT_NAME_INFORMATION poni;
		};

		ULONG cb = 0x10000;
		NTSTATUS status;
		do
		{
			status = STATUS_NO_MEMORY;

			if (buf = LocalAlloc(LMEM_FIXED, cb += 0x1000))
			{
				if (0 <= (status = NtQuerySystemInformation(SystemExtendedHandleInformation, buf, cb, &cb)))
				{
					if (ULONG_PTR NumberOfHandles = pshi->NumberOfHandles)
					{
						ULONG_PTR MyProcessId = GetCurrentProcessId();

						PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles = pshi->Handles;
						do
						{
							if (Handles->UniqueProcessId == MyProcessId && Handles->HandleValue == (ULONG_PTR)hEvent)
							{
								PVOID Object = Handles->Object;
								NumberOfHandles = pshi->NumberOfHandles;
								Handles = pshi->Handles;

								do
								{
									if (Object == Handles->Object && Handles->UniqueProcessId != MyProcessId)
									{
										MyProcessId = Handles->UniqueProcessId;

										if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (ULONG)MyProcessId))
										{
											const static UNICODE_STRING dbgview64 = RTL_CONSTANT_STRING(L"\\dbgview64.exe");
											PVOID wow;
											if (0 <= NtQueryInformationProcess(hProcess, ProcessWow64Information, &wow, sizeof(wow), 0) &&
												!wow &&
												0 <= NtQueryInformationProcess(hProcess, ProcessImageFileName, buf, cb, &cb) &&
												poni->Name.Length >= dbgview64.Length)
											{
												(ULONG_PTR&)poni->Name.Buffer += poni->Name.Length - dbgview64.Length;
												poni->Name.Length = dbgview64.Length;

												if (RtlEqualUnicodeString(&poni->Name, &dbgview64, TRUE))
												{
													fOk = TRUE;
													*pProcessId = (ULONG)MyProcessId;
												}
											}

											NtClose(hProcess);

											if (fOk)
											{
												break;
											}
										}
									}

								} while (Handles++, --NumberOfHandles);

								break;
							}
						} while (Handles++, --NumberOfHandles);
					}
				}
				LocalFree(buf);
			}

		} while (STATUS_INFO_LENGTH_MISMATCH == status);

		NtClose(hEvent);
	}

	return fOk;
}

BOOL IsDebugged(ULONG pid, _Out_ void** ppv)
{
	if (HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid))
	{
		ULONG cb;
		NTSTATUS status = NtQueryInformationProcess(hProcess, ProcessDebugPort, ppv, sizeof(void*), &cb);
		NtClose(hProcess);

		return 0 <= status;
	}

	return FALSE;
}

void CALLBACK ep(PVOID pv)
{
	ULONG pid = 0;
	while (!GetDbgview64(&pid) && 
		MessageBoxW(0, L"run dbgview64 and enable \"Capture Win32\"", 0, MB_ICONWARNING | MB_OKCANCEL) == IDOK);
	
	if (pid)
	{
		if (!IsDebugged(pid, &pv) || !pv)
		{
			WCHAR sz[0x80];
			if (0 < swprintf_s(sz, _countof(sz), L"Attach debugger to 0x%x (%u) process", pid, pid))
			{
				MessageBoxW(0, sz, L"", MB_ICONINFORMATION);
			}
		}

		while (IsDebuggerPresent() &&
			MessageBoxW(0, L"Detach Debugger from me, before OutputDebugStringA", L":))", MB_ICONWARNING | MB_OKCANCEL) == IDOK)
		{
		}

		OutputDebugStringA("*");
	}
	ExitProcess(0);
}
