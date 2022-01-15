#include "pch.h"
#include "drivercommon.h"
#include "commonhelpers.h"
#include "datastore.h"


BOOLEAN CsEqualUnicodeStringWithRange(__in PCUNICODE_STRING String1, __in PCUNICODE_STRING String2, __in ULONG CompareLength, __in BOOLEAN CaseInSensitive) {
    // defensive programming: These strings can come from somebody else's code (ref: lordjeb)
    if (String1 == NULL || String2 == NULL || !String1->Buffer || 0 == String1->Length || !String2->Buffer || 0 == String2->Length)
    {
        return FALSE;
    }
    // compare strings whose length > 0 and length >= CompareLength
    if (String1->Length < CompareLength || String2->Length < CompareLength) {
        return FALSE;
    }
    // compare the buffers from 2 strings
    // 
    // this solution is temporarily wiped out because I still can't find a good function for lower a wide string
    //return CaseInSensitive ? (::wcsncmp(_wcslwr_s(String1->Buffer), ::wcslwr(String2->Buffer), CompareLength) == 0) : (::wcsncmp(String1->Buffer, String2->Buffer, CompareLength) == 0);

    // naive implementation for comparison: loop for each char :)
    //KdPrint((DRIVER_PREFIX "Length is: %d\r\n", CompareLength));
    for (int i = 0; i < (int)CompareLength; i++) {
        if (!CaseInSensitive) {
            if (String1->Buffer[i] != String2->Buffer[i]) {
                return FALSE;
            }
        }
        else {
            //KdPrint((DRIVER_PREFIX "view the character successfully: %d - %d\r\n", tolower(String1->Buffer[i]), tolower(String2->Buffer[i])));
            if (tolower(String1->Buffer[i]) != tolower(String2->Buffer[i])) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

// smart but maibe dangerous method: automatically reallocate the buffer of LinkTarget to query the symbolic link
NTSTATUS CsQuerySymbolicLinkObject(__in PUNICODE_STRING LinkSource, __inout PUNICODE_STRING LinkTarget, __out_opt PULONG ReturnedLength) {
    // defensive programming: These strings can come from somebody else's code (ref: lordjeb)
    // to avoid warning code C6001: describe the parameter LinkTarget as __inout
    if (!LinkSource || !LinkTarget || LinkTarget->Buffer == NULL || LinkSource->Buffer == NULL) {
        return STATUS_NO_MEMORY;
    }

    NTSTATUS status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES symLinkAttr;
    HANDLE hSymlink;
    ULONG objectLength;

    InitializeObjectAttributes(&symLinkAttr, LinkSource, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    if (!NT_SUCCESS(status = ZwOpenSymbolicLinkObject(&hSymlink, GENERIC_READ, &symLinkAttr))) {
        KdPrint((DO_DRIVER_PREFIX "open the object Failed: %08x\r\n", status));
        return status;
    }
    //KdPrint((DRIVER_PREFIX "open the object %wZ successfully\r\n", LinkSource));

    do {
        if (!NT_SUCCESS(status = ZwQuerySymbolicLinkObject(hSymlink, LinkTarget, &objectLength))) {
            if (status == STATUS_BUFFER_TOO_SMALL) {
                // deallocate the buffer of LinkTarget
                ExFreePool(LinkTarget->Buffer);
                // reallocate the buffer of LinkTarget
                LinkTarget->MaximumLength = (USHORT)objectLength;
                LinkTarget->Buffer = (PWSTR)ExAllocatePoolWithTag(NonPagedPool, objectLength, DRIVER_TAG);
                // now we will query again with correct length
                if (!NT_SUCCESS(status = ZwQuerySymbolicLinkObject(hSymlink, LinkTarget, ReturnedLength))) {
                    break;
                }
            }
        }
        ReturnedLength = &objectLength;
    } while (false);

    if (NT_SUCCESS(status)) {
        //KdPrint((DRIVER_PREFIX "query the object name successfully: %wZ\r\n", LinkTarget));
    }
    else {
        KdPrint((DO_DRIVER_PREFIX "query the object name failed: %08x\r\n", status));
    }
    //KdPrint((DRIVER_PREFIX "require length: %d\r\n", objectLength));
    if (hSymlink) {
        ZwClose(hSymlink);
    }
    return status;
}