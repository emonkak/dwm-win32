/* 
 * dwm-win32 satusbar format API
 */

#define _WIN32_WINNT            0x0500

#include <stdio.h>
#include <time.h>
#include <windows.h>

#define SystemBasicInformation       0
#define SystemPerformanceInformation 2
#define SystemTimeInformation        3

/* macros */
#define LI2DOUBLE(x) ((double)((x).u.HighPart) * 4.294967296E9 + (double)((x).u.LowPart))

typedef struct
{
	DWORD dwUnknown1;
	ULONG uKeMaximumIncrement;
	ULONG uPageSize;
	ULONG uMmNumberOfPhysicalPages;
	ULONG uMmLowestPhysicalPage;
	ULONG uMmHighestPhysicalPage;
	ULONG uAllocationGranularity;
	PVOID pLowestUserAddress;
	PVOID pMmHighestUserAddress;
	ULONG uKeActiveProcessors;
	BYTE bKeNumberProcessors;
	BYTE bUnknown2;
	WORD wUnknown3;
} SYSTEM_BASIC_INFORMATION;

typedef struct
{
	LARGE_INTEGER liIdleTime;
	DWORD dwSpare[76];
} SYSTEM_PERFORMANCE_INFORMATION;

typedef struct {
	LARGE_INTEGER liKeBootTime;
	LARGE_INTEGER liKeSystemTime;
	LARGE_INTEGER liExpTimeZoneBias;
	ULONG uCurrentTimeZoneId;
	DWORD dwUnknown1[5];
} SYSTEM_TIME_INFORMATION;

typedef LONG (WINAPI *PNTQSI)(UINT, PVOID, ULONG, PULONG);
static PNTQSI NtQuerySystemInformation;

/* variables */
static HMODULE ntdll;
static SYSTEM_BASIC_INFORMATION sysinfo;


void
statussetup()
{
	ntdll = GetModuleHandle("ntdll.dll");
	if(ntdll == NULL)
		return;

	NtQuerySystemInformation = (PNTQSI)
	                           GetProcAddress(ntdll, "NtQuerySystemInformation");
	if(NtQuerySystemInformation == NULL) {
		FreeLibrary(ntdll);
		ntdll = NULL;
		return;
	}

	NtQuerySystemInformation(SystemBasicInformation, &sysinfo,
	                         sizeof(sysinfo), NULL);
}

void
statuscleanup()
{
	FreeLibrary(ntdll);
	ntdll = NULL;
	NtQuerySystemInformation = NULL;
}

int
getcpu()
{
	SYSTEM_PERFORMANCE_INFORMATION cpu;
	SYSTEM_TIME_INFORMATION time;
	double idle, total;
	static double pidle = 0, ptotal = 0;
	int usage = 0;

	if(NtQuerySystemInformation == NULL)
		return -1;

	/* get cpu idle time */
	if(NtQuerySystemInformation(SystemPerformanceInformation, &cpu,
	                            sizeof(cpu), NULL) != NO_ERROR)
		return -1;

	/* get system time */
	if(NtQuerySystemInformation(SystemTimeInformation, &time,
	                            sizeof(time), 0) != NO_ERROR)
		return -1;

	idle = LI2DOUBLE(cpu.liIdleTime);
	total = LI2DOUBLE(time.liKeSystemTime);

	if(pidle != 0)
		usage = 100 - ((idle - pidle) / (total - ptotal) * 100
		               / sysinfo.bKeNumberProcessors);

	pidle = idle;
	ptotal = total;

	return usage;
}

int
getmemory()
{
	MEMORYSTATUS memory;

	GlobalMemoryStatus(&memory);

	return (int)memory.dwMemoryLoad;
}

size_t
gettime(char* str, size_t maxsize, const char* format)
{
	time_t timer;
	struct tm *date;

	time(&timer);
	date = localtime(&timer);

	return strftime(str, maxsize, format, date);
}
