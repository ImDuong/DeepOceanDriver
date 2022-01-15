#pragma once

#define S1MPLE_DEVICE 0x8069

enum class ItemType : short {
	None, 
	ProcessCreate, 
	ProcessExit,
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