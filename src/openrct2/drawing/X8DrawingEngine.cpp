/*****************************************************************************
 * Copyright (c) 2014-2019 OpenRCT2 developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/OpenRCT2/OpenRCT2
 *
 * OpenRCT2 is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "X8DrawingEngine.h"

#include "../Context.h"
#include "../Game.h"
#include "../Intro.h"
#include "../config/Config.h"
#include "../interface/Screenshot.h"
#include "../interface/Viewport.h"
#include "../interface/Window.h"
#include "../ui/UiContext.h"
#include "Drawing.h"
#include "IDrawingContext.h"
#include "IDrawingEngine.h"
#include "LightFX.h"
#include "Rain.h"

#include <algorithm>
#include <cstring>

using namespace OpenRCT2;
using namespace OpenRCT2::Drawing;
using namespace OpenRCT2::Ui;

X8RainDrawer::X8RainDrawer()
{
    _rainPixels = new RainPixel[_rainPixelsCapacity];
}

X8RainDrawer::~X8RainDrawer()
{
    delete[] _rainPixels;
}

void X8RainDrawer::SetDPI(rct_drawpixelinfo* dpi)
{
    _screenDPI = dpi;
}

void X8RainDrawer::Draw(int32_t x, int32_t y, int32_t width, int32_t height, int32_t xStart, int32_t yStart)
{
    const uint8_t* pattern = RainPattern;
    uint8_t patternXSpace = *pattern++;
    uint8_t patternYSpace = *pattern++;

    uint8_t patternStartXOffset = xStart % patternXSpace;
    uint8_t patternStartYOffset = yStart % patternYSpace;

    uint32_t pixelOffset = (_screenDPI->pitch + _screenDPI->width) * y + x;
    uint8_t patternYPos = patternStartYOffset % patternYSpace;

    uint8_t* screenBits = _screenDPI->bits;

    // Stores the colours of changed pixels
    RainPixel* newPixels = &_rainPixels[_rainPixelsCount];
    for (; height != 0; height--)
    {
        uint8_t patternX = pattern[patternYPos * 2];
        if (patternX != 0xFF)
        {
            if (_rainPixelsCount < (_rainPixelsCapacity - (uint32_t)width))
            {
                uint32_t finalPixelOffset = width + pixelOffset;

                uint32_t xPixelOffset = pixelOffset;
                xPixelOffset += ((uint8_t)(patternX - patternStartXOffset)) % patternXSpace;

                uint8_t patternPixel = pattern[patternYPos * 2 + 1];
                for (; xPixelOffset < finalPixelOffset; xPixelOffset += patternXSpace)
                {
                    uint8_t current_pixel = screenBits[xPixelOffset];
                    screenBits[xPixelOffset] = patternPixel;
                    _rainPixelsCount++;

                    // Store colour and position
                    *newPixels++ = { xPixelOffset, current_pixel };
                }
            }
        }

        pixelOffset += _screenDPI->pitch + _screenDPI->width;
        patternYPos++;
        patternYPos %= patternYSpace;
    }
}

void X8RainDrawer::Restore()
{
    if (_rainPixelsCount > 0)
    {
        uint32_t numPixels = (_screenDPI->width + _screenDPI->pitch) * _screenDPI->height;
        uint8_t* bits = _screenDPI->bits;
        for (uint32_t i = 0; i < _rainPixelsCount; i++)
        {
            RainPixel rainPixel = _rainPixels[i];
            if (rainPixel.Position >= numPixels)
            {
                // Pixel out of bounds, bail
                break;
            }

            bits[rainPixel.Position] = rainPixel.Colour;
        }
        _rainPixelsCount = 0;
    }
}

#ifdef __WARN_SUGGEST_FINAL_METHODS__
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wsuggest-final-methods"
#endif

X8DrawingEngine::X8DrawingEngine([[maybe_unused]] const std::shared_ptr<Ui::IUiContext>& uiContext)
{
    _drawingContext = new X8DrawingContext(this);
    _bitsDPI.DrawingEngine = this;
#ifdef __ENABLE_LIGHTFX__
    lightfx_set_available(true);
    _lastLightFXenabled = (gConfigGeneral.enable_light_fx != 0);
#endif
}

X8DrawingEngine::~X8DrawingEngine()
{
    delete _drawingContext;
    delete[] _dirtyGrid.Blocks;
    delete[] _bits;
}

void X8DrawingEngine::Initialise()
{
}

void X8DrawingEngine::Resize(uint32_t width, uint32_t height)
{
    uint32_t pitch = width;
    ConfigureBits(width, height, pitch);
}

void X8DrawingEngine::SetPalette([[maybe_unused]] const rct_palette_entry* palette)
{
}

void X8DrawingEngine::SetVSync([[maybe_unused]] bool vsync)
{
    // Not applicable for this engine
}

void X8DrawingEngine::Invalidate(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    left = std::max(left, 0);
    top = std::max(top, 0);
    right = std::min(right, (int32_t)_width);
    bottom = std::min(bottom, (int32_t)_height);

    if (left >= right)
        return;
    if (top >= bottom)
        return;

    right--;
    bottom--;

    left >>= _dirtyGrid.BlockShiftX;
    right >>= _dirtyGrid.BlockShiftX;
    top >>= _dirtyGrid.BlockShiftY;
    bottom >>= _dirtyGrid.BlockShiftY;

    uint32_t dirtyBlockColumns = _dirtyGrid.BlockColumns;
    uint8_t* screenDirtyBlocks = _dirtyGrid.Blocks;
    for (int16_t y = top; y <= bottom; y++)
    {
        uint32_t yOffset = y * dirtyBlockColumns;
        for (int16_t x = left; x <= right; x++)
        {
            screenDirtyBlocks[yOffset + x] = 0xFF;
        }
    }
}

void X8DrawingEngine::BeginDraw()
{
    if (gIntroState == INTRO_STATE_NONE)
    {
#ifdef __ENABLE_LIGHTFX__
        // HACK we need to re-configure the bits if light fx has been enabled / disabled
        if (_lastLightFXenabled != (gConfigGeneral.enable_light_fx != 0))
        {
            Resize(_width, _height);
        }
#endif
        _rainDrawer.SetDPI(&_bitsDPI);
        _rainDrawer.Restore();
    }
}

void X8DrawingEngine::EndDraw()
{
}

void X8DrawingEngine::PaintWindows()
{
    window_reset_visibilities();

    // Redraw dirty regions before updating the viewports, otherwise
    // when viewports get panned, they copy dirty pixels
    DrawAllDirtyBlocks();
    window_update_all_viewports();
    DrawAllDirtyBlocks();
}

void X8DrawingEngine::UpdateWindows()
{
    window_update_all();
}

void X8DrawingEngine::PaintRain()
{
    DrawRain(&_bitsDPI, &_rainDrawer);
}

void X8DrawingEngine::CopyRect(int32_t x, int32_t y, int32_t width, int32_t height, int32_t dx, int32_t dy)
{
    if (dx == 0 && dy == 0)
        return;

    // Originally 0x00683359
    // Adjust for move off screen
    // NOTE: when zooming, there can be x, y, dx, dy combinations that go off the
    // screen; hence the checks. This code should ultimately not be called when
    // zooming because this function is specific to updating the screen on move
    int32_t lmargin = std::min(x - dx, 0);
    int32_t rmargin = std::min((int32_t)_width - (x - dx + width), 0);
    int32_t tmargin = std::min(y - dy, 0);
    int32_t bmargin = std::min((int32_t)_height - (y - dy + height), 0);
    x -= lmargin;
    y -= tmargin;
    width += lmargin + rmargin;
    height += tmargin + bmargin;

    int32_t stride = _bitsDPI.width + _bitsDPI.pitch;
    uint8_t* to = _bitsDPI.bits + y * stride + x;
    uint8_t* from = _bitsDPI.bits + (y - dy) * stride + x - dx;

    if (dy > 0)
    {
        // If positive dy, reverse directions
        to += (height - 1) * stride;
        from += (height - 1) * stride;
        stride = -stride;
    }

    // Move bytes
    for (int32_t i = 0; i < height; i++)
    {
        memmove(to, from, width);
        to += stride;
        from += stride;
    }
}

std::string X8DrawingEngine::Screenshot()
{
    return screenshot_dump_png(&_bitsDPI);
}

IDrawingContext* X8DrawingEngine::GetDrawingContext(rct_drawpixelinfo* dpi)
{
    _drawingContext->SetDPI(dpi);
    return _drawingContext;
}

rct_drawpixelinfo* X8DrawingEngine::GetDrawingPixelInfo()
{
    return &_bitsDPI;
}

DRAWING_ENGINE_FLAGS X8DrawingEngine::GetFlags()
{
    return DEF_DIRTY_OPTIMISATIONS;
}

void X8DrawingEngine::InvalidateImage([[maybe_unused]] uint32_t image)
{
    // Not applicable for this engine
}

rct_drawpixelinfo* X8DrawingEngine::GetDPI()
{
    return &_bitsDPI;
}

void X8DrawingEngine::ConfigureBits(uint32_t width, uint32_t height, uint32_t pitch)
{
    size_t newBitsSize = pitch * height;
    uint8_t* newBits = new uint8_t[newBitsSize];
    if (_bits == nullptr)
    {
        std::fill_n(newBits, newBitsSize, 0);
    }
    else
    {
        if (_pitch == pitch)
        {
            std::copy_n(_bits, std::min(_bitsSize, newBitsSize), newBits);
        }
        else
        {
            uint8_t* src = _bits;
            uint8_t* dst = newBits;

            uint32_t minWidth = std::min(_width, width);
            uint32_t minHeight = std::min(_height, height);
            for (uint32_t y = 0; y < minHeight; y++)
            {
                std::copy_n(src, minWidth, dst);
                if (pitch - minWidth > 0)
                {
                    std::fill_n(dst + minWidth, pitch - minWidth, 0);
                }
                src += _pitch;
                dst += pitch;
            }
        }
        delete[] _bits;
    }

    _bits = newBits;
    _bitsSize = newBitsSize;
    _width = width;
    _height = height;
    _pitch = pitch;

    rct_drawpixelinfo* dpi = &_bitsDPI;
    dpi->bits = _bits;
    dpi->x = 0;
    dpi->y = 0;
    dpi->width = width;
    dpi->height = height;
    dpi->pitch = _pitch - width;

    ConfigureDirtyGrid();

#ifdef __ENABLE_LIGHTFX__
    if (lightfx_is_available())
    {
        lightfx_update_buffers(dpi);
    }
#endif
}

void X8DrawingEngine::OnDrawDirtyBlock(
    [[maybe_unused]] uint32_t x, [[maybe_unused]] uint32_t y, [[maybe_unused]] uint32_t columns, [[maybe_unused]] uint32_t rows)
{
}

void X8DrawingEngine::ConfigureDirtyGrid()
{
    _dirtyGrid.BlockShiftX = 7;
    _dirtyGrid.BlockShiftY = 6;
    _dirtyGrid.BlockWidth = 1 << _dirtyGrid.BlockShiftX;
    _dirtyGrid.BlockHeight = 1 << _dirtyGrid.BlockShiftY;
    _dirtyGrid.BlockColumns = (_width >> _dirtyGrid.BlockShiftX) + 1;
    _dirtyGrid.BlockRows = (_height >> _dirtyGrid.BlockShiftY) + 1;

    delete[] _dirtyGrid.Blocks;
    _dirtyGrid.Blocks = new uint8_t[_dirtyGrid.BlockColumns * _dirtyGrid.BlockRows];
}

void X8DrawingEngine::DrawAllDirtyBlocks()
{
    uint32_t dirtyBlockColumns = _dirtyGrid.BlockColumns;
    uint32_t dirtyBlockRows = _dirtyGrid.BlockRows;
    uint8_t* dirtyBlocks = _dirtyGrid.Blocks;

    for (uint32_t x = 0; x < dirtyBlockColumns; x++)
    {
        for (uint32_t y = 0; y < dirtyBlockRows; y++)
        {
            uint32_t yOffset = y * dirtyBlockColumns;
            if (dirtyBlocks[yOffset + x] == 0)
            {
                continue;
            }

            // Determine columns
            uint32_t xx;
            for (xx = x; xx < dirtyBlockColumns; xx++)
            {
                if (dirtyBlocks[yOffset + xx] == 0)
                {
                    break;
                }
            }
            uint32_t columns = xx - x;

            // Check rows
            uint32_t yy;
            for (yy = y; yy < dirtyBlockRows; yy++)
            {
                uint32_t yyOffset = yy * dirtyBlockColumns;
                for (xx = x; xx < x + columns; xx++)
                {
                    if (dirtyBlocks[yyOffset + xx] == 0)
                    {
                        goto endRowCheck;
                    }
                }
            }

        endRowCheck:
            uint32_t rows = yy - y;
            DrawDirtyBlocks(x, y, columns, rows);
        }
    }
}

void X8DrawingEngine::DrawDirtyBlocks(uint32_t x, uint32_t y, uint32_t columns, uint32_t rows)
{
    uint32_t dirtyBlockColumns = _dirtyGrid.BlockColumns;
    uint8_t* screenDirtyBlocks = _dirtyGrid.Blocks;

    // Unset dirty blocks
    for (uint32_t top = y; top < y + rows; top++)
    {
        uint32_t topOffset = top * dirtyBlockColumns;
        for (uint32_t left = x; left < x + columns; left++)
        {
            screenDirtyBlocks[topOffset + left] = 0;
        }
    }

    // Determine region in pixels
    uint32_t left = std::max<uint32_t>(0, x * _dirtyGrid.BlockWidth);
    uint32_t top = std::max<uint32_t>(0, y * _dirtyGrid.BlockHeight);
    uint32_t right = std::min(_width, left + (columns * _dirtyGrid.BlockWidth));
    uint32_t bottom = std::min(_height, top + (rows * _dirtyGrid.BlockHeight));
    if (right <= left || bottom <= top)
    {
        return;
    }

    // Draw region
    OnDrawDirtyBlock(x, y, columns, rows);
    window_draw_all(&_bitsDPI, left, top, right, bottom);
}

#ifdef __WARN_SUGGEST_FINAL_METHODS__
#    pragma GCC diagnostic pop
#endif

X8DrawingContext::X8DrawingContext(X8DrawingEngine* engine)
{
    _engine = engine;
}

IDrawingEngine* X8DrawingContext::GetEngine()
{
    return _engine;
}

void X8DrawingContext::Clear(uint8_t paletteIndex)
{
    rct_drawpixelinfo* dpi = _dpi;

    int32_t w = dpi->width / dpi->zoom_level;
    int32_t h = dpi->height / dpi->zoom_level;
    uint8_t* ptr = dpi->bits;

    for (int32_t y = 0; y < h; y++)
    {
        std::fill_n(ptr, w, paletteIndex);
        ptr += w + dpi->pitch;
    }
}

/** rct2: 0x0097FF04 */
// clang-format off
static constexpr const uint16_t Pattern[] = {
    0b0111111110000000,
    0b0011111111000000,
    0b0001111111100000,
    0b0000111111110000,
    0b0000011111111000,
    0b0000001111111100,
    0b0000000111111110,
    0b0000000011111111,
    0b1000000001111111,
    0b1100000000111111,
    0b1110000000011111,
    0b1111000000001111,
    0b1111100000000111,
    0b1111110000000011,
    0b1111111000000001,
    0b1111111100000000,
};

/** rct2: 0x0097FF14 */
static constexpr const uint16_t PatternInverse[] = {
    0b1000000001111111,
    0b1100000000111111,
    0b1110000000011111,
    0b1111000000001111,
    0b1111100000000111,
    0b1111110000000011,
    0b1111111000000001,
    0b1111111100000000,
    0b0111111110000000,
    0b0011111111000000,
    0b0001111111100000,
    0b0000111111110000,
    0b0000011111111000,
    0b0000001111111100,
    0b0000000111111110,
    0b0000000011111111
};

/** rct2: 0x0097FEFC */
static constexpr const uint16_t * Patterns[] = {
    Pattern,
    PatternInverse
};
// clang-format on

void X8DrawingContext::FillRect(uint32_t colour, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    rct_drawpixelinfo* dpi = _dpi;

    if (left > right)
        return;
    if (top > bottom)
        return;
    if (dpi->x > right)
        return;
    if (left >= dpi->x + dpi->width)
        return;
    if (bottom < dpi->y)
        return;
    if (top >= dpi->y + dpi->height)
        return;

    uint16_t crossPattern = 0;

    int32_t startX = left - dpi->x;
    if (startX < 0)
    {
        crossPattern ^= startX;
        startX = 0;
    }

    int32_t endX = right - dpi->x + 1;
    if (endX > dpi->width)
    {
        endX = dpi->width;
    }

    int32_t startY = top - dpi->y;
    if (startY < 0)
    {
        crossPattern ^= startY;
        startY = 0;
    }

    int32_t endY = bottom - dpi->y + 1;
    if (endY > dpi->height)
    {
        endY = dpi->height;
    }

    int32_t width = endX - startX;
    int32_t height = endY - startY;

    if (colour & 0x1000000)
    {
        // Cross hatching
        uint8_t* dst = (startY * (dpi->width + dpi->pitch)) + startX + dpi->bits;
        for (int32_t i = 0; i < height; i++)
        {
            uint8_t* nextdst = dst + dpi->width + dpi->pitch;
            uint32_t p = ror32(crossPattern, 1);
            p = (p & 0xFFFF0000) | width;

            // Fill every other pixel with the colour
            for (; (p & 0xFFFF) != 0; p--)
            {
                p = p ^ 0x80000000;
                if (p & 0x80000000)
                {
                    *dst = colour & 0xFF;
                }
                dst++;
            }
            crossPattern ^= 1;
            dst = nextdst;
        }
    }
    else if (colour & 0x2000000)
    {
        assert(false);
    }
    else if (colour & 0x4000000)
    {
        uint8_t* dst = startY * (dpi->width + dpi->pitch) + startX + dpi->bits;

        // The pattern loops every 15 lines this is which
        // part the pattern is on.
        int32_t patternY = (startY + dpi->y) % 16;

        // The pattern loops every 15 pixels this is which
        // part the pattern is on.
        int32_t startPatternX = (startX + dpi->x) % 16;
        int32_t patternX = startPatternX;

        const uint16_t* patternsrc = Patterns[colour >> 28]; // or possibly uint8_t)[esi*4] ?

        for (int32_t numLines = height; numLines > 0; numLines--)
        {
            uint8_t* nextdst = dst + dpi->width + dpi->pitch;
            uint16_t pattern = patternsrc[patternY];

            for (int32_t numPixels = width; numPixels > 0; numPixels--)
            {
                if (pattern & (1 << patternX))
                {
                    *dst = colour & 0xFF;
                }
                patternX = (patternX + 1) % 16;
                dst++;
            }
            patternX = startPatternX;
            patternY = (patternY + 1) % 16;
            dst = nextdst;
        }
    }
    else
    {
        uint8_t* dst = startY * (dpi->width + dpi->pitch) + startX + dpi->bits;
        for (int32_t i = 0; i < height; i++)
        {
            std::fill_n(dst, width, colour & 0xFF);
            dst += dpi->width + dpi->pitch;
        }
    }
}

void X8DrawingContext::FilterRect(FILTER_PALETTE_ID palette, int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    rct_drawpixelinfo* dpi = _dpi;

    if (left > right)
        return;
    if (top > bottom)
        return;
    if (dpi->x > right)
        return;
    if (left >= dpi->x + dpi->width)
        return;
    if (bottom < dpi->y)
        return;
    if (top >= dpi->y + dpi->height)
        return;

    int32_t startX = left - dpi->x;
    if (startX < 0)
    {
        startX = 0;
    }

    int32_t endX = right - dpi->x + 1;
    if (endX > dpi->width)
    {
        endX = dpi->width;
    }

    int32_t startY = top - dpi->y;
    if (startY < 0)
    {
        startY = 0;
    }

    int32_t endY = bottom - dpi->y + 1;
    if (endY > dpi->height)
    {
        endY = dpi->height;
    }

    int32_t width = endX - startX;
    int32_t height = endY - startY;

    // 0x2000000
    // 00678B7E   00678C83
    // Location in screen buffer?
    uint8_t* dst = dpi->bits
        + (uint32_t)((startY / dpi->zoom_level) * ((dpi->width / dpi->zoom_level) + dpi->pitch) + (startX / dpi->zoom_level));

    // Find colour in colour table?
    uint16_t g1Index = palette_to_g1_offset[palette];
    auto g1Element = gfx_get_g1_element(g1Index);
    if (g1Element != nullptr)
    {
        auto g1Bits = g1Element->offset;
        const int32_t scaled_width = width / dpi->zoom_level;
        const int32_t step = ((dpi->width / dpi->zoom_level) + dpi->pitch);

        // Fill the rectangle with the colours from the colour table
        auto c = height / dpi->zoom_level;
        for (int32_t i = 0; i < c; i++)
        {
            uint8_t* nextdst = dst + step * i;
            for (int32_t j = 0; j < scaled_width; j++)
            {
                *(nextdst + j) = g1Bits[*(nextdst + j)];
            }
        }
    }
}

void X8DrawingContext::DrawLine(uint32_t colour, int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    gfx_draw_line_software(_dpi, x1, y1, x2, y2, colour);
}

void X8DrawingContext::DrawSprite(uint32_t image, int32_t x, int32_t y, uint32_t tertiaryColour)
{
    gfx_draw_sprite_software(_dpi, ImageId::FromUInt32(image, tertiaryColour), x, y);
}

void X8DrawingContext::DrawSpriteRawMasked(int32_t x, int32_t y, uint32_t maskImage, uint32_t colourImage)
{
    gfx_draw_sprite_raw_masked_software(_dpi, x, y, maskImage, colourImage);
}

void X8DrawingContext::DrawSpriteSolid(uint32_t image, int32_t x, int32_t y, uint8_t colour)
{
    uint8_t palette[256];
    std::fill_n(palette, sizeof(palette), colour);
    palette[0] = 0;
    gfx_draw_sprite_palette_set_software(_dpi, ImageId::FromUInt32((image & 0x7FFFF) | IMAGE_TYPE_REMAP), x, y, palette);
}

void X8DrawingContext::DrawGlyph(uint32_t image, int32_t x, int32_t y, uint8_t* palette)
{
    gfx_draw_sprite_palette_set_software(_dpi, ImageId::FromUInt32(image), x, y, palette);
}

void X8DrawingContext::SetDPI(rct_drawpixelinfo* dpi)
{
    _dpi = dpi;
}
