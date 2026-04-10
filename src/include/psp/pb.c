/*=========================================================
	
	pb.c
	
	ピクセルの転送を行う。
	
=========================================================*/
#include <pspge.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "cgerrs.h"
#include "pb.h"

/* もうフォントは決めうち */
#ifdef PB_SJIS_SUPPORT
#include "psp/fonts/misaki8x8.c"
#else
#include "psp/fonts/font_cg.c"
#endif

/*=========================================================
	マクロ
=========================================================*/
#define PB_DISABLE_CACHE 0x40000000

#define PB_SWAP( a, b ) { \
	int t = *(int *)a; \
	*(int *)a = *(int *)b; \
	*(int *)b = t; \
}

#define PB_BLEND_FUNC( src_ch, dst_ch, alpha, maxbits ) ( ( ( ( src_ch ) + 1 - ( dst_ch ) ) * ( alpha ) >> ( maxbits ) ) + ( dst_ch ) )

#define PB_COLOR_EXTRACT_8888( c, r, g, b, a ) \
	r = ( c ) & 0xFF; \
	g = ( ( c ) >>  8 ) & 0xFF; \
	b = ( ( c ) >> 16 ) & 0xFF; \
	a = ( ( c ) >> 24 ) & 0xFF;

#define PB_COLOR_EXTRACT_4444( c, r, g, b, a ) \
	r = ( c ) & 0xF; \
	g = ( ( c ) >>  4 ) & 0xF; \
	b = ( ( c ) >>  8 ) & 0xF; \
	a = ( ( c ) >> 12 ) & 0xF;

#define PB_COLOR_EXTRACT_5551( c, r, g, b, a ) \
	r = ( c ) & 0x1F; \
	g = ( ( c ) >> 5 ) & 0x1F; \
	b = ( ( c ) >> 10 ) & 0x1F; \
	a = ( ( c ) >> 15 ) & 0x1 ? 1 : 0;

#define PB_COLOR_EXTRACT_5650( c, r, g, b ) \
	r = ( c ) & 0x1F; \
	g = ( ( c ) >>  5 ) & 0x3F; \
	b = ( ( c ) >> 11 ) & 0x1F;

#define PB_COLOR_JOIN_8888( r, g, b, a ) ( (unsigned int)( r ) | (unsigned int)( g ) << 8 | (unsigned int)( b ) << 16 | (unsigned int)( a ) << 24 )
#define PB_COLOR_JOIN_4444( r, g, b, a ) ( (unsigned int)( r ) | (unsigned int)( g ) << 4 | (unsigned int)( b ) <<  8 | (unsigned int)( a ) << 12 )
#define PB_COLOR_JOIN_5551( r, g, b, a ) ( (unsigned int)( r ) | (unsigned int)( g ) << 5 | (unsigned int)( b ) << 10 | (unsigned int)( a ) << 15 )
#define PB_COLOR_JOIN_5650( r, g, b )    ( (unsigned int)( r ) | (unsigned int)( g ) << 5 | (unsigned int)( b ) << 11 )

#define PB_PUT_PIXEL( a, c ) { \
	unsigned int pc = c; \
	if( st_params.draw->colorConv ) pc = ( st_params.draw->colorConv )( pc, (void *)a ); \
	memcpy( (void *)a, (void *)&pc, st_params.draw->pixelSize ); \
}

/*=========================================================
	型宣言
=========================================================*/
typedef unsigned int ( *color_convert  )( unsigned int, void* );

enum pb_blend_target {
	PB_BLEND_SRC,
	PB_BLEND_DST
};

struct pb_frame_buffer {
	int  format;
	void *addr;
	int  width;
	
	unsigned char pixelSize;
	unsigned int  lineSize;
	
	color_convert   colorConv;
};

struct pb_params {
	struct pb_frame_buffer frame0, frame1;
	struct pb_frame_buffer *display, *draw;
	unsigned int   options;
	unsigned int   blendFactor;
	unsigned int   linebreakWidth;
};

/*=========================================================
	ローカル関数
=========================================================*/
static color_convert pb_get_color_converter( int format );
static color_convert pb_get_color_blender( int format );
static void *pb_buf_addr( struct pb_frame_buffer *fb, int x, int y );

#ifdef PB_SJIS_SUPPORT
static unsigned short pb_get_glyph_index_by_sjis( unsigned char sjis_hi, unsigned char sjis_lo );
#endif

static unsigned int pb_color_8888_5650( unsigned int color, void *null );
static unsigned int pb_color_8888_5551( unsigned int color, void *null );
static unsigned int pb_color_8888_4444( unsigned int color, void *null );

static unsigned int pb_color_blend_8888( unsigned int src, void *dst_addr );
static unsigned int pb_color_blend_4444( unsigned int src, void *dst_addr );
static unsigned int pb_color_blend_5551( unsigned int src, void *dst_addr );
static unsigned int pb_color_blend_5650( unsigned int src, void *dst_addr );

/*=========================================================
	ローカル変数
=========================================================*/
static struct pb_params st_params;

/*=========================================================
	関数
=========================================================*/
static color_convert pb_get_color_converter( int format )
{
	switch( format ){
		case PB_PXF_4444:
			return pb_color_8888_4444;
		case PB_PXF_5551:
			return pb_color_8888_5551;
		case PB_PXF_5650:
			return pb_color_8888_5650;
		default:
			return NULL;
	}
}

static color_convert pb_get_color_blender( int format )
{
	switch( format ){
		case PB_PXF_8888:
			return pb_color_blend_8888;
		case PB_PXF_4444:
			return pb_color_blend_4444;
		case PB_PXF_5551:
			return pb_color_blend_5551;
		case PB_PXF_5650:
			return pb_color_blend_5650;
		default:
			return NULL;
	}
}

/*-----------------------------------------------
	設定
-----------------------------------------------*/
void pbInit( void )
{
	st_params.frame0.format    = 0;
	st_params.frame0.addr      = NULL;
	st_params.frame0.width     = 0;
	st_params.frame0.pixelSize = 0;
	st_params.frame0.lineSize  = 0;
	st_params.frame0.colorConv = NULL;
	
	st_params.frame1.format    = 0;
	st_params.frame1.addr      = NULL;
	st_params.frame1.width     = 0;
	st_params.frame1.pixelSize = 0;
	st_params.frame1.lineSize  = 0;
	st_params.frame1.colorConv = NULL;
	
	st_params.display        = &(st_params.frame0);
	st_params.draw           = &(st_params.frame1);
	st_params.options        = 0;
	st_params.blendFactor    = 0;
	st_params.linebreakWidth = 0;
}

void pbSetDisplayBuffer( int format, void *fbp, int width )
{
	st_params.display->format = format;
	st_params.display->addr   = fbp;
	st_params.display->width  = width;
}

void pbSetDrawBuffer( int format, void *fbp, int width )
{
	st_params.draw->format = format;
	st_params.draw->addr   = fbp;
	st_params.draw->width  = width;
}

void pbGetCurrentDisplayBuffer( int *__restrict__ format, void **__restrict__ fbp, int *__restrict__ width )
{
	*format = st_params.display->format;
	*fbp    = st_params.display->addr;
	*width  = st_params.display->width;
}

void pbGetCurrentDrawBuffer( int *__restrict__ format, void **__restrict__ fbp, int *__restrict__ width )
{
	*format = st_params.draw->format;
	*fbp    = st_params.draw->addr;
	*width  = st_params.draw->width;
}

void *pbGetCurrentDisplayBufferAddr( void )
{
	return st_params.display->addr;
}

void *pbGetCurrentDrawBufferAddr( void )
{
	return st_params.draw->addr;
}

void pbSetOptions( unsigned int opt )
{
	st_params.options = opt;
}

unsigned int pbGetOptions( void )
{
	return st_params.options;
}

void pbEnable( unsigned int opt )
{
	st_params.options |= opt;
}

void pbDisable( unsigned int opt )
{
	st_params.options &= ~opt;
}

bool pbIsEnabled( unsigned int opt )
{
	return st_params.options & opt ? true : false;
}

int pbApply( void )
{
	if( ! st_params.display->addr ) return CG_ERROR_INVALID_ARGUMENT;
	if( ! st_params.draw->addr ) *(st_params.draw) = *(st_params.display);
	
	st_params.display->pixelSize = pbGetPixelDataSize( st_params.display->format );
	st_params.draw->pixelSize    = pbGetPixelDataSize( st_params.draw->format );
	
	st_params.display->lineSize = st_params.display->width * st_params.display->pixelSize;
	st_params.draw->lineSize    = st_params.draw->width * st_params.draw->pixelSize;
	
	if( st_params.options & PB_NO_CACHE ){
		if( st_params.display->addr ) st_params.display->addr = (void *)( (uintptr_t)st_params.display->addr | PB_DISABLE_CACHE );
		if( st_params.draw->addr    ) st_params.draw->addr    = (void *)( (uintptr_t)st_params.draw->addr | PB_DISABLE_CACHE );
	} else{
		st_params.display->addr = (void *)( (uintptr_t)st_params.display->addr & ~PB_DISABLE_CACHE );
		st_params.draw->addr    = (void *)( (uintptr_t)st_params.draw->addr & ~PB_DISABLE_CACHE );
	}
	
	if( st_params.options & PB_BLEND ){
		st_params.display->colorConv = pb_get_color_blender( st_params.display->format );
		st_params.draw->colorConv    = pb_get_color_blender( st_params.draw->format );
	} else{
		st_params.display->colorConv = pb_get_color_converter( st_params.display->format );
		st_params.draw->colorConv    = pb_get_color_converter( st_params.draw->format );
	}
	
	return 0;
}

int pbSync( int bufsync )
{
	return sceDisplaySetFrameBuf( st_params.display->addr, st_params.display->width, st_params.display->format, bufsync );
}

void *pbSwapBuffers( int bufsync )
{
	if( st_params.draw != st_params.display ){
		struct pb_frame_buffer *swap = st_params.display;
		st_params.display = st_params.draw;
		st_params.draw = swap;
		sceDisplaySetFrameBuf( st_params.display->addr, st_params.display->width, st_params.display->format, bufsync );
	}
	return st_params.draw->addr;
}

/*-----------------------------------------------
	計算
-----------------------------------------------*/
int pbGetPixelDataSize( int format )
{
	switch( format ){
		case PB_PXF_8888:
			return 4;
		default:
			return 2;
	}
}

int pbGetFrameBufferDataSize( int format, int width )
{
	return width * PB_SCREEN_HEIGHT * pbGetPixelDataSize( format );
}


unsigned int pbMeasureNString( const char *str, int n )
{
	unsigned int i;
	unsigned int len;
	
	for( i = 0, len = 0; str[i]; i++ ){
		if( n >= 0 ){
			if( n == 0 ){
				break;
			} else{
				n--;
			}
		}
		
#ifdef PB_SJIS_SUPPORT
		if( (unsigned char)str[i] < 0x80 ){
			len += PB_CHAR_WIDTH;
		} else if( (unsigned char)str[i] >= 0xA0 && (unsigned char)str[i] <= 0xDF ){
			len += PB_CHAR_WIDTH;
		} else if( n && str[i + 1] ){
			len += PB_WCHAR_WIDTH;
			i++;
			n--;
		}
#else
		len += PB_CHAR_WIDTH;
#endif
	}
	
	return len;
}

/*-----------------------------------------------
	描画
-----------------------------------------------*/
int pbPrint( int x, int y, unsigned int fg, unsigned int bg, const char *str )
{
	uintptr_t      start_addr, put_addr;
	unsigned int   i, glyph_index;
	const char     *glyph;
	unsigned char  glyph_line_data, chr_width, chr_width_bytes;
	unsigned short glyph_x, glyph_y;
	unsigned int   color;
	
#ifndef PB_SJIS_SUPPORT
	chr_width       = PB_CHAR_WIDTH;
	chr_width_bytes = PB_CHAR_WIDTH * st_params.draw->pixelSize;
#endif
	
	start_addr      = (uintptr_t)pb_buf_addr( st_params.draw, x, y );
	put_addr        = start_addr;
	for( i = 0; str[i]; i++ ){
		if( (unsigned char)str[i] == '\n' ){
			start_addr += st_params.draw->pixelSize * st_params.draw->width * PB_CHAR_HEIGHT;
			put_addr = start_addr;
			continue;
		}
		
		/* グリフ取得 */
#ifdef PB_SJIS_SUPPORT
		if( (unsigned char)str[i] < 0x80 ){
			glyph = &(font_ascii[(unsigned char)str[i] * PB_CHAR_HEIGHT]);
			chr_width = PB_CHAR_WIDTH;
		} else if( (unsigned char)str[i] >= 0xA0 && (unsigned char)str[i] <= 0xDF ){
			glyph = &(font_ascii[(unsigned char)(str[i] - 0xA0) + 0x80]);
			chr_width = PB_CHAR_WIDTH;
		} else if( str[i + 1] ){
			glyph_index = pb_get_glyph_index_by_sjis( str[i], str[i + 1] );
			glyph = &(font_misaki[glyph_index * PB_CHAR_HEIGHT]);
			chr_width = PB_WCHAR_WIDTH;
			i++;
		} else{
			continue;
		}
		chr_width_bytes = chr_width * st_params.draw->pixelSize;
#else
		glyph_index = (unsigned char)str[i] * PB_CHAR_HEIGHT;
		if( glyph_index < sizeof( font_cg ) ){
			glyph = &(font_cg[glyph_index]);
		} else{
			glyph = &(font_cg['?' * PB_CHAR_HEIGHT]);
		}
#endif
		
		/* 描画 */
		for( glyph_y = 0; glyph_y < PB_CHAR_HEIGHT; glyph_y++, put_addr += st_params.draw->lineSize - chr_width_bytes ){
			glyph_line_data = glyph[glyph_y];
			for( glyph_x = 0; glyph_x < chr_width; glyph_x++, glyph_line_data <<= 1, put_addr += st_params.draw->pixelSize ){
				color = glyph_line_data & 0x80 ? fg : bg;
				if( color != PB_TRANSPARENT ) PB_PUT_PIXEL( put_addr, color );
			}
		}
		put_addr -= st_params.draw->pixelSize * st_params.draw->width * PB_CHAR_HEIGHT - chr_width_bytes;
	}
	return i;
}

int pbPutChar( int x, int y, unsigned int fg, unsigned int bg, const char chr )
{
	const char str[] = { chr, '\0' };
	return pbPrint( x, y, fg, bg, str );
}

int pbPrintf( int x, int y, unsigned int fg, unsigned int bg, const char *format, ... )
{
	char str[255];
	va_list ap;
	
	va_start( ap, format );
	vsnprintf( str, sizeof( str ), format, ap );
	va_end( ap );
	
	return pbPrint( x, y, fg, bg, str );
}

void pbPoint( int x, int y, unsigned int color )
{
	uintptr_t draw_addr;
	
	if( color == PB_TRANSPARENT ) return;
	
	draw_addr = (uintptr_t)pb_buf_addr( st_params.draw, x, y );
	PB_PUT_PIXEL( draw_addr, color );
}

void pbLine( int sx, int sy, int ex, int ey, uint32_t color )
{
	uintptr_t draw_addr;
	int       e, dx, dy;
	
	if( color == PB_TRANSPARENT ) return;
	
	if( sx > ex ) PB_SWAP( &sx, &ex );
	if( sy > ey ) PB_SWAP( &sy, &ey );
	
	draw_addr = (uintptr_t)pb_buf_addr( st_params.draw, sx, sy );
	
	if( sx == ex ){
		for( ; sy <= ey; sy++, draw_addr += st_params.draw->lineSize ) PB_PUT_PIXEL( draw_addr, color );
	} else if( sy == ey ){
		for( ; sx <= ex; sx++, draw_addr += st_params.draw->pixelSize ) PB_PUT_PIXEL( draw_addr, color );
	} else{
		/* ブレゼンハムの線分描画アルゴリズム */
		e  = 0;
		dx = ex - sx;
		dy = ey - sy;
		
		if( dx > dy ){
			int x;
			for( x = sx; x <= ex; x++, draw_addr += st_params.draw->pixelSize ){
				e += dy;
				if( e > dx ){
					e -= dx;
					draw_addr += st_params.draw->lineSize;
				}
				PB_PUT_PIXEL( draw_addr, color );
			}
		} else{
			int y;
			for( y = sy; y <= ey; y++, draw_addr += st_params.draw->lineSize ){
				e += dx;
				if( e > dy ){
					e -= dy;
					draw_addr += st_params.draw->pixelSize;
				}
				PB_PUT_PIXEL( draw_addr, color );
			}
		}
	}
}

void pbLineRect( int sx, int sy, int ex, int ey, uint32_t color )
{
	if( color == PB_TRANSPARENT ) return;
	
	pbLine( sx, sy, ex, sy, color );
	pbLine( ex, sy, ex, ey, color );
	pbLine( ex, ey, sx, ey, color );
	pbLine( sx, ey, sx, sy, color );
}
 
void pbFillRect( int sx, int sy, int ex, int ey, uint32_t color )
{
	uintptr_t    start_addr, draw_addr;
	unsigned int offset;
	int          w, h, x;
	
	if( color == PB_TRANSPARENT ) return;
	
	if( sx > ex ) PB_SWAP( &sx, &ex );
	if( sy > ey ) PB_SWAP( &sy, &ey );
	
	w = ex - sx;
	h = ey - sy;
	
	start_addr = (uintptr_t)pb_buf_addr( st_params.draw, sx, sy );
	for( ; h; h--, start_addr += st_params.draw->lineSize ){
		offset = 0;
		for( x = 0; x < w; x++, offset += st_params.draw->pixelSize ){
			draw_addr = start_addr + offset;
			PB_PUT_PIXEL( draw_addr, color );
		}
	}
}

/* ミッチェナーの円弧描画アルゴリズム */
void pbLineCircle( int x, int y, unsigned int radius, uint32_t color )
{
	int cx = 0, cy = radius;
	int d = 3 - 2 * radius;
	
	if( color == PB_TRANSPARENT || ! radius ) return;
	
	/* 開始点 */
	pbPoint( x, y + radius, color ); /* ( 0, R) */
	pbPoint( x, y - radius, color ); /* ( 0,-R) */
	pbPoint( x + radius, y, color ); /* ( R, 0) */
	pbPoint( x - radius, y, color ); /* (-R, 0) */
	
	for( cx = 0; cx <= cy; cx++ ){
		if( d >= 0 ){
			d += 10 + 4 * cx - 4 * cy--;
		} else{
			d += 6 + 4 * cx;
		}
		
		pbPoint( x + cy, y + cx, color ); /*   0-45  度 */
		pbPoint( x + cx, y + cy, color ); /*  45-90  度 */
		pbPoint( x - cx, y + cy, color ); /*  90-135 度 */
		pbPoint( x - cy, y + cx, color ); /* 135-180 度 */
		pbPoint( x - cy, y - cx, color ); /* 180-225 度 */
		pbPoint( x - cx, y - cy, color ); /* 225-270 度 */
		pbPoint( x + cx, y - cy, color ); /* 270-315 度 */
		pbPoint( x + cy, y - cx, color ); /* 315-360 度 */
	}
}

/*-----------------------------------------------
	座標->アドレス変換
-----------------------------------------------*/
static void *pb_buf_addr( struct pb_frame_buffer *fb, int x, int y )
{
	return (void *)( (uintptr_t)fb->addr + ( x + y * fb->width ) * fb->pixelSize );
}

/*-----------------------------------------------
	文字コード変換
	
	参考にしたサイト: http://xaiax.at.infoseek.co.jp/ConvertUnicode_java.html
-----------------------------------------------*/
#ifdef PB_SJIS_SUPPORT
static unsigned short pb_get_glyph_index_by_sjis( unsigned char sjis_hi, unsigned char sjis_lo )
{
	unsigned char  kuten_hi = sjis_hi - 0x81;
	unsigned short kuten_lo = sjis_lo - 0x40;
	
	if( sjis_hi > 0x9F ) kuten_hi -= 0x40;
	if( sjis_lo > 0x7E ) kuten_lo--;
	
	kuten_hi <<= 1;
	if( kuten_lo > 0x5D ) kuten_hi++;
	
	kuten_lo %= 0x5E;
	
	/* 16区以降であれば、未定義の9～15区分を引く */
	if( kuten_hi >= 15 ){
		kuten_hi -= 7;
		kuten_lo--;
	}
	
	return kuten_hi * 94 + kuten_lo;
}
#endif

/*-----------------------------------------------
	色の変換
-----------------------------------------------*/
static unsigned int pb_color_8888_5650( unsigned int color, void *null )
{
	register unsigned char r, g, b;
	r = ( color >> 3  ) & 0x1F;
	g = ( color >> 10 ) & 0x3F;
	b = ( color >> 19 ) & 0x1F;
	return PB_COLOR_JOIN_5650( r, g, b );
}

static unsigned int pb_color_8888_5551( unsigned int color, void *null )
{
	register unsigned char r, g, b, a;
	r = ( color >> 3  ) & 0x1F;
	g = ( color >> 11 ) & 0x1F;
	b = ( color >> 19 ) & 0x1F;
	a = ( color >> 24 ) ? 1 : 0;
	return PB_COLOR_JOIN_5551( r, g, b, a );
}

static unsigned int pb_color_8888_4444( unsigned int color, void *null )
{
	register unsigned char r, g, b, a;
	r = ( color >> 4  ) & 0xF;
	g = ( color >> 12 ) & 0xF;
	b = ( color >> 20 ) & 0xF;
	a = ( color >> 28 ) & 0xF;
	return PB_COLOR_JOIN_4444( r, g, b, a );
}

/*-----------------------------------------------
	ブレンド
-----------------------------------------------*/
static unsigned int pb_color_blend_8888( unsigned int src, void *dst_addr )
{
	uint8_t      alpha = ( src >> 24 ) & 0xFF;
	unsigned int dst   = *((uint32_t *)dst_addr);
	
	if( ! alpha ){
		return dst;
	} else if( alpha == 0xFF ){
		return src;
	} else{
		uint32_t color = alpha << 24;
		color |= PB_BLEND_FUNC( ( src       ) & 0xFF, ( dst       ) & 0xFF, alpha, 8 );
		color |= PB_BLEND_FUNC( ( src >>  8 ) & 0xFF, ( dst >>  8 ) & 0xFF, alpha, 8 ) << 8;
		color |= PB_BLEND_FUNC( ( src >> 16 ) & 0xFF, ( dst >> 16 ) & 0xFF, alpha, 8 ) << 16;
		return color;
	}
}

static unsigned int pb_color_blend_4444( unsigned int src, void *dst_addr )
{
	uint8_t  alpha = ( src >> 12 ) & 0xF;
	uint16_t dst   = *((uint32_t *)dst_addr);
	
	src = pb_color_8888_4444( src, NULL );
	
	if( ! alpha ){
		return dst;
	} else if( alpha == 0xF ){
		return src;
	} else{
		uint16_t color = alpha << 12;
		color |= PB_BLEND_FUNC( ( src      ) & 0xF, ( dst      ) & 0xF, alpha, 4 );
		color |= PB_BLEND_FUNC( ( src >> 4 ) & 0xF, ( dst >> 4 ) & 0xF, alpha, 4 ) << 4;
		color |= PB_BLEND_FUNC( ( src >> 8 ) & 0xF, ( dst >> 4 ) & 0xF, alpha, 4 ) << 8;
		return color;
	}
}

static unsigned int pb_color_blend_5551( unsigned int src, void *dst_addr )
{
	uint8_t  alpha = ( src >> 27 ) & 0x1F;
	uint16_t dst   = *((uint32_t *)dst_addr);
	
	src = pb_color_8888_5551( src, NULL );
	
	if( ! alpha ){
		return dst;
	} else if( alpha == 0x1F ){
		return src;
	} else{
		uint16_t color = ( alpha ? 1 : 0 ) << 15;
		color |= PB_BLEND_FUNC( ( src       ) & 0x1F, ( dst       ) & 0x1F, alpha, 5 );
		color |= PB_BLEND_FUNC( ( src >>  5 ) & 0x1F, ( dst >>  5 ) & 0x1F, alpha, 5 ) << 5;
		color |= PB_BLEND_FUNC( ( src >> 10 ) & 0x1F, ( dst >> 10 ) & 0x1F, alpha, 5 ) << 10;
		return color;
	}
}

static unsigned int pb_color_blend_5650( unsigned int src, void *dst_addr )
{
	uint8_t  alpha = ( src >> 26 ) & 0x3F;
	uint16_t dst   = *((uint32_t *)dst_addr);
	
	src = pb_color_8888_5650( src, NULL );
	
	if( ! alpha ){
		return dst;
	} else if( alpha == 0x3F ){
		return src;
	} else{
		uint16_t color = 0;
		color |= PB_BLEND_FUNC( ( src       ) & 0x1F, ( dst       ) & 0x1F, ( alpha >> 1 ) & 0x1F, 5 );
		color |= PB_BLEND_FUNC( ( src >>  5 ) & 0x3F, ( dst >>  5 ) & 0x3F, alpha                , 6 ) << 5;
		color |= PB_BLEND_FUNC( ( src >> 11 ) & 0x1F, ( dst >> 11 ) & 0x1F, ( alpha >> 1 ) & 0x1F, 5 ) << 11;
		return color;
	}
}
