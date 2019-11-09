
#include "fsl_flash.h"
#include "fsl_debug_console.h"

#include "flash.h"

//------------------------------------------------------------------------------
#define BUFFER_LEN 4

//------------------------------------------------------------------------------
/*! @brief Flash driver Structure */
static flash_config_t s_flashDriver;

enum EStatus
{
    kUninitialized,
    kInitFailure,
    kSecured,       // flash memory is protected/secured. No writing/erasing possible
    kReady          // system is initialized and ready to use
};
static EStatus status;

static uint32_t pflashBlockBase = 0;
static uint32_t pflashTotalSize = 0;
static uint32_t pflashSectorSize = 0;

//------------------------------------------------------------------------------
int flash_init()
{
    flash_security_state_t securityStatus = kFLASH_SecurityStateNotSecure; /* Return protection status */
    status_t result;    /* Return code from each flash driver function */

    pflashBlockBase = 0;
    pflashTotalSize = 0;
    pflashSectorSize = 0;

    /* Clean up Flash driver Structure*/
    memset(&s_flashDriver, 0, sizeof(flash_config_t));

    /* Setup flash driver structure for device and initialize variables. */
    result = FLASH_Init(&s_flashDriver);
    if (kStatus_FLASH_Success != result)
    {
        PRINTF("FLASH_Init failure");
        status = kInitFailure;
        return status;
    }
    /* Get flash properties*/
    FLASH_GetProperty(&s_flashDriver, kFLASH_PropertyPflashBlockBaseAddr, &pflashBlockBase);
    FLASH_GetProperty(&s_flashDriver, kFLASH_PropertyPflashTotalSize, &pflashTotalSize);
    FLASH_GetProperty(&s_flashDriver, kFLASH_PropertyPflashSectorSize, &pflashSectorSize);

    /* print welcome message */
    PRINTF("\r\n PFlash Example Start \r\n");
    /* Print flash information - PFlash. */
    PRINTF("\r\n PFlash Information: ");
    PRINTF("\r\n Total Program Flash Size:\t%d KB, Hex: (0x%x)", (pflashTotalSize / 1024), pflashTotalSize);
    PRINTF("\r\n Program Flash Sector Size:\t%d KB, Hex: (0x%x) ", (pflashSectorSize / 1024), pflashSectorSize);

    /* Check security status. */
    result = FLASH_GetSecurityState(&s_flashDriver, &securityStatus);
    if (kStatus_FLASH_Success != result)
    {
        PRINTF("FLASH_GetSecurityState failure");
        status = kInitFailure;
        return status;
    }
    /* Print security status. */
    switch (securityStatus)
    {
        case kFLASH_SecurityStateNotSecure:
            PRINTF("\r\n Flash is UNSECURE!");
            break;
        case kFLASH_SecurityStateBackdoorEnabled:
            PRINTF("\r\n Flash is SECURE, BACKDOOR is ENABLED!");
            break;
        case kFLASH_SecurityStateBackdoorDisabled:
            PRINTF("\r\n Flash is SECURE, BACKDOOR is DISABLED!");
            break;
        default:
            break;
    }
    PRINTF("\r\n");

    if (kFLASH_SecurityStateNotSecure == securityStatus)
    {
        PRINTF("flash_init() complete. Write/Erase is permitted!\n");
        status = kReady;
    }
    else
    {
        PRINTF("flash_init() complete. But memory is protected/secured. No write/erase permitted!\n");
        status = kSecured;
    }
    return status;
}

//------------------------------------------------------------------------------
int flash_ready()
{
    return status == kReady;
}

//------------------------------------------------------------------------------
// Base address is set to last available sector of flash memory
uint32_t flash_get_eeprom_base()
{
#if defined(FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP) && FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP
    /* Note: we should make sure that the sector shouldn't be swap indicator sector*/
    uint32_t destAdrss = pflashBlockBase + (pflashTotalSize - (pflashSectorSize * 2));
#else
    uint32_t destAdrss = pflashBlockBase + (pflashTotalSize - pflashSectorSize);
#endif
    return destAdrss;
}

//------------------------------------------------------------------------------
int flash_read(void *dest, unsigned sizeBytes)
{
    if (status != kReady)
    {
        return -1;
    }

#if defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
    /* Clean the D-Cache before reading the flash data*/
    SCB_CleanInvalidateDCache();
#endif

    uint32_t eepromAddr = flash_get_eeprom_base();
    uint8_t *pDst = (uint8_t*)dest;
    int i;
    for (i=0; i<(int)sizeBytes; i++)
    {
        pDst[i] = ((volatile uint8_t*)eepromAddr)[i];
    }
    return i;
}

//------------------------------------------------------------------------------
int flash_write(const void *src, unsigned sizeBytes)
{
    if (status != kReady)
    {
        return 0;
    }

    if (0x3 & (int)src)
    {
        PRINTF("flash_write source buffer isn't 32-bit aligned\n");
        return 0;
    }

    if (sizeBytes & 3)
    {
        PRINTF("flash_write sizeBytes isn't 32-bit aligned\n");
        sizeBytes = (sizeBytes + 3) & ~0x3;
    }

    /* Erase last sector on upper pflash block where there is (hopefully, ha-ha) no code */
    PRINTF("Erasing last sector of flash\n");
    uint32_t destAdrss = flash_get_eeprom_base();
    status_t result = FLASH_Erase(&s_flashDriver, destAdrss, pflashSectorSize, kFLASH_ApiEraseKey);
    if (kStatus_FLASH_Success != result)
    {
        PRINTF("FLASH_Erase failure\n");
        return 0;
    }

    /* Verify sector if it's been erased. */
    result = FLASH_VerifyErase(&s_flashDriver, destAdrss, pflashSectorSize, kFLASH_MarginValueUser);
    if (kStatus_FLASH_Success != result)
    {
        PRINTF("FLASH_VerifyErase failure\n");
        return 0;
    }

    PRINTF("Writing buffer to last sector of flash\n");
    result = FLASH_Program(&s_flashDriver, destAdrss, (uint32_t*)src, sizeBytes);
    if (kStatus_FLASH_Success != result)
    {
        PRINTF("FLASH_Program failure\n");
        return 0;
    }

    /* Verify programming by Program Check command with user margin levels */
    uint32_t failAddr, failDat;
    result = FLASH_VerifyProgram(&s_flashDriver, destAdrss, sizeBytes, (uint32_t*)src, kFLASH_MarginValueUser,
                                    &failAddr, &failDat);
    if (kStatus_FLASH_Success != result)
    {
        PRINTF("FLASH_VerifyProgram failure\n");
        return 0;
    }

#if defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
    /* Clean the D-Cache before reading the flash data*/
    SCB_CleanInvalidateDCache();
#endif

    /* Verify programming by reading back from flash directly*/
    const uint32_t *pSrcCheck = (uint32_t*)src;
    for (uint32_t i = 0; i < (sizeBytes/4); i++)
    {
        uint32_t check = *(volatile uint32_t *)(destAdrss + i * 4);
        if (check != pSrcCheck[i])
        {
            PRINTF("byte verify failure\n");
            return 0;
        }
    }
    return sizeBytes;
}


//------------------------------------------------------------------------------
void flash_test()
{
    if (status == kUninitialized)
    {
        flash_init();
    }
    if (status != kReady)
    {
        return;
    }

    static bool tested;
    if (tested)
    {
        return;
    }
    tested = true;

    /* Erase last sector on upper pflash block where there is (hopefully, ha-ha) no code */
    PRINTF("Erasing last sector of flash");

    /* Erase a sector from destAdrss. */
    uint32_t destAdrss; /* Address of the target location */
#if defined(FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP) && FSL_FEATURE_FLASH_HAS_PFLASH_BLOCK_SWAP
    /* Note: we should make sure that the sector shouldn't be swap indicator sector*/
    destAdrss = pflashBlockBase + (pflashTotalSize - (pflashSectorSize * 2));
#else
    destAdrss = pflashBlockBase + (pflashTotalSize - pflashSectorSize);
#endif
    status_t result = FLASH_Erase(&s_flashDriver, destAdrss, pflashSectorSize, kFLASH_ApiEraseKey);
    if (kStatus_FLASH_Success != result)
    {
        PRINTF("FLASH_Erase failure\n");
        while (1)
            ;
    }

    /* Verify sector if it's been erased. */
    result = FLASH_VerifyErase(&s_flashDriver, destAdrss, pflashSectorSize, kFLASH_MarginValueUser);
    if (kStatus_FLASH_Success != result)
    {
        PRINTF("FLASH_VerifyErase failure\n");
        while (1)
            ;
    }

    /* Print message for user. */
    PRINTF("Successfully Erased Sector 0x%x -> 0x%x\n", destAdrss, (destAdrss + pflashSectorSize));

    /* Print message for user. */
    PRINTF("Program a buffer to a sector of flash\n");


    /* Prepare user buffer. */
    uint32_t s_buffer[BUFFER_LEN];
    for (uint32_t i = 0; i < BUFFER_LEN; i++)
    {
        s_buffer[i] = i;
    }
    /* Program user buffer into flash*/
    result = FLASH_Program(&s_flashDriver, destAdrss, s_buffer, sizeof(s_buffer));
    if (kStatus_FLASH_Success != result)
    {
        PRINTF("FLASH_Program failure\n");
        while (1)
            ;
    }

    /* Verify programming by Program Check command with user margin levels */
    uint32_t failAddr, failDat;
    result = FLASH_VerifyProgram(&s_flashDriver, destAdrss, sizeof(s_buffer), s_buffer, kFLASH_MarginValueUser,
                                    &failAddr, &failDat);

    if (kStatus_FLASH_Success != result)
    {
        PRINTF("FLASH_VerifyProgram failure\n");
        while (1)
            ;
    }

#if defined(__DCACHE_PRESENT) && __DCACHE_PRESENT
    /* Clean the D-Cache before reading the flash data*/
    SCB_CleanInvalidateDCache();
#endif
    /* Verify programming by reading back from flash directly*/
    uint32_t s_buffer_rbc[BUFFER_LEN];
    for (uint32_t i = 0; i < BUFFER_LEN; i++)
    {
        s_buffer_rbc[i] = *(volatile uint32_t *)(destAdrss + i * 4);
        if (s_buffer_rbc[i] != s_buffer[i])
        {
            PRINTF("byte verify failure\n");
            while (1)
                ;
        }
    }

    PRINTF("Successfully Programmed and Verified Location 0x%x -> 0x%x\n", destAdrss,
            (destAdrss + sizeof(s_buffer)));

    /* Erase the context we have progeammed before*/
    /* Note: we should make sure that the sector which will be set as swap indicator should be blank*/
    FLASH_Erase(&s_flashDriver, destAdrss, pflashSectorSize, kFLASH_ApiEraseKey);
}
