#pragma once

#define S1MPLE_DEVICE 0x8069

#define IOCTL_S1MPLE_SET_PRIORITY CTL_CODE(S1MPLE_DEVICE, 0x800 + 1, METHOD_NEITHER, FILE_ANY_ACCESS)
// METHOD_OUT_DIRECT: used for Direct I/O Method and the caller will receive data from driver
#define IOCTL_S1MPLE_GET_ZERO_STATS CTL_CODE(S1MPLE_DEVICE, 0x800 + 2, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

// sample for the case we blacklist path by path
#define IOCTL_S1MPLE_BLOCK_PROCESS CTL_CODE(S1MPLE_DEVICE, 0x800 + 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

// for allowing process to run, most of the cases we will allow path by path -> use small amount of buffer
#define IOCTL_S1MPLE_ALLOW_PROCESS CTL_CODE(S1MPLE_DEVICE, 0x800 + 4, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_S1MPLE_PROTECT_PROCESS_BY_PID CTL_CODE(S1MPLE_DEVICE, 0x800 + 5, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_S1MPLE_UNPROTECT_PROCESS_BY_PID CTL_CODE(S1MPLE_DEVICE, 0x800 + 6, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_S1MPLE_CLEAR_PROTECT_PROCESS CTL_CODE(S1MPLE_DEVICE, 0x800 + 7, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_S1MPLE_REGKEY_PROTECT_ADD	CTL_CODE(S1MPLE_DEVICE, 0x800 + 8, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_S1MPLE_REGKEY_PROTECT_REMOVE	CTL_CODE(S1MPLE_DEVICE, 0x800 + 9, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_S1MPLE_REGKEY_PROTECT_CLEAR	CTL_CODE(S1MPLE_DEVICE, 0x800 +  10, METHOD_BUFFERED, FILE_ANY_ACCESS)


struct ThreadData {
	ULONG ThreadId; 
	int Priority;
};

struct ZeroStats {
	long long TotalRead;
	long long TotalWritten;
};

enum class ItemType : short {
	None, 
	ProcessCreate, 
	ProcessExit,
	ThreadCreate,
	ThreadExit,
	ImageLoad, 
	RegistrySetValue
};

struct ItemHeader {
	// event type
	ItemType Type;
	// payload size
	USHORT Size;
	// event time
	LARGE_INTEGER Time;
};

struct ProcessExitInfo : ItemHeader {
	ULONG ProcessId;
};

// We can replace ULONG with HANDLE or DWORD. But consider these points: 
// user mode can be confused with HANDLE
// although DWORD (32 bit unsigned integer) is commonly used in user mode, it is not defined in WDK headers. Hence, we need to define it explicity in kernel mode.

// note that in this Cpp coding style, ProcessExitInfo inherits ItemHeader. Hence, ProcessExitInfo can access Type, Size & Time right immediately
// hence, for accessing Type, for example, use ProcessExitInfo.Type

////for code C, we can simulate as follows: 
//struct ProcessExitInfo {
//	ItemHeader Header;
//	ULONG ProcessId;
//};

// in this C coding style, for accessing Type, for example, use ProcessExitInfo.ItemHeader.Type

struct ProcessCreateInfo : ItemHeader {
	ULONG ProcessId;
	ULONG ParentProcessId;
	USHORT CommandLineLength;
	USHORT CommandLineOffset;
};

// there are several ways to store the command line
// 1. `WCHAR CommandLine[1024]`: waste when command line is too short and too problematic when the command line is longer than the predefined size
// 2. `UNICODE_STRING CommandLine`: not defined in usermode and worse when internal pointer to actual characters relies in system space -> user mode cannot access
// 3. Store length and offset of command line: simple and adjustable with the size of cmd


struct ThreadCreateExitInfo : ItemHeader {
	ULONG ThreadId;
	ULONG ProcessId;
};

const int MaxImageFileSize = 300;

struct ImageLoadInfo : ItemHeader {
	ULONG ProcessId;
	void* LoadAddress;
	ULONG_PTR ImageSize;
	WCHAR ImageFileName[MaxImageFileSize + 1];
};

struct RegistrySetValueInfo : ItemHeader {
	ULONG ProcessId;
	ULONG ThreadId;
	WCHAR KeyName[256]; // full key name
	WCHAR ValueName[64]; // value name
	ULONG DataType; // REG_xxx
	UCHAR Data[128]; // data
	ULONG DataSize; // size of data
};
// use WCHAR[] for simplicity. For production-level driver, we should use dynamic solution

const int MaxRegNameSize = 300;
struct RegistryKeyProtectInfo {
	WCHAR KeyName[MaxRegNameSize]{};
};