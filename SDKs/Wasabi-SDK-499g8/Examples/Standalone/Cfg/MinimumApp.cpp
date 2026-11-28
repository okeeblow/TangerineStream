/*

  Nullsoft WASABI Source File License

  Copyright 1999-2003 Nullsoft, Inc.

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

*/
// Minimum.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "wasabi/api/apiinit.h"

// {BE5ABCF2-9EC7-49df-8881-C752BC844A92}
static const GUID MINIAPP_GUID = 
{ 0xbe5abcf2, 0x9ec7, 0x49df, { 0x88, 0x81, 0xc7, 0x52, 0xbc, 0x84, 0x4a, 0x92 } };


int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
 	// TODO: Place code here.

  ApiInit::init(hInstance, MINIAPP_GUID, lpCmdLine);


  ApiInit::shutdown();

	return 0;
}



