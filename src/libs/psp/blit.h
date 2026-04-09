/*
	PSPのVRAM上に文字を描画する。
*/

#ifndef __BLIT_H__
#define __BLIT_H__

#include <pspdisplay.h>
#include <stdio.h>
#include <string.h>

#define ALPHA_BIT 0x8000
#define MASK_4BIT 0xF
#define MASK_5BIT 0x1F
#define MASK_6BIT 0x3F

#ifdef __cplusplus
extern "C" {
#endif

/* blitColorConvert565
	PSP_DISPLAY_PIXEL_FORAMT_8888形式の色を
	PSP_DISPLAY_PIXEL_FORAMT_565形式に変換する。
	
	@param u32 color
		変換したいPSP_DISPLAY_PIXEL_FORAMT_8888形式の色。
	
	@return u16
		変換された色。
*/
u16 blitColorConvert565( u32 color );

/* blitColorConvert5551
	PSP_DISPLAY_PIXEL_FORAMT_8888形式の色を
	PSP_DISPLAY_PIXEL_FORAMT_5551形式に変換する。
	
	@param u32 color
		変換したいPSP_DISPLAY_PIXEL_FORAMT_8888形式の色。
	
	@return u16
		変換された色。
*/
u16 blitColorConvert5551( u32 color );

/* blitColorConvert4444
	PSP_DISPLAY_PIXEL_FORAMT_8888形式の色を
	PSP_DISPLAY_PIXEL_FORAMT_4444形式に変換する。
	
	@param u32 color
		変換したいPSP_DISPLAY_PIXEL_FORAMT_8888形式の色。
	
	@return u16
		変換された色。
*/
u16 blitColorConvert4444( u32 color );

/* blitConvertColorFrom8888
	PSP_DISPLAY_PIXEL_FORAMT_8888形式の色を
	指定したピクセルフォーマットの形式に変換する。
	
	@param u32 color
		変換したいPSP_DISPLAY_PIXEL_FORAMT_8888形式の色。
	
	@return u32
		変換された色。
*/
u32 blitConvertColorFrom8888( enum PspDisplayPixelFormats pxfmt, u32 color );

/* blitGetPixelLength
	渡したピクセルフォーマットのデータのバイト数を返す。
	
	@param enum PspDisplayPixelFormats pxfmt
		長さを調べたいピクセルフォーマット。
	
	@return int
		バイト数。
*/
int blitGetPixelLength( enum PspDisplayPixelFormats pxfmt );

/* blitFillRect
	指定した範囲を塗りつぶす。
	
	@param unsigned int x
		塗りつぶしを開始するX座標。
	
	@param unsigned int y
		塗りつぶしを開始するY座標。
	
	@param unsigned int w
		塗りつぶす長さ。
	
	@param unsigned int h
		塗りつぶす高さ。
*/
void blitFillRect( unsigned int x, unsigned int y, unsigned int w, unsigned int h, u32 color );

/* blitString
	任意の位置に文字列を描画する。
	
	@param unsigned int sx
		描画を開始するX座標。
	
	@param unsigned int sy
		描画を開始するY座標。
	
	@param const char *msg
		描画する文字列。
	
	@param u32 fgcolor
		文字の色。
	
	@param u32 bgcolor
		文字の背景の色。
	
	@return int
		描画した文字数。
*/
int blitString( unsigned int sx, unsigned int sy, const char *msg, u32 fgcolor, u32 bgcolor );

#ifdef __cplusplus
}
#endif

#endif
