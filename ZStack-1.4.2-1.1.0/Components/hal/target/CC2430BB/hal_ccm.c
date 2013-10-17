/******************************************************************************
    Filename:       ccm.c
    Revised:        $Date: 2006-07-25 09:14:25 -0700 (Tue, 25 Jul 2006) $
    Revision:       $Revision: 11447 $

    Description:

    Describe the purpose and contents of the file.

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
******************************************************************************/
/******************************************************************************
 * INCLUDES
 */

#include "osal.h"
#include "hal_aes.h"
#include "hal_dma.h"

/******************************************************************************
 * MACROS
 */

/******************************************************************************
 * CONSTANTS
 */

/******************************************************************************
 * TYPEDEFS
 */

/******************************************************************************
 * LOCAL VARIABLES
 */

/******************************************************************************
 * GLOBAL VARIABLES
 */

/******************************************************************************
 * FUNCTION PROTOTYPES
 */

void SSP_CCM_Auth (uint8, uint8 *, uint8 *, uint16, uint8 *,
                                                uint16, uint8 *, uint8 *);
void SSP_CCM_Encrypt (uint8, uint8 *, uint8 *, uint16, uint8 *, uint8 *);
void SSP_CCM_Decrypt (uint8, uint8 *, uint8 *, uint16, uint8 *, uint8 *);
uint8 SSP_CCM_InvAuth (uint8, uint8 *, uint8 *, uint16, uint8 *,
                                                uint16, uint8 *, uint8 *);

/******************************************************************************
 * @fn      SSP_CCM_Auth
 *
 * @brief   Generates CCM Authentication tag U.
 *
 * input parameters
 *
 * @param   Mval    - Length of authentication field in octets [0,2,4,6,8,10,12,14 or 16]
 * @param   N       - Pointer to 13-byte Nonce
 * @param   M       - Pointer to octet string 'm'
 * @param   len_m   - Length of M[] in octets
 * @param   A       - Pointer to octet string 'a'
 * @param   len_a   - Length of A[] in octets
 * @param   AesKey  - Pointer to AES Key or Pointer to Key Expansion buffer.
 * @param   Cstate  - Pointer to output buffer
 *
 * output parameters
 *
 * @param   Cstate[]    - The first Mval bytes contain Authentication Tag T
 *
 * @return  None
 *
 */
void SSP_CCM_Auth (uint8 Mval, uint8 *N, uint8 *M, uint16 len_m, uint8 *A,
                                    uint16 len_a, uint8 *AesKey, uint8 *Cstate)
{
#if ((defined SOFTWARE_AES) && (SOFTWARE_AES == TRUE)) || ((defined SW_AES_AND_KEY_EXP) && (SW_AES_AND_KEY_EXP == TRUE))

  uint8   B[16], *bptr;
  uint8   i, remainder;
  uint16  blocks;

	// Check if authentication is even requested.  If not, exit.
	// This check is actually not needed because the rest of the
	// code works fine even with Mval==0.  I added it to reduce
	// unnecessary calculations and to speed up performance when
	// Mval==0
  if (!Mval) return;

  //
  // Construct B0
  //
  B[0] = 1;                               // L=2, L-encoding = L-1 = 1
  if (len_a)  B[0] |= 0x40;               // Adata bit
  if (Mval)  B[0] |= (Mval-2) << 2;		// M encoding

  osal_memcpy (B+1, N, 13);                   // append Nonce (13-bytes)

  B[14] = (uint8)(len_m >> 8);                // append l(m)
  B[15] = (uint8)(len_m);

  osal_memset (Cstate, 0, 16);                    // X0 = 0

  for (i=0; i < 16; i++)  Cstate[i] ^= B[i];
  pSspAesEncrypt (AesKey, Cstate);                 // Cstate[] = X1

  //
  // Construct B1
  //
  B[0] = (uint8) (len_a >> 8);
  B[1] = (uint8) (len_a);

  if (len_a > 14)  osal_memcpy (B+2, A, 14);
  else
  {
      osal_memset (B+2, 0, 14);
      osal_memcpy (B+2, A, len_a);
  }
  for (i=0; i < 16; i++)  Cstate[i] ^= B[i];
  pSspAesEncrypt (AesKey, Cstate);                 // Cstate[] = X2

  //
  // Construct B2..Ba, where Ba is the last block containing A[]
  //
  if (len_a > 14)
  {
    len_a -= 14;
    blocks = len_a >> 4;
    remainder = len_a & 0x0f;
    bptr = A+14;

    while (blocks--)
    {
      for (i=0; i < 16; i++)  Cstate[i] ^= *bptr++;
      pSspAesEncrypt (AesKey, Cstate);
    }

    if (remainder)
    {
      osal_memset (B, 0, 16);
      osal_memcpy (B, bptr, remainder);
      for (i=0; i < 16; i++)  Cstate[i] ^= B[i];
      pSspAesEncrypt (AesKey, Cstate);
    }
  }

  //
  // Construct Ba+1..Bm, where Bm is the last block containing M[]
  //
  blocks = len_m >> 4;
  remainder = len_m & 0x0f;
  bptr = M;

  while (blocks--)
  {
    for (i=0; i < 16; i++)  Cstate[i] ^= *bptr++;
    pSspAesEncrypt (AesKey, Cstate);
  }

  if (remainder)
  {
    osal_memset (B, 0, 16);
    osal_memcpy (B, bptr, remainder);
    for (i=0; i < 16; i++)  Cstate[i] ^= B[i];
    pSspAesEncrypt (AesKey, Cstate);
  }

#else /* HARDWARE_AES */

  uint8   B[STATE_BLENGTH], *bptr, *msg_in;
  uint8   i;
  uint16  blocks, msg_len;

  /* Check if authentication is even requested.  If not, exit. */
  if (!Mval) return;

  /* Construct B0 */
  B[0] = 1;                               /* L=2, L-encoding = L-1 = 1 */
  if (len_a)  B[0] |= 0x40;               /* Adata bit */
  if (Mval)  B[0] |= (Mval-2) << 2;       /* M encoding */

  osal_memcpy (B+1, N, 13);               /* append Nonce (13-bytes) */

  B[14] = (uint8)(len_m >> 8);            /* append l(m) */
  B[15] = (uint8)(len_m);

  osal_memset (Cstate, 0, STATE_BLENGTH); /* X0 = 0 */

  /* Calculate msg length and allocate OSAL buffer */
  msg_len = 48 + (len_a ? 2 : 0) + len_a - ((len_a ? len_a+2 : len_a) & 0x0f) + len_m - (len_m & 0x0f);
  bptr = msg_in = (uint8 *)osal_mem_alloc( msg_len );
  if (!bptr) return;

  /* Move B0 into position */
  osal_memcpy( bptr, B, STATE_BLENGTH );
  bptr += STATE_BLENGTH;

  /* Encode l(a) and move l(a) into position */
  if (len_a)
  {
    *bptr++ = (uint8) (len_a >> 8);
    *bptr++ = (uint8) (len_a);
  }
  osal_memcpy( bptr, A, len_a );
  bptr += len_a;

  /* Pad additional authentication data blocks with zeros if len_a > 0 */
  if ( len_a )
  {
    i = STATE_BLENGTH - ((len_a + 2) & 0x0f);
    osal_memset( bptr, 0, i );
    bptr += i;
  }

  /* Move M into position */
  osal_memcpy( bptr, M, len_m );
  bptr += len_m;

  /* Pad message with zeros to the nearest multiple of 16 */
  if ( len_m )
  {
    i = STATE_BLENGTH - (len_m & 0x0f);
    osal_memset( bptr, 0, i );
    bptr += i;
  }

  /* Prepare CBC-MAC */
  AES_SETMODE(CBC_MAC);
  AesLoadIV( Cstate );
  AesDmaSetup( Cstate, STATE_BLENGTH, msg_in, msg_len );
  AES_SET_ENCR_DECR_KEY_IV( AES_ENCRYPT );

  /* Calculate the block size and kick it off */
  blocks = (msg_len >> 4) - 1;
  while (blocks--)
  {
    /* CBC-MAC does not generate output until the last block */
    AES_START();
    while( !(ENCCS & 0x08) );
  }

  /* Switch to CBC mode for the last block and kick it off */
  AES_SETMODE(CBC);
  HAL_DMA_CLEAR_IRQ( HAL_DMA_AES_OUT );
  AES_START();
  while( !HAL_DMA_CHECK_IRQ( HAL_DMA_AES_OUT ) );

  osal_mem_free( msg_in );

#endif
}

/******************************************************************************
 * @fn      SSP_CCM_Encrypt
 *
 * @brief   Performs CCM encryption.
 *
 * input parameters
 *
 * @param   Mval    - Length of authentication field in octets [0,2,4,6,8,10,12,14 or 16]
 * @param   N       - Pointer to 13-byte Nonce
 * @param   M       - Pointer to octet string 'm'
 * @param   len_m   - Length of M[] in octets
 * @param   AesKey  - Pointer to AES Key or Pointer to Key Expansion buffer.
 * @param   Cstate  - Pointer to Authentication Tag U
 *
 * output parameters
 *
 * @param   M[]         - Encrypted octet string 'm'
 * @param   Cstate[]    - The first Mval bytes contain Encrypted Authentication Tag U
 *
 * @return  None
 *
 */
void SSP_CCM_Encrypt (uint8 Mval, uint8 *N, uint8 *M, uint16 len_m,
                                                  uint8 *AesKey, uint8 *Cstate)
{
#if ((defined SOFTWARE_AES) && (SOFTWARE_AES == TRUE)) || ((defined SW_AES_AND_KEY_EXP) && (SW_AES_AND_KEY_EXP == TRUE))

  uint8   A[16], T[16], *bptr;
  uint8   i, remainder;
  uint16  blocks, counter;

  osal_memcpy (T, Cstate, Mval);

  A[0] = 1;               // L=2, L-encoding = L-1 = 1
  osal_memcpy (A+1, N, 13);   // append Nonce
  counter = 1;
  bptr = M;

  blocks = len_m >> 4;
  remainder = len_m & 0x0f;

  while (blocks--)
  {
    osal_memcpy (Cstate, A, 14);
    Cstate[14] = (uint8) (counter >> 8);
    Cstate[15] = (uint8) (counter);
    pSspAesEncrypt (AesKey, Cstate);
    for (i=0; i < 16; i++) *bptr++ ^= Cstate[i];
    counter++;
  }

  if (remainder)
  {
    osal_memcpy (Cstate, A, 14);
    Cstate[14] = (uint8) (counter >> 8);
    Cstate[15] = (uint8) (counter);
    pSspAesEncrypt (AesKey, Cstate);
    for (i=0; i < remainder; i++) *bptr++ ^= Cstate[i];
  }

  osal_memcpy (Cstate, A, 14);
  Cstate[14] = Cstate[15] = 0;        // A0
  pSspAesEncrypt (AesKey, Cstate);     // Cstate = S0

  for (i=0; i < Mval; i++)  Cstate[i] ^= T[i];

#else /* HARDWARE_AES */

  uint8   A[STATE_BLENGTH], T[STATE_BLENGTH], *bptr;
  uint8   remainder;
  uint16  blocks;

  osal_memcpy (T, Cstate, Mval);

  A[0] = 1;                   /* L=2, L-encoding = L-1 = 1 */
  osal_memcpy (A+1, N, 13);   /* append Nonce */
  A[14] = A[15] = 0;          /* clear the CTR field */

  /* Claculate block sizes */
  blocks = len_m >> 4;
  remainder = len_m & 0x0f;
  if (remainder) blocks++;

  /* Allocate OSAL memory for message buffer */
  bptr = (uint8 *)osal_mem_alloc( blocks*STATE_BLENGTH );
  if (!bptr) return;

  /* Move message into position and pad with zeros */
  osal_memcpy( bptr, M, len_m );
  osal_memset( bptr+len_m, 0, STATE_BLENGTH-remainder );

  /* Set OFB mode and encrypt T to U */
  AES_SETMODE(OFB);
  AesLoadIV(A);
  AesDmaSetup( Cstate, STATE_BLENGTH, T, STATE_BLENGTH ); /* T -> U */
  AES_SET_ENCR_DECR_KEY_IV( AES_ENCRYPT );

  /* Kick it off */
  HAL_DMA_CLEAR_IRQ( HAL_DMA_AES_OUT );
  AES_START();
  while( !HAL_DMA_CHECK_IRQ( HAL_DMA_AES_OUT ) );

  /* Switch to CTR mode to encrypt message. CTR field must be greater than zero */
  AES_SETMODE(CTR);
  A[15] = 1;
  AesLoadIV(A);
  AesDmaSetup( bptr, blocks*STATE_BLENGTH, bptr, blocks*STATE_BLENGTH );
  AES_SET_ENCR_DECR_KEY_IV( AES_ENCRYPT );

  /* Kick it off */
  while (blocks--)
  {
    AES_START();
    while ( !(ENCCS & 0x08) );
  }

  /* Copy the encrypted message back to M and return OSAL memory */
  osal_memcpy( M, bptr, len_m );
  osal_mem_free( bptr );

#endif
}

/******************************************************************************
 * @fn      SSP_CCM_Decrypt
 *
 * @brief   Performs CCM decryption.
 *
 * input parameters
 *
 * @param   Mval    - Length of authentication field in octets [0,2,4,6,8,10,12,14 or 16]
 * @param   N       - Pointer to 13-byte Nonce
 * @param   C       - Pointer to octet string 'c', where 'c' = encrypted 'm' || encrypted auth tag U
 * @param   len_c   - Length of C[] in octets
 * @param   AesKey  - Pointer to AES Key or Pointer to Key Expansion buffer.
 * @param   Cstate  - Pointer AES state buffer (cannot be part of C[])
 *
 * output parameters
 *
 * @param   C[]         - Decrypted octet string 'm' || auth tag T
 * @param   Cstate[]    - The first Mval bytes contain  Authentication Tag T
 *
 * @return  None
 *
 */
void SSP_CCM_Decrypt (uint8 Mval, uint8 *N, uint8 *C, uint16 len_c,
                                                     uint8 *AesKey, uint8 *Cstate)
{
#if ((defined SOFTWARE_AES) && (SOFTWARE_AES == TRUE)) || ((defined SW_AES_AND_KEY_EXP) && (SW_AES_AND_KEY_EXP == TRUE))

  uint8   A[16], *bptr;
  uint8   i, remainder;
  uint16  blocks, counter;

  A[0] = 1;               // L=2, L-encoding = L-1 = 1
  osal_memcpy (A+1, N, 13);   // append Nonce
  counter = 1;
  bptr = C;

  i = len_c - Mval;
  blocks = i >> 4;
  remainder = i & 0x0f;

  while (blocks--)
  {
    osal_memcpy (Cstate, A, 14);
    Cstate[14] = (uint8) (counter >> 8);
    Cstate[15] = (uint8) (counter);
    pSspAesEncrypt (AesKey, Cstate);
    for (i=0; i < 16; i++) *bptr++ ^= Cstate[i];
    counter++;
  }

  if (remainder)
  {
    osal_memcpy (Cstate, A, 14);
    Cstate[14] = (uint8) (counter >> 8);
    Cstate[15] = (uint8) (counter);
    pSspAesEncrypt (AesKey, Cstate);
    for (i=0; i < remainder; i++) *bptr++ ^= Cstate[i];
  }

  osal_memcpy (Cstate, A, 14);
  Cstate[14] = Cstate[15] = 0;    // A0
  pSspAesEncrypt (AesKey, Cstate); // Cstate = S0

  counter = len_c - Mval;
  for (i=0; i < Mval; i++)
  {
    Cstate[i] ^= C[counter];    // save T in Cstate[]
    C[counter++] = Cstate[i];   // replace U with T (last Mval bytes of C[])
  }

#else /* HARDWARE_AES */

  uint8   *bptr;
  uint8   A[STATE_BLENGTH], U[STATE_BLENGTH];
  uint8   i;
  uint16  blocks;

  A[0] = 1;                  /* L=2, L-encoding = L-1 = 1 */
  osal_memcpy (A+1, N, 13);  /* append Nonce */
  A[14] = A[15] = 0;         /* clear the CTR field */

  /* Seperate M from C */
  i = len_c - Mval;
  blocks = i >> 4;
  if (i & 0x0f) blocks++;

  /* Extract U and pad it with zeros */
  osal_memset(U, 0, STATE_BLENGTH);
  osal_memcpy(U, C+i, Mval);

  /* Set OFB mode to encrypt U to T */
  AES_SETMODE(OFB);
  AesLoadIV(A);
  AesDmaSetup( Cstate, STATE_BLENGTH, U, STATE_BLENGTH ); /* U -> T */
  AES_SET_ENCR_DECR_KEY_IV( AES_ENCRYPT );

  /* Kick it off */
  HAL_DMA_CLEAR_IRQ( HAL_DMA_AES_OUT );
  AES_START();
  while ( !HAL_DMA_CHECK_IRQ( HAL_DMA_AES_OUT ) );

  /* Allocate OSAL memory for message buffer */
  bptr = (uint8 *)osal_mem_alloc( blocks*STATE_BLENGTH );
  if (!bptr) return;

  /* Move message into position and pad with zeros */
  osal_memset( bptr, 0, blocks*STATE_BLENGTH );
  osal_memcpy( bptr, C, i );

  /* Switch to CTR mode to decrypt message. CTR field must be greater than zero */
  AES_SETMODE(CTR);
  A[15] = 1;
  AesLoadIV(A);
  AesDmaSetup( bptr, blocks*STATE_BLENGTH, bptr, blocks*STATE_BLENGTH );
  AES_SET_ENCR_DECR_KEY_IV( AES_DECRYPT );

  /* Kick it off */
  while (blocks--)
  {
    AES_START();
    while ( !(ENCCS & 0x08) );
  }

  /* Copy the decrypted message back to M and return OSAL memory */
  osal_memcpy( C, bptr, i );
  osal_mem_free(bptr);

  /* Copy T to where U used to be */
  osal_memcpy(C+i, Cstate, Mval);

#endif
}

/******************************************************************************
 * @fn      SSP_CCM_InvAuth
 *
 * @brief   Verifies CCM authentication.
 *
 * input parameters
 *
 * @param   Mval    - Length of authentication field in octets [0,2,4,6,8,10,12,14 or 16]
 * @param   N       - Pointer to 13-byte Nonce
 * @param   C       - Pointer to octet string 'c' = 'm' || auth tag T
 * @param   len_c   - Length of C[] in octets
 * @param   A       - Pointer to octet string 'a'
 * @param   len_a   - Length of A[] in octets
 * @param   AesKey  - Pointer to AES Key or Pointer to Key Expansion buffer.
 * @param   Cstate  - Pointer to AES state buffer (cannot be part of C[])
 *
 * output parameters
 *
 * @param   Cstate[]    - The first Mval bytes contain computed Authentication Tag T
 *
 * @return  0 = Success, 1 = Failure
 *
 */
uint8 SSP_CCM_InvAuth (uint8 Mval, uint8 *N, uint8 *C, uint16 len_c, uint8 *A,
                                        uint16 len_a, uint8 *AesKey, uint8 *Cstate)
{
  uint8   i, t;
  uint8   status=0;

	// Check if authentication is even requested.  If not, return
	// success and exit.  This check is actually not needed because
	// the rest of the code works fine even with Mval==0.  I added
	// it to reduce unnecessary calculations and to speed up
	// performance when Mval==0
  if (!Mval) return 0;

  t = len_c - Mval;

  SSP_CCM_Auth (Mval, N, C, t, A, len_a, AesKey, Cstate);

  for (i=0; i < Mval; i++)
  {
    if (Cstate[i] != C[t++])
    {
      status = 1;
      break;
    }
  }
  return (status);
}
