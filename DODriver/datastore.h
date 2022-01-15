#pragma once

#include "FastMutex.h"

// headers for everything related to system monitor
// deoc driver
#define DRIVER_TAG 'coed'

// prefix for kdprint
#define DO_DRIVER_PREFIX "Deep Ocean Service: "

// a struct containing all global variables -> avoid creating separate global variables
struct Globals {
	// variables related to the list of process load notification
	LIST_ENTRY ProcNotiItemsHead;
	int ProcNotiItemsCount;
	FastMutex ProcNotiLock;

	// variables for blocking program
	LIST_ENTRY ProgItemsHead;
	int ProgItemsCount;
	FastMutex ProgLock;


	void Init() {
		// initialize some important var
		// init the list of process notification
		InitializeListHead(&ProcNotiItemsHead);

		// init the list of process notification
		InitializeListHead(&ProgItemsHead);

		// load the mutex for process notification
		ProcNotiLock.Init();

		// load the mutex for process notification
		ProgLock.Init();
	}
};

// todo: get this limit item count from registry key (ZwOpenKey or IoOpenDeviceRegistryKey and then ZwQueryValueKey) instead of hardcode
#define MAX_PROC_NOTI_COUNT 1024
#define MAX_PROG_BLOCK_COUNT 100

// use template to avoid creating multitude of types
template<typename T>
struct DOItem {
	// LINKED LIST object in kernel mode
	LIST_ENTRY Entry;
	T Data;
};

// the layout of DOItem<T> is as follows: 
// 1. LIST_ENTRY Entry 
// 2. T Data (data that transfer to client)
// 2.1. ItemHeader
// 2.2. Specific Data

// example of DOItem<ProcessExitInfo>
// 1. LIST_ENTRY Entry
// 2. ProcessExitInfo Data
// 2.1. ItemHeader: Type, Size & Time
// 2.2. ProcessId

// for code C, no ways to create such a natural template. Hence, we have to build multiple structs