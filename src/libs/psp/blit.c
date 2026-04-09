/*
	PSP VSH 24bpp text bliter (Original from "SDK for devhook 0.43")
*/

#include "blit.h"

#include "font.c"

u16 blitColorConvert565( u32 color )
{
	unsigned int r, g, b;
	
	r = ( color >> 3  ) & MASK_5BIT;
	g = ( color >> 10 ) & MASK_6BIT;
	b = ( color >> 19 ) & MASK_5BIT;
	
	return ( r | ( g << 5 ) | ( b << 11 ) );
}

u16 blitColorConvert5551( u32 color )
{
	unsigned int a, r, g, b;
	
	a = ( color >> 24 ) ? ALPHA_BIT : 0;
	r = ( color >> 3  ) & MASK_5BIT;
	g = ( color >> 11 ) & MASK_5BIT;
	b = ( color >> 19 ) & MASK_5BIT;
	
	return ( a | r | ( g << 5 ) | ( b << 10 ) );
}

u16 blitColorConvert4444( u32 color )
{
	unsigned int a, r, g, b;
	
	a = ( color >> 28 ) & MASK_4BIT; 
	r = ( color >> 4  ) & MASK_4BIT;
	g = ( color >> 12 ) & MASK_4BIT;
	b = ( color >> 20 ) & MASK_4BIT;
	
	return ( ( a << 12 ) | r | ( g << 4 ) | ( b << 8 ) );
}

int blitGetPixelLength( enum PspDisplayPixelFormats pxfmt )
{
	switch( pxfmt ){
		case PSP_DISPLAY_PIXEL_FORMAT_565:
		case PSP_DISPLAY_PIXEL_FORMAT_5551:
		case PSP_DISPLAY_PIXEL_FORMAT_4444:
			return 2;
		case PSP_DISPLAY_PIXEL_FORMAT_8888:
		default:
			return 4;
	}
}

u32 blitConvertColorFrom8888( enum PspDisplayPixelFormats pxfmt, u32 color )
{
	switch( pxfmt ){
		case PSP_DISPLAY_PIXEL_FORMAT_565:
			return blitColorConvert565( color );
		case PSP_DISPLAY_PIXEL_FORMAT_5551:
			return blitColorConvert5551( color );
		case PSP_DISPLAY_PIXEL_FORMAT_4444:
			return blitColorConvert4444( color );
		case PSP_DISPLAY_PIXEL_FORMAT_8888:
		default:
			return color;
	}
}

int blitString( unsigned int sx, unsigned int sy, const char *msg, u32 fgcolor, u32 bgcolor )
{
	int c, x, y, p;
	int offset;
	char code;
	unsigned char font;
	int mode, width, height, bufwidth, pxfmt, pxlen;
	unsigned int vram;
	
	sceDisplayGetMode( &mode, &width, &height );
	sceDisplayGetFrameBuf( (void **)&vram, &bufwidth, &pxfmt, PSP_DISPLAY_SETBUF_IMMEDIATE );
	
	if( ! bufwidth || ! vram || ( fgcolor < 0 || bgcolor < 0 ) ) return -1;
	
	pxlen   = blitGetPixelLength( pxfmt );
	fgcolor = blitConvertColorFrom8888( pxfmt, fgcolor );
	bgcolor = blitConvertColorFrom8888( pxfmt, bgcolor );
	
	for( c = 0, x = 0; msg[c]; c++, x++ ){
		if( msg[c] == '\n' || x >= ( width / 7 ) ){
			sy += 8;
			if( msg[c] == '\n' ){
				x = -1;
				continue;
			} else{
				x = 0;
			}
		}
		
		code = msg[c] & 0x7f; // 7bit ANK
		for( y = 0; y < 8; y++ ){
			offset = ( ( sy + y ) * bufwidth + ( sx + x ) * 7 ) * pxlen;
			font = msx[ code*8 + y ];
			for( p = 0; p < 8; p++ ){
				if( font & 0x80 ){
					memcpy( (void *)(vram + offset), (void *)&fgcolor, pxlen );
				} else{
					memcpy( (void *)(vram + offset), (void *)&bgcolor, pxlen );
				}
				font <<= 1;
				offset += pxlen;
			}
		}
	}
	return c;
}

void blitFillRect( unsigned int x, unsigned int y, unsigned int w, unsigned int h, u32 color )
{
	int cx, cy;
	int offset;
	int mode, width, height, bufwidth, pxfmt, pxlen;
	unsigned int vram;
	
	sceDisplayGetMode( &mode, &width, &height );
	sceDisplayGetFrameBuf( (void **)&vram, &bufwidth, &pxfmt, PSP_DISPLAY_SETBUF_IMMEDIATE );
	
	pxlen = blitGetPixelLength( pxfmt );
	color = blitConvertColorFrom8888( pxfmt, color );
	
	for( cy = 0; cy < h; cy++ ){
		offset = ( x + ( ( y + cy ) * bufwidth ) ) * pxlen;
		for( cx = 0; cx < w; cx++ ){
			if( cx >= 512 ) break;
			
			memcpy( (void *)(vram + offset), (void *)&color, pxlen );
			offset += pxlen;
		}
	}
}
