#ifndef     HAL_AES_H_
#define     HAL_AES_H_

#include "ZComDef.h"

#define     STATE_BLENGTH   16      // Number of bytes in State
#define     KEY_BLENGTH     16      // Number of bytes in Key
#define     KEY_EXP_LENGTH  176     // Nb * (Nr+1) * 4

/* AES Engine is default to hardware AES. To turn on software AES, #define one of the followings:
 * #define SOFTWARE_AES TRUE, uses software aes  ( slowest setting )
 * #define SW_AES_AND_KEY_EXP TRUE, enables software aes with key expansion ( improves speed at the cost of 176 bytes of data (RAM) )
 */
#if ((defined SOFTWARE_AES) && (SOFTWARE_AES == TRUE)) && ((defined SW_AES_AND_KEY_EXP) && (SW_AES_AND_KEY_EXP == TRUE))
#error "SOFTWARE_AES and SW_AES_AND_KEY_EXP cannot be both defined."
#endif

extern void HalAesInit( void );
extern void AesLoadIV(uint8 *);
extern void AesDmaSetup( uint8 *, uint16, uint8 *, uint16 );
extern void AesLoadKey( uint8 * );

extern void (*pSspAesEncrypt)( uint8 *, uint8 * );
extern void ssp_HW_KeyInit (uint8 *);
extern void sspKeyExpansion (uint8 *, uint8 *);

extern void sspAesEncryptHW (uint8 *, uint8 *);
extern void sspAesEncryptKeyExp (uint8 *, uint8 *);
extern void sspAesEncryptBasic (uint8 *, uint8 *);

// Following are definitions needed for CC2430 hardware AES engine

#include "ioCC2430.h"

#define AES_BUSY    0x08
#define ENCRYPT     0x00
#define DECRYPT     0x01

// Macro for setting the mode of the AES operation
#define AES_SETMODE(mode) do { ENCCS &= ~0x70; ENCCS |= mode; } while (0)

// _mode_ is one of
#define CBC         0x00
#define CFB         0x10
#define OFB         0x20
#define CTR         0x30
#define ECB         0x40
#define CBC_MAC     0x50

// Macro for starting or stopping encryption or decryption
#define AES_SET_ENCR_DECR_KEY_IV(mode) \
   do {                                \
    ENCCS = (ENCCS & ~0x07) | mode     \
   } while(0)

// Where _mode_ is one of
#define AES_ENCRYPT     0x00;
#define AES_DECRYPT     0x02;
#define AES_LOAD_KEY    0x04;
#define AES_LOAD_IV     0x06;

// Macro for starting the AES module for either encryption, decryption,
// key or initialisation vector loading.
#define AES_START()     ENCCS |= 0x01

/* Used by DMA macros to shift 1 to create a mask for DMA registers. */
#define  HAL_DMA_AES_IN    1
#define  HAL_DMA_AES_OUT   2

/* AES registers */
#define  HAL_AES_IN_ADDR   0xDFB1
#define  HAL_AES_OUT_ADDR  0xDFB2

// End of CC2430 hardware AES engine definitions

#endif  // HAL_AES_H_

