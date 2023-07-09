#if defined(_M_X64) || defined(_M_AMD64)
#define _AMD64_
#elif defined(_M_IX86)
#error "The project assumes that only x64 applications are used!"
#endif

#include <ntddk.h>

#define VARIABLE_ATTRIBUTE_NON_VOLATILE			0x00000001
#define VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS	0x00000002
#define VARIABLE_ATTRIBUTE_RUNTIME_ACCESS		0x00000004


NTSYSCALLAPI
NTSTATUS
NTSYSAPI
ZwSetSystemEnvironmentValueEx(
	_In_ PUNICODE_STRING64 VariableName,
	_In_ LPGUID VariableGuid,
	_In_reads_bytes_opt_(ValueLength) PVOID Value,
	_In_ ULONG ValueLength,
	_In_ ULONG Attributes
);

VOID
NTAPI
CalamityEntry(
	VOID
) {
	UNICODE_STRING64 String;
	RtlInitUnicodeString(&String, L"CalamityNVRAM");

	GUID CalamityGuid = { 0xEAFB5F6D, 0x9D4B, 0x4F30, { 0xB5, 0x46, 0x7E, 0x22, 0xBE, 0xD1, 0xC5, 0xB4 } };
	UINT8 *Data[] = { 0x43, 0x61, 0x6c, 0x61, 0x6d, 0x69, 0x74, 0x79 };

	// Just create variable in NVRAM storage
	ZwSetSystemEnvironmentValueEx(&String, &CalamityGuid, (PVOID)Data, sizeof(Data), (VARIABLE_ATTRIBUTE_NON_VOLATILE | VARIABLE_ATTRIBUTE_BOOTSERVICE_ACCESS | VARIABLE_ATTRIBUTE_RUNTIME_ACCESS));
}
