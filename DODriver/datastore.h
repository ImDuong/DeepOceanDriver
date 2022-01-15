#pragma once

#include "FastMutex.h"

// headers for everything related to system monitor
// deoc driver
#define DRIVER_TAG 'coed'

// a struct containing all global variables -> avoid creating separate global variables
struct Globals {
	// variables related to the list of process, thread, image load notification
	LIST_ENTRY ItemsHead;
	int ItemCount;
	
	FastMutex NotiLock;

	void Init() {
		// initialize some important var
		// init the list of process, thread, image load notification
		//InitializeListHead(&g_Globals.ItemsHead);
		InitializeListHead(&ItemsHead);

		// load the mutex for process notification
		NotiLock.Init();
	}
};

// todo: get this limit item count from registry key (ZwOpenKey or IoOpenDeviceRegistryKey and then ZwQueryValueKey) instead of hardcode
#define MAX_ITEM_COUNT 1024

// use template to avoid creating multitude of types
template<typename T>
struct FullItem {
	// LINKED LIST object in kernel mode
	LIST_ENTRY Entry;
	T Data;
};

// the layout of FullItem<T> is as follows: 
// 1. LIST_ENTRY Entry 
// 2. T Data (data that transfer to client)
// 2.1. ItemHeader
// 2.2. Specific Data

// example of FullItem<ProcessExitInfo>
// 1. LIST_ENTRY Entry
// 2. ProcessExitInfo Data
// 2.1. ItemHeader: Type, Size & Time
// 2.2. ProcessId

// for code C, no ways to create such a natural template. Hence, we have to build multiple structs