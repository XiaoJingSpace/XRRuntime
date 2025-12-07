//=============================================================================
// FILE: svrApiVersion.cpp
//                  Copyright (c) 2016 QUALCOMM Technologies Inc.
//                              All Rights Reserved.
//
//==============================================================================

#include <stdlib.h>
#include <stdio.h>

#include "svrApiCore.h"

//-----------------------------------------------------------------------------
const char* sxrGetVersion()
//-----------------------------------------------------------------------------
{
    static char versionBuffer[256];
    snprintf(versionBuffer, 256, "%d.%d.%d, %s - %s", SXR_MAJOR_VERSION, SXR_MINOR_VERSION, SXR_REVISION_VERSION, __DATE__, __TIME__);
    return &versionBuffer[0];
}