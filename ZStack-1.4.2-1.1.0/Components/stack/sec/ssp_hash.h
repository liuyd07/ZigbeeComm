#ifndef SSP_HASH_H
#define SSP_HASH_H

/*********************************************************************
    Filename:       ssp_hash.h
    Revised:        $Date: 2006-04-06 08:19:08 -0700 (Thu, 06 Apr 2006) $
    Revision:       $Revision: 10396 $

    Description:

        Provides interface for the keyed hash function.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif


/*********************************************************************
 * INCLUDES
 */

#include  "ssp.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * FUNCTIONS
 */

//void SSP_KeyedHash (uint8 *, uint16, uint8 *, uint8 *);
void sspMMOHash (uint8 *, uint8, uint8 *, uint16, uint8 *);
void SSP_KeyedHash (uint8 *M, uint16 bitlen, uint8 *AesKey, uint8 *Cstate);

/*********************************************************************
*********************************************************************/
#ifdef __cplusplus
}
#endif

#endif /* SSP_HASH_H */

