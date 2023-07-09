#pragma once

#pragma pack(push, 1)
typedef struct _CALAMITY_WPBT_VIEW {
	/* ACPI Standard Header */
	EFI_ACPI_DESCRIPTION_HEADER Header;
	/* WPBT Specific */
	UINT32 HandoffMemSize;
	UINT64 HandoffMemLocation;
	UINT8  ContentLayout;
	UINT8  ContentType;
	/* Native-Shell specific */
	UINT16 Argc;
	UINT64 Argv[0]; // May be variable
} CALAMITY_WPBT_VIEW, *PCALAMITY_WPBT_VIEW;
#pragma pack(pop)

EFI_STATUS
EFIAPI
CalamityInstall(
	VOID
);
