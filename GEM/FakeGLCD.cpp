
#include "FakeGLCD.h"
#include "font.h"
#include "screen.h" // Using this for rendering

// Foreground color is normally 1, but can be set to 0xFF to indicate XOR
// drawing rather than color setting
//
static uint8_t fgColor = 1;
static uint16_t cursorX = 0;
static uint16_t cursorY = 0;

enum { kMaxSprites = 10 };
static const unsigned char* gSprites[kMaxSprites];

//------------------------------------------------------------------------------
GLCD::GLCD() : xdim(LCD_WIDTH), ydim(LCD_HEIGHT)
{
}

//------------------------------------------------------------------------------
void GLCD::clearScreen()
{
    // use screen_fill() and not screen_clear() because _clear() also does
    // a screen_update()
    screen_fill(0);
}

//------------------------------------------------------------------------------
void GLCD::presentScreen()
{
    screen_update();
}

//------------------------------------------------------------------------------
void GLCD::fontFace(uint8_t charset)
{
    switch (charset)
    {
        // Default font (font_system5x7), but maybe try: lucidaConsole_8ptBitmaps, font_arial
        case 0:
            screen_set_font(font_system5x7);
            break;

        // Title bar font (font_tomthumb3x5)
        case 1:
            screen_set_font(font_tomthumb3x5);
            break;
    }
}


//------------------------------------------------------------------------------
void GLCD::setXY(uint8_t posX, uint8_t posY)
{
    cursorX = posX;
    cursorY = posY;
}

//------------------------------------------------------------------------------
void GLCD::setX(uint8_t posX)
{
    cursorX = posX;
}

//------------------------------------------------------------------------------
void GLCD::setY(uint8_t posY)
{
    cursorY = posY;
}

//------------------------------------------------------------------------------
void GLCD::putstr(const char *s)
{
    screen_puts_xy(cursorX, cursorY, fgColor, s);
}

//------------------------------------------------------------------------------
void GLCD::drawSprite(uint8_t x, uint8_t y, uint8_t sprite_id, uint8_t mode)
{
    if (sprite_id >= kMaxSprites)
    {
        return;
    }
    
    const uint8_t *sprite = gSprites[sprite_id];
    if (!sprite)
    {
        return;
    }
    screen_put_sprite(x, y, sprite, mode);
}

//------------------------------------------------------------------------------
void GLCD::eraseBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    screen_fill_rect(x1, y1, x2-x1, y2-y1, 0);
}

//------------------------------------------------------------------------------
void GLCD::drawMode(uint8_t mode)
{
    switch (mode)
    {
        case GLCD_MODE_NORMAL:
            fgColor = 1;
            break;
 
        case GLCD_MODE_XOR:
            fgColor = 0xFF;
            break;

        default:
            return;
    }
}

//------------------------------------------------------------------------------
void GLCD::drawBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode)
{
    if (mode != GLCD_MODE_NORMAL)
    {
        return;
    }
    screen_draw_rect(x1, y1, x2-x1, y2-y1, fgColor);
}

//------------------------------------------------------------------------------
void GLCD::fillBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t pattern)
{
    if (pattern != 0)
    {
        return;
    }
    screen_fill_rect(x1, y1, x2-x1, y2-y1, 0);
}

//------------------------------------------------------------------------------
void GLCD::fillBox(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
    screen_fill_rect(x1, y1, x2-x1, y2-y1, fgColor);
}


//------------------------------------------------------------------------------
void GLCD::drawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t mode)
{
    if (mode != GLCD_MODE_NORMAL)
    {
        return;
    }
}

//------------------------------------------------------------------------------
void GLCD::loadSprite_P(uint8_t id, const uint8_t* sprite)
{
    if (id < kMaxSprites)
    {
        gSprites[id] = sprite;
    }
}

