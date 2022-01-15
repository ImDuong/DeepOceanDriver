#pragma once

BOOLEAN CsEqualUnicodeStringWithRange(__in PCUNICODE_STRING String1, __in PCUNICODE_STRING String2, __in ULONG CompareLength, __in BOOLEAN CaseInSensitive);

NTSTATUS CsQuerySymbolicLinkObject(__in PUNICODE_STRING LinkSource, __inout PUNICODE_STRING LinkTarget, __out_opt PULONG ReturnedLength);

