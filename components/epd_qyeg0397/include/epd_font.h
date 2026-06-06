/**
 * @file    epd_font.h
 * @brief   8×16 monospace bitmap font + text rendering for EPD buffer
 *
 * The font covers ASCII 32–126 (95 printable characters).
 * Each glyph is 8 pixels wide × 16 pixels tall.
 * At 800×480 this gives 100 columns × 30 rows.
 */

#ifndef EPD_FONT_HEADER_H
#define EPD_FONT_HEADER_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*  Font metrics                                                             */
/* ========================================================================= */
#define EPD_FONT_W          8
#define EPD_FONT_H          16
#define EPD_FONT_START      32      /* first code-point (space)             */
#define EPD_FONT_END        126     /* last code-point  (~)                */
#define EPD_FONT_COUNT      (EPD_FONT_END - EPD_FONT_START + 1)
#define EPD_FONT_BYTES      (EPD_FONT_COUNT * EPD_FONT_H)

extern const uint8_t epd_font_data[EPD_FONT_BYTES];

/* ========================================================================= */
/*  Text rendering                                                           */
/* ========================================================================= */

/**
 * @brief  Draw a single character into an EPD pixel buffer.
 *
 * @param  buf     EPD frame buffer (EPD_DATA_SIZE bytes, 2-bit packed).
 * @param  x, y    Top-left pixel position (0-based).
 * @param  ch      ASCII character to draw.
 * @param  colour  One of EPD_COLOR_* (2-bit).
 * @return Number of horizontal pixels consumed (always EPD_FONT_W).
 */
int epd_draw_char(uint8_t *buf, int x, int y, char ch, uint8_t colour);

/**
 * @brief  Draw a null-terminated string into an EPD buffer.
 *
 * No automatic wrapping — characters that fall outside the display are
 * silently clipped.
 *
 * @param  buf     EPD frame buffer.
 * @param  x, y    Top-left of first character.
 * @param  str     Null-terminated string.
 * @param  colour  Foreground colour.
 * @return Total horizontal pixels consumed (charcount × 8).
 */
int epd_draw_string(uint8_t *buf, int x, int y,
                    const char *str, uint8_t colour);

/**
 * @brief  Draw a multi-line string with automatic wrapping.
 *
 * If a line exceeds the right margin it wraps to the next line.
 * Text that falls below the display bottom is clipped.
 *
 * @param  buf       EPD frame buffer.
 * @param  x, y      Starting position.
 * @param  str       Null-terminated string (may contain '\n').
 * @param  colour    Foreground colour.
 * @param  margin_r  Right margin (x beyond which to wrap).
 */
void epd_draw_string_wrap(uint8_t *buf, int x, int y,
                          const char *str, uint8_t colour, int margin_r);

/**
 * @brief  Get the width in pixels a string would occupy.
 */
int epd_text_width(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* EPD_FONT_HEADER_H */
