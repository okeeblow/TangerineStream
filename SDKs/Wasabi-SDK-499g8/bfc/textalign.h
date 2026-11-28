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
/*

Darkain Made This.  :)
// and BU tweaked it

*/

/*
Darkain:  i wanted one base for ALL text alignment in ALL classes.
*/


#ifndef _TEXTALIGN_H
#define _TEXTALIGN_H

typedef enum {
  TEXTALIGN_LEFT,
  TEXTALIGN_CENTER, //what ever default center style is... see bellow
  TEXTALIGN_RIGHT,
  TEXTALIGN_EVENSPACING,  //add more space between letters/words to make it fit in 100% of the area
  TEXTALIGN_FITTOWIDTH,  //make the font larger or smaller to fit in 100% of the area
  TEXTALIGN_LEFT_ELLIPSIS, //align left, and truncate text to fit if too large
  TEXTALIGN_CENTER_CENTER,  //if text is too wide, it will still center on the middle, choping off left and right sides
  TEXTALIGN_CENTER_LEFT,	//will chop off right side if too big
  TEXTALIGN_CENTER_RIGHT,	//will chop off left side if too big
  TEXTALIGN_SCROLL,  //if text is too large, it will use default scrolling (see bellow)
  TEXTALIGN_SCROLL_BACKFORTH,  //text will scroll back and forth if too large
  TEXTALIGN_SCROLL_TICKER,  //text will scroll in one direction, and loop
} TextAlign;


#endif // TEXT_ALIGN_H
