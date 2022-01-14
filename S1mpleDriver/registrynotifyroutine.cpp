#include "pch.h"
#include "registrynotifyroutine.h"
#include "sysmon.h"
#include "s1mplecommon.h"
#include "notifyroutine.h"
#include "fastmutex.h"
#include "autolock.h"

extern Globals g_Globals;


NTSTATUS OnRegistryNotify(PVOID context, PVOID arg1, PVOID arg2) {
	UNREFERENCED_PARAMETER(context);

	static const WCHAR machine[] = L"\\REGISTRY\\MACHINE\\";

	auto status = STATUS_SUCCESS;

	switch ((REG_NOTIFY_CLASS)(ULONG_PTR)arg1) {
	case RegNtPostSetValueKey: {
		auto args = static_cast<REG_POST_OPERATION_INFORMATION*>(arg2);
		if (!NT_SUCCESS(args->Status))
			break;

		PCUNICODE_STRING name;
		if (NT_SUCCESS(CmCallbackGetKeyObjectIDEx(&g_Globals.RegCookie, args->Object, nullptr, &name, 0))) {
			// filter out none-HKLM writes
			if (::wcsncmp(name->Buffer, machine, ARRAYSIZE(machine) - 1) == 0) {
				auto preInfo = (REG_SET_VALUE_KEY_INFORMATION*)args->PreInformation;
				NT_ASSERT(preInfo);

				auto size = sizeof(FullItem<RegistrySetValueInfo>);
				auto info = (FullItem<RegistrySetValueInfo>*)ExAllocatePoolWithTag(PagedPool, size, DRIVER_TAG);
				if (info == nullptr)
					break;
				// zero out structure to make sure strings are null-terminated when copied
				RtlZeroMemory(info, size);
				// fill standard data
				auto& item = info->Data;
				KeQuerySystemTimePrecise(&item.Time);
				item.Size = sizeof(item);
				item.Type = ItemType::RegistrySetValue;
				// get specific key / value data
				::wcsncpy_s(item.KeyName, name->Buffer, name->Length / sizeof(WCHAR) - 1);
				::wcsncpy_s(item.ValueName, preInfo->ValueName->Buffer, preInfo->ValueName->Length / sizeof(WCHAR) - 1);
				item.DataType = preInfo->Type;
				item.DataSize = preInfo->DataSize;
				// get client PID/TID (this is our caller)
				item.ProcessId = HandleToULong(PsGetCurrentProcessId());
				item.ThreadId = HandleToULong(PsGetCurrentThreadId());
				::memcpy(item.Data, preInfo->Data, min(item.DataSize, sizeof(item.Data)));

				PushItem(&info->Entry);
			}
			CmCallbackReleaseKeyObjectIDEx(name);
		}
		break;
	}
	case RegNtPreSetValueKey:
	{
		if (g_Globals.RegItemCount > 0) {
			auto preInfo = static_cast<PREG_SET_VALUE_KEY_INFORMATION>(arg2);
			PCUNICODE_STRING keyName = nullptr;
			if (!NT_SUCCESS(CmCallbackGetKeyObjectID(&g_Globals.RegCookie, preInfo->Object, nullptr, &keyName))) {
				break;
			}

			//KdPrint(("Keyname to be Compared is: %wZ\n", keyName));

			AutoLock<FastMutex> lock(g_Globals.RegProtLock);

			PLIST_ENTRY pEntry = g_Globals.RegItemsHead.Flink;
			if (pEntry != NULL) {
				int i = 0;
				do {
					auto info = CONTAINING_RECORD(pEntry, FullItem<RegistryKeyProtectInfo*>, Entry);
					auto kName = (WCHAR*)&info->Data->KeyName;
					UNICODE_STRING tbcName;
					RtlInitUnicodeString(&tbcName, kName);
					KdPrint(("KeyName(U_C) In Linked List is: %wZ!\n", tbcName));

					if (RtlCompareUnicodeString(keyName, &tbcName, TRUE) == 0)
					{
						KdPrint(("Found a Matching Protected key. Blocking Any Modification Attempts: %wZ\n", tbcName));
						status = STATUS_CALLBACK_BYPASS;
						break;
					}
					pEntry = pEntry->Flink;
					i++;
				} while (i < g_Globals.RegItemCount && pEntry != &g_Globals.RegItemsHead);
			}
		}
		break;
	}

	}

	return status;
}