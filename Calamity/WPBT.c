#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>

#include <Protocol/AcpiTable.h>

#include <IndustryStandard/PeImage.h>
#include <IndustryStandard/Acpi65.h>

#include "Globals.h"
#include "WPBT.h"
#include "Driver.h"

// Default WPBT size by specification
#define WPBT_SIZE						52

// Supported content types by specification
#define WPBT_CONTENT_TYPE_SINGLE_PE		1
#define WPBT_CONTENT_LAYOUT_NATIVE		1


/**
  Filling WPBT structure.

  @param[out]   CalamityWpbt	Pointer to WPBT structure which we fill

  @retval None
**/
VOID
EFIAPI
InitializeCalamityWpbt(
	OUT PCALAMITY_WPBT_VIEW CalamityWpbt
) {
	EFI_ACPI_DESCRIPTION_HEADER Header;
	Header.Signature = SIGNATURE_32('W', 'P', 'B', 'T');
	Header.Length = WPBT_SIZE;
	Header.Revision = 1;
	Header.OemTableId = SIGNATURE_64('S', 'K', 'V', 'L', 'L', 'L', 'L', 'Z');
	Header.OemRevision = 1; // Do not change!
	Header.CreatorId = SIGNATURE_32('F', 'V', 'C', 'K');
	Header.CreatorRevision = 1;

	// Copy OemId (110% autistic move)
	UINT8 OemId[] = { 'S', 'K', 'V', 'L', 'L', 'Z' };
	CopyMem(Header.OemId, OemId, sizeof(OemId));

	// Copy current header to our WPBT structure
	CalamityWpbt->Header = Header;
	
	// Fill WPBT specific info.
	// Info like memory location, driver size and checksum will
	// be initialized later. Argc and Argv fields not used in this project.
	CalamityWpbt->ContentType = WPBT_CONTENT_TYPE_SINGLE_PE;
	CalamityWpbt->ContentLayout = WPBT_CONTENT_LAYOUT_NATIVE;
}

/**
  Prepares the PE image that will be used when installing WPBT.

  @param[out]   MemoryBlock		Pointer to PE in ACPI memory
  @param[out]	DriverSize		Size of PE image

  @retval EFI_SUCCESS			PE image prepared succesfully
  @retval EFI_UNSUPPORTED		Cannot find MZ or PE signature of PE image
  @retval Other					Cannot allocate ACPI memory for PE image (check returned status)
**/
EFI_STATUS
EFIAPI
PrepareWpbtImage(
	OUT VOID	**MemoryBlock,
	OUT UINT32	*DriverSize
) {
	// Get headers, then get file size
	EFI_IMAGE_DOS_HEADER *Dos;
	EFI_IMAGE_NT_HEADERS64 *Nt64;

	Dos = (EFI_IMAGE_DOS_HEADER *)Driver;
	Nt64 = (EFI_IMAGE_NT_HEADERS64 *)(Driver + Dos->e_lfanew);
	
	UINT32 Size = Nt64->OptionalHeader.SizeOfImage;

	// WPBT binaries must use ACPI memory (by specification)
	VOID *Buffer;
	EFI_STATUS Status = gBS->AllocatePool(EfiACPIReclaimMemory, Size, &Buffer);
	if(EFI_ERROR(Status)) {
		Print(L"[Calamity] Cannot allocate ACPI memory! Status: %r (0x%llx)\r\n", Status, Status);
		return Status;
	}

	*MemoryBlock = Buffer;
	*DriverSize = Size;

	return EFI_SUCCESS;
}

/**
  Installing WPBT in system.

  @param[in]		MemoryBlock		Pointer to PE in ACPI memory
  @param[in]		DriverSize		Size of PE image
  @param[in, out]	CalamityWpbt	WPBT structure

  @retval EFI_SUCCESS				WPBT installed succesfully
  @retval Other						Cannot install WPBT or locate ACPI protocol (check returned status)
**/
EFI_STATUS
EFIAPI
InstallWpbt(
	IN		VOID				*HandoffLocation,
	IN		UINT32				HandoffSize,
	IN OUT	PCALAMITY_WPBT_VIEW	CalamityWpbt
) {
	// Locate ACPI protocol
	EFI_ACPI_TABLE_PROTOCOL *Acpi;
	EFI_STATUS Status = gBS->LocateProtocol(&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&Acpi);
	if(EFI_ERROR(Status)) {
		Print(L"[Calamity] Cannot locate \"EFI_ACPI_TABLE_PROTOCOL\" protocol! Status: %r (0x%llx)\r\n", Status, Status);
		return Status;
	}

	// Allocate memory for WPBT
	VOID *WpbtBuf;
	Status = gBS->AllocatePool(EfiBootServicesData, WPBT_SIZE, (VOID **)&WpbtBuf);
	if(EFI_ERROR(Status)) {
		Print(L"[Calamity] Cannot allocate buffer for WPBT! Status: %r (0x%llx)\r\n", Status, Status);
		return Status;
	}

	// Fill remaining fields
	CalamityWpbt->HandoffMemLocation = HandoffLocation;
	CalamityWpbt->HandoffMemSize = HandoffSize;

	// Calculate checksum of WPBT
	CalamityWpbt->Header.Checksum = CalculateCheckSum8((UINT8 *)CalamityWpbt, CalamityWpbt->Header.Length);

	// Move our WPBT
	CopyMem(WpbtBuf, CalamityWpbt, WPBT_SIZE);

	// Install WPBT into ACPI
	UINTN TableKey;
	Status = Acpi->InstallAcpiTable(Acpi, WpbtBuf, WPBT_SIZE, &TableKey);
	if(EFI_ERROR(Status)) {
		Print(L"[Calamity] Cannot install WPBT! Status: %r (0x%llx)\r\n", Status, Status);
		return Status;
	}

	return EFI_SUCCESS;
}

/**
  Main routine of WPBT installing.

  @param[in]					None

  @retval EFI_SUCCESS			WPBT installed succesfully
  @retval EFI_ACCESS_DENIED		WPBT already installed in the system
  @retval Other					While WPBT installing we received error (check returned status)
**/
EFI_STATUS
EFIAPI
CalamityInstall(
	VOID
) {
	// First of all, we should check if our (or other) WPBT
	// already exists
	if(EfiLocateFirstAcpiTable(SIGNATURE_32('W', 'P', 'B', 'T')) != NULL) {
		Print(L"[Calamity] Looks like WPBT already exists!\r\n");
		return EFI_ACCESS_DENIED;
	}

	// Fill Calamity WPBT structure
	CALAMITY_WPBT_VIEW CalamityWpbt;
	InitializeCalamityWpbt(&CalamityWpbt);

	// Now we should prepare our driver
	VOID *HandoffLocation;
	UINT32 HandoffSize;
	EFI_STATUS Status = PrepareWpbtImage(&HandoffLocation, &HandoffSize);
	if(EFI_ERROR(Status))
		return Status;

	// Install WPBT
	Status = InstallWpbt(HandoffLocation, HandoffSize, &CalamityWpbt);
	if(EFI_ERROR(Status))
		return Status;

	gST->ConOut->SetAttribute(gST->ConOut, (EFI_LIGHTGREEN | EFI_BACKGROUND_BLACK));

	Print(L"[Calamity] WPBT succesfully installed!\r\n");
	Print(L"[Calamity] WPBT location: 0x%llx\r\n", CalamityWpbt.HandoffMemLocation);
	Print(L"[Calamity] WPBT size: %ul\r\n", CalamityWpbt.HandoffMemSize);

	gST->ConOut->SetAttribute(gST->ConOut, (EFI_YELLOW | EFI_BACKGROUND_BLACK));

	return EFI_SUCCESS;
}
