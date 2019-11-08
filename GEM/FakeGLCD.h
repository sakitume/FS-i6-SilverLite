/* -!- C++ -!- ********************************************* Alternative
 * Graphic Serial LCD Libary Header File
 * 
 * Parts of this library are based on the original Sparkfun library by:
 *
 * Joel Bartlett 
 * SparkFun Electronics
 * 9-25-13
 * 
 * Jon Green - 205-04-01 
 * New interface and in-line definitions added 2015-04-01 and library
 * re-formatted
 * 
 * 
 * Copyright (c) 2015 Jon Green
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 ***************************************************************************/

#ifndef _GLCD_H_
#define _GLCD_H_

#include "FakeArduino.h"


// Normal
#define GLCD_MODE_NORMAL           ((uint8_t)(0x01))
// XOR mode (screen ^ pixel)
#define GLCD_MODE_XOR              ((uint8_t)(0x04))

#define GLCD_ID_CRLF               0x06 /* Line ending CR+LF */
#define GLCD_ID_SCROLL             0x09 /* Scroll on/off */


/// A lot of the methods are other calls with no processing so they are
/// mapped immediataly rather than nesting function calls.
class GLCD
{
public:
    //////////////////////////////////////////////////////////////////////////
    // The x screen dimension (width). This is only valid after a reset().
    uint8_t xdim;

    //////////////////////////////////////////////////////////////////////////
    // The y screen dimension (height). This is only valid after a reset().
    uint8_t ydim;

    //////////////////////////////////////////////////////////////////////////
    /// Constructor.
    ///
    /// @param [in] software_serial The handle of the software serial object
    GLCD();

    //////////////////////////////////////////////////////////////////////////
    /// Clear the screen
    ///
    void clearScreen();

    //////////////////////////////////////////////////////////////////////////
    /// Changes the font face temporarily.
    ///
    /// @param [in] charset The character set to use.
    ///
    void fontFace (uint8_t charset);

    /////////////////////////////////////////////////////////////////////////
    /// Modify the x and y position for character rendering. The character is
    /// rendered with the top left corner of the character drawn at the
    /// specified (x,y) coordinate.
    ///
    /// @param [in] posX The new x-coordinate position.
    /// @param [in] posY The new y-coordinate position.
    ///
    void setXY(uint8_t posX, uint8_t posY);

    /////////////////////////////////////////////////////////////////////////
    /// Modify the x and y position for character rendering. The character is
    /// rendered with the top left corner of the character drawn at the
    /// specified (x,y) coordinate.
    ///
    /// @param [in] posX The new x-coordinate position.
    ///
    void setX(uint8_t posX);

    /////////////////////////////////////////////////////////////////////////
    /// Modify the x and y position for character rendering. The character is
    /// rendered with the top left corner of the character drawn at the
    /// specified (x,y) coordinate.
    ///
    /// @param [in] posY The new y-coordinate position.
    ///
    void setY(uint8_t posY);

    /////////////////////////////////////////////////////////////////////////
    /// Put a nil terminated string to the screen. The call manages XON and
    /// XOFF to ensure that the command does not overflow.
    ///
    /// @param [in] s The string to draw from memory.
    ///
    void putstr(const char *s);

    //////////////////////////////////////////////////////////////////////////
    /// Put a character to the screen. This is the fundemental call for the
    /// whole of the library and manages XON/OXFF to ensure that the screen
    /// buffer does not overflow. Noted I wanted to call this putc but there
    /// is a macro that prevents this.
    ///
    /// @param cc [in] The charcter to put to the sceeen
    void put (uint8_t cc);

    //////////////////////////////////////////////////////////////////////////
    /// Draw an image in the screen from memory.
    ///
    /// @param [in] x The top left x-coordinate.
    /// @param [in] y The top left y-coordinate.
    /// @param [in] sprite_id The identity of the sprite to draw.
    /// @param [in] mode The drawing mode of the line.
    ///
    void drawSprite(uint8_t x, uint8_t y, uint8_t sprite_id, uint8_t mode);

    //////////////////////////////////////////////////////////////////////////
    /// Erase a rectangular block of the screen to the background colour.
    ///
    /// @param [in] x1 The top left x-coordinate.
    /// @param [in] y1 The top left y-coordinate.
    /// @param [in] x2 The bottom right x-coordinate.
    /// @param [in] y2 The bottom right y-coordinate.
    ///
    void eraseBox (uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

    //////////////////////////////////////////////////////////////////////////
    /// Change the current draw mode
    ///
    /// @param [in] mode The new drawing mode.
    ///
    void drawMode (uint8_t mode);

    //////////////////////////////////////////////////////////////////////////
    /// Draws a box. The box is described by a diagonal line from x, y1 to x2,
    /// y2.
    ///
    /// @param [in] x1 The upper left x-coordinate.
    /// @param [in] y1 The upper left y-coordingate.
    /// @param [in] x2 The lower right x-coordinate.
    /// @param [in] y2 The lower right y-coordinate
    ///
    void drawBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

    //////////////////////////////////////////////////////////////////////////
    /// Draws a box. The box is described by a diagonal line from x, y1 to x2,
    /// y2.
    ///
    /// @param [in] x1 The upper left x-coordinate.
    /// @param [in] y1 The upper left y-coordingate.
    /// @param [in] x2 The lower right x-coordinate.
    /// @param [in] y2 The lower right y-coordinate
    /// @param [in] mode The drawing mode of the line.
    ///
    void drawBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode);

    //////////////////////////////////////////////////////////////////////////
    /// Draws a box and fills it with a pattern. The box is described by a
    /// diagonal line from x, y1 to x2, y2
    ///
    /// @param [in] x1 The upper left x-coordinate.
    /// @param [in] y1 The upper left y-coordingate.
    /// @param [in] x2 The lower right x-coordinate.
    /// @param [in] y2 The lower right y-coordinate
    /// @param [in] pattern The vertical pattern to use as fill pattern.
    ///
    void fillBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t pattern);

    //////////////////////////////////////////////////////////////////////////
    /// Draws a box and fills it in the current draw mode. The box is
    /// described by a diagonal line from x, y1 to x2, y2
    ///
    /// @param [in] x1 The upper left x-coordinate.
    /// @param [in] y1 The upper left y-coordingate.
    /// @param [in] x2 The lower right x-coordinate.
    /// @param [in] y2 The lower right y-coordinate
    ///
    void fillBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);

    //////////////////////////////////////////////////////////////////////////
    /// Draws a line.
    ///
    /// @param [in] x1 The start x-coordinate.
    /// @param [in] y1 The start y-coordingate.
    /// @param [in] x2 The end x-coordinate.
    /// @param [in] y2 The end y-coordinate
    /// @param [in] mode The drawing mode of the line.
    ///
    void drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode);
};

#endif  /* _GLCD_H_ */
