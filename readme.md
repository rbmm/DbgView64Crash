if run dbgview64.exe from [DebugView v4.90](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview)
it will crashed after some time.
mandatory enable `Capture Win32`( or/and `Capture Global Win32`
for force crash - call OutputDebugStringA("*") not under debugger.

(the DbgView64Crash.exe simply do this, nothing more, work when `Capture Win32` enabled)

who can found exactly reason and source place of crash ?
