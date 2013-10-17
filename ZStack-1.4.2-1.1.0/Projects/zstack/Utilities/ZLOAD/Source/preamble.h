#ifndef PREAMBLE_H
#define PREAMBLE_H

/*********************************************************************
    Filename:       preamble.h
    Revised:        $Date: 2006-04-06 08:19:08 -0700 (Thu, 06 Apr 2006) $
    Revision:       $Revision: 10396 $

    Description:

        This file provides the Customer access point for specification
        of image identification including Version, Manufacturer ID and
        Product ID, each 2 bytes. There is an entry for each of End
        Device, Router, and Coordinator. This file is included by the
        source file ZLOAD_App.c and results in the identifying infomation
        being placed at a known offset in the binary image that results
        from the build.

    Notes:

    Copyright (c) 2006 by Texas Instruments, Inc.
    All Rights Reserved.  Permission to use, reproduce, copy, prepare
    derivative works, modify, distribute, perform, display or sell this
    software and/or its documentation for any purpose is prohibited
    without the express written consent of Texas Instruments, Inc.
*********************************************************************/

#if defined(ZDO_COORDINATOR)
  // COORDINATOR
  #define IMAGE_VERSION    ((uint16) 0x3043)
  #define MANUFACTURER_ID  ((uint16) 0xF8F8)
  #define PRODUCT_ID       ((uint16) 0x00AD)

#elif defined(RTR_NWK)
  //ROUTER
  #define IMAGE_VERSION    ((uint16) 0x3252)
  #define MANUFACTURER_ID  ((uint16) 0xF8F8)
  #define PRODUCT_ID       ((uint16) 0x00AD)

#else
  //END DEVICE
  #define IMAGE_VERSION    ((uint16) 0x3045)
  #define MANUFACTURER_ID  ((uint16) 0xF8F8)
  #define PRODUCT_ID       ((uint16) 0x00AD)
#endif


#endif