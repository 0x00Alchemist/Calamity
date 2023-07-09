#include <Library/UefiLib.h>

#include "Globals.h"
#include "WPBT.h"

#pragma region VisualUefiConfig
CHAR8 *gEfiCallerBaseName = "Calamity";
CONST UINT32 _gUefiDriverRevision = 0;
CONST UINT8  _gDriverUnloadImageCount = 0;

BOOLEAN mPostEBS = FALSE;
EFI_SYSTEM_TABLE *mDebugST = NULL;
#pragma endregion


/**
  Unload routine.

  @param[in]		ImageHandle		Handle of current EFI driver

  @retval EFI_SUCCESS				EFI driver unloaded	
**/
EFI_STATUS
EFIAPI
UefiUnload(
	IN EFI_HANDLE ImageHandle
) {
	return EFI_SUCCESS;
}

/**
  Entry point of module.

  @param[in]		ImageHandle		Handle of current EFI driver
  @param[in]		ST				Pointer to EFI_SYSTEM_TABLE

  @retval EFI_SUCCESS				All things done
  @retval Other						Smth went wrong, check returned status
**/
EFI_STATUS
EFIAPI
UefiMain(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE *ST
) {
	gST = ST;
	gBS = gST->BootServices;

	gST->ConOut->ClearScreen(gST->ConOut);
	gST->ConOut->SetAttribute(gST->ConOut, (EFI_YELLOW | EFI_BACKGROUND_BLACK));

	Print(L"[Calamity] Loaded at: 0x%llx\r\n", ImageHandle);
	Print(L"[Calamity] Start install routine..\r\n");

	EFI_STATUS Status = CalamityInstall();
	if(EFI_ERROR(Status))
		Print(L"[Calamity] Installation failed!\r\n");

	return Status;
}
