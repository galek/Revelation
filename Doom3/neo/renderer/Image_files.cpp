/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"

/*

This file only has a single entry point:

void R_LoadImage( const char *name, byte **pic, int *width, int *height, bool makePowerOf2 );

*/

/*
================
R_WriteTGA
================
*/
void R_WriteTGA( const char *filename, const byte *data, int width, int height, bool flipVertical ) {
	byte	*buffer;
	int		i;
	int		bufferSize = width * height * 4 + 18;
	int     imgStart = 18;
	buffer = ( byte * )Mem_Alloc( bufferSize );
	memset( buffer, 0, 18 );
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 32;	// pixel size
	if( !flipVertical ) {
		buffer[17] = ( 1 << 5 );	// flip bit, for normal top to bottom raster order
	}
	// swap rgb to bgr
	for( i = imgStart ; i < bufferSize ; i += 4 ) {
		buffer[i] = data[i - imgStart + 2];			// blue
		buffer[i + 1] = data[i - imgStart + 1];		// green
		buffer[i + 2] = data[i - imgStart + 0];		// red
		buffer[i + 3] = data[i - imgStart + 3];		// alpha
	}
	fileSystem->WriteFile( filename, buffer, bufferSize );
	Mem_Free( buffer );
}

/*
================
R_WritePalTGA
================
*/
void R_WritePalTGA( const char *filename, const byte *data, const byte *palette, int width, int height, bool flipVertical ) {
	byte	*buffer;
	int		i;
	int		bufferSize = ( width * height ) + ( 256 * 3 ) + 18;
	int     palStart = 18;
	int     imgStart = 18 + ( 256 * 3 );
	buffer = ( byte * )Mem_Alloc( bufferSize );
	memset( buffer, 0, 18 );
	buffer[1] = 1;		// color map type
	buffer[2] = 1;		// uncompressed color mapped image
	buffer[5] = 0;		// number of palette entries (lo)
	buffer[6] = 1;		// number of palette entries (hi)
	buffer[7] = 24;		// color map bpp
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 8;	// pixel size
	if( !flipVertical ) {
		buffer[17] = ( 1 << 5 );	// flip bit, for normal top to bottom raster order
	}
	// store palette, swapping rgb to bgr
	for( i = palStart ; i < imgStart ; i += 3 ) {
		buffer[i] = palette[i - palStart + 2];		// blue
		buffer[i + 1] = palette[i - palStart + 1];		// green
		buffer[i + 2] = palette[i - palStart + 0];		// red
	}
	// store the image data
	for( i = imgStart ; i < bufferSize ; i++ ) {
		buffer[i] = data[i - imgStart];
	}
	fileSystem->WriteFile( filename, buffer, bufferSize );
	Mem_Free( buffer );
}

static void LoadBMP( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp );
static void LoadTGA( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp );
static void LoadJPG( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp );

/*
========================================================================

PCX files are used for 8 bit images

========================================================================
*/

typedef struct {
	char			manufacturer;
	char			version;
	char			encoding;
	char			bits_per_pixel;
	unsigned short	xmin, ymin, xmax, ymax;
	unsigned short	hres, vres;
	unsigned char	palette[48];
	char			reserved;
	char			color_planes;
	unsigned short	bytes_per_line;
	unsigned short	palette_type;
	char			filler[58];
	unsigned char	data;			// unbounded
} pcx_t;

/*
========================================================================

TGA files are used for 24/32 bit images

========================================================================
*/

#define TGA_MAXCOLORS 16384

/* Definitions for image types. */
#define TGA_Null		0	/* no image data */
#define TGA_Map			1	/* Uncompressed, color-mapped images. */
#define TGA_RGB			2	/* Uncompressed, RGB images. */
#define TGA_Mono		3	/* Uncompressed, black and white images. */
#define TGA_RLEMap		9	/* Runlength encoded color-mapped images. */
#define TGA_RLERGB		10	/* Runlength encoded RGB images. */
#define TGA_RLEMono		11	/* Compressed, black and white images. */
#define TGA_CompMap		32	/* Compressed color-mapped data, using Huffman, Delta, and runlength encoding. */
#define TGA_CompMap4	33	/* Compressed color-mapped data, using Huffman, Delta, and runlength encoding.  4-pass quadtree-type process. */

/* Definitions for interleave flag. */
#define TGA_IL_None		0	/* non-interleaved. */
#define TGA_IL_Two		1	/* two-way (even/odd) interleaving */
#define TGA_IL_Four		2	/* four way interleaving */
#define TGA_IL_Reserved	3	/* reserved */

/* Definitions for origin flag */
#define TGA_O_UPPER		0	/* Origin in lower left-hand corner. */
#define TGA_O_LOWER		1	/* Origin in upper left-hand corner. */

typedef struct _TargaHeader {
	unsigned char 	id_length, colormap_type, colormap_size, image_type;
	unsigned char	pixel_size, attributes;
	unsigned short	colormap_index, colormap_length;
	unsigned short	x_origin, y_origin, width, height;
} TargaHeader;

/*
=========================================================

BMP LOADING

=========================================================
*/
typedef struct {
	char id[2];
	unsigned long fileSize;
	unsigned long reserved0;
	unsigned long bitmapDataOffset;
	unsigned long bitmapHeaderSize;
	unsigned long width;
	unsigned long height;
	unsigned short planes;
	unsigned short bitsPerPixel;
	unsigned long compression;
	unsigned long bitmapDataSize;
	unsigned long hRes;
	unsigned long vRes;
	unsigned long colors;
	unsigned long importantColors;
	unsigned char palette[256][4];
} BMPHeader_t;

/*
==============
LoadBMP
==============
*/
static void LoadBMP( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp ) {
	int			columns, rows, numPixels;
	byte		*pixbuf;
	int			row, column;
	byte		*buf_p;
	byte		*buffer;
	int			length;
	BMPHeader_t bmpHeader;
	byte		*bmpRGBA;
	if( !pic ) {
		fileSystem->ReadFile( name, NULL, timestamp );
		return;	// just getting timestamp
	}
	*pic = NULL;
	//
	// load the file
	//
	length = fileSystem->ReadFile( name, ( void ** )&buffer, timestamp );
	if( !buffer ) {
		return;
	}
	buf_p = buffer;
	bmpHeader.id[0] = *buf_p++;
	bmpHeader.id[1] = *buf_p++;
	bmpHeader.fileSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.reserved0 = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataOffset = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapHeaderSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.width = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.height = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.planes = LittleShort( * ( short * ) buf_p );
	buf_p += 2;
	bmpHeader.bitsPerPixel = LittleShort( * ( short * ) buf_p );
	buf_p += 2;
	bmpHeader.compression = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.bitmapDataSize = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.hRes = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.vRes = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.colors = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	bmpHeader.importantColors = LittleLong( * ( long * ) buf_p );
	buf_p += 4;
	memcpy( bmpHeader.palette, buf_p, sizeof( bmpHeader.palette ) );
	if( bmpHeader.bitsPerPixel == 8 ) {
		buf_p += 1024;
	}
	if( bmpHeader.id[0] != 'B' && bmpHeader.id[1] != 'M' ) {
		common->Error( "LoadBMP: only Windows-style BMP files supported (%s)\n", name );
	}
	if( bmpHeader.fileSize != ( unsigned long )length ) {
		common->Error( "LoadBMP: header size does not match file size (%lu vs. %d) (%s)\n", bmpHeader.fileSize, length, name );
	}
	if( bmpHeader.compression != 0 ) {
		common->Error( "LoadBMP: only uncompressed BMP files supported (%s)\n", name );
	}
	if( bmpHeader.bitsPerPixel < 8 ) {
		common->Error( "LoadBMP: monochrome and 4-bit BMP files not supported (%s)\n", name );
	}
	columns = bmpHeader.width;
	rows = bmpHeader.height;
	if( rows < 0 ) {
		rows = -rows;
	}
	numPixels = columns * rows;
	if( width ) {
		*width = columns;
	}
	if( height ) {
		*height = rows;
	}
	bmpRGBA = ( byte * )R_StaticAlloc( numPixels * 4 );
	*pic = bmpRGBA;
	for( row = rows - 1; row >= 0; row-- ) {
		pixbuf = bmpRGBA + row * columns * 4;
		for( column = 0; column < columns; column++ ) {
			unsigned char red, green, blue, alpha;
			int palIndex;
			unsigned short shortPixel;
			switch( bmpHeader.bitsPerPixel ) {
			case 8:
				palIndex = *buf_p++;
				*pixbuf++ = bmpHeader.palette[palIndex][2];
				*pixbuf++ = bmpHeader.palette[palIndex][1];
				*pixbuf++ = bmpHeader.palette[palIndex][0];
				*pixbuf++ = 0xff;
				break;
			case 16:
				shortPixel = * ( unsigned short * ) pixbuf;
				pixbuf += 2;
				*pixbuf++ = ( shortPixel & ( 31 << 10 ) ) >> 7;
				*pixbuf++ = ( shortPixel & ( 31 << 5 ) ) >> 2;
				*pixbuf++ = ( shortPixel & ( 31 ) ) << 3;
				*pixbuf++ = 0xff;
				break;
			case 24:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = 255;
				break;
			case 32:
				blue = *buf_p++;
				green = *buf_p++;
				red = *buf_p++;
				alpha = *buf_p++;
				*pixbuf++ = red;
				*pixbuf++ = green;
				*pixbuf++ = blue;
				*pixbuf++ = alpha;
				break;
			default:
				common->Error( "LoadBMP: illegal pixel_size '%d' in file '%s'\n", bmpHeader.bitsPerPixel, name );
				break;
			}
		}
	}
	fileSystem->FreeFile( buffer );
}

/*
=================================================================

PCX LOADING

=================================================================
*/

/*
==============
LoadPCX
==============
*/
static void LoadPCX( const char *filename, byte **pic, byte **palette, int *width, int *height, ID_TIME_T *timestamp ) {
	byte	*raw;
	pcx_t	*pcx;
	int		x, y;
	int		len;
	int		dataByte, runLength;
	byte	*out, *pix;
	int		xmax, ymax;
	if( !pic ) {
		fileSystem->ReadFile( filename, NULL, timestamp );
		return;	// just getting timestamp
	}
	*pic = NULL;
	*palette = NULL;
	//
	// load the file
	//
	len = fileSystem->ReadFile( filename, ( void ** )&raw, timestamp );
	if( !raw ) {
		return;
	}
	//
	// parse the PCX file
	//
	pcx = ( pcx_t * )raw;
	raw = &pcx->data;
	xmax = LittleShort( pcx->xmax );
	ymax = LittleShort( pcx->ymax );
	if( pcx->manufacturer != 0x0a ||
		pcx->version != 5 ||
		pcx->encoding != 1 ||
		pcx->bits_per_pixel != 8 ||
		xmax >= 1024 ||
		ymax >= 1024 ) {
		common->Printf( "Bad pcx file %s (%i x %i) (%i x %i)\n", filename, xmax + 1, ymax + 1, pcx->xmax, pcx->ymax );
		return;
	}
	out = ( byte * )R_StaticAlloc( ( ymax + 1 ) * ( xmax + 1 ) );
	*pic = out;
	pix = out;
	if( palette ) {
		*palette = ( byte * )R_StaticAlloc( 768 );
		memcpy( *palette, ( byte * )pcx + len - 768, 768 );
	}
	if( width ) {
		*width = xmax + 1;
	}
	if( height ) {
		*height = ymax + 1;
	}
	// FIXME: use bytes_per_line here?
	for( y = 0 ; y <= ymax ; y++, pix += xmax + 1 ) {
		for( x = 0 ; x <= xmax ; ) {
			dataByte = *raw++;
			if( ( dataByte & 0xC0 ) == 0xC0 ) {
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			} else {
				runLength = 1;
			}
			while( runLength-- > 0 ) {
				pix[x++] = dataByte;
			}
		}
	}
	if( raw - ( byte * )pcx > len ) {
		common->Printf( "PCX file %s was malformed", filename );
		R_StaticFree( *pic );
		*pic = NULL;
	}
	fileSystem->FreeFile( pcx );
}

/*
==============
LoadPCX32
==============
*/
static void LoadPCX32( const char *filename, byte **pic, int *width, int *height, ID_TIME_T *timestamp ) {
	byte	*palette;
	byte	*pic8;
	int		i, c, p;
	byte	*pic32;
	if( !pic ) {
		fileSystem->ReadFile( filename, NULL, timestamp );
		return;	// just getting timestamp
	}
	LoadPCX( filename, &pic8, &palette, width, height, timestamp );
	if( !pic8 ) {
		*pic = NULL;
		return;
	}
	c = ( *width ) * ( *height );
	pic32 = *pic = ( byte * )R_StaticAlloc( 4 * c );
	for( i = 0 ; i < c ; i++ ) {
		p = pic8[i];
		pic32[0] = palette[p * 3];
		pic32[1] = palette[p * 3 + 1];
		pic32[2] = palette[p * 3 + 2];
		pic32[3] = 255;
		pic32 += 4;
	}
	R_StaticFree( pic8 );
	R_StaticFree( palette );
}

/*
=========================================================

TARGA LOADING

=========================================================
*/

/*
=============
LoadTGA
=============
*/
static void LoadTGA( const char *name, byte **pic, int *width, int *height, ID_TIME_T *timestamp ) {
	int			w, h, x, y, len, realrow, truerow, baserow, i, temp1, temp2, pixel_size, map_idx;
	int			RLE_count, RLE_flag, size, interleave, origin;
	bool		mapped, rlencoded;
	byte		*data, *dst, r, g, b, a, j, k, l, *ColorMap;
	byte		*buf_p;
	byte		*buffer;
	TargaHeader	header;
	if( !pic ) {
		fileSystem->ReadFile( name, NULL, timestamp );
		return;	// just getting timestamp
	}
	*pic = NULL;
	//
	// load the file
	//
	len = fileSystem->ReadFile( name, ( void ** )&buffer, timestamp );
	if( !buffer || len <= 0 ) {
		return;
	}
	buf_p = buffer;
	header.id_length = *buf_p++;
	header.colormap_type = *buf_p++;
	header.image_type = *buf_p++;
	header.colormap_index = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.colormap_length = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.colormap_size = *buf_p++;
	header.x_origin = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.y_origin = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.width = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.height = LittleShort( *( short * )buf_p );
	buf_p += 2;
	header.pixel_size = *buf_p++;
	header.attributes = *buf_p++;
	if( header.id_length != 0 ) {
		buf_p += header.id_length;
	}
	/* validate TGA type */
	switch( header.image_type ) {
	case TGA_Map:
	case TGA_RGB:
	case TGA_Mono:
	case TGA_RLEMap:
	case TGA_RLERGB:
	case TGA_RLEMono:
		break;
	default:
		common->Printf( "%s : Only type 1 (map), 2 (RGB), 3 (mono), 9 (RLEmap), 10 (RLERGB), 11 (RLEmono) TGA images supported\n", name );
		return;
	}
	/* validate color depth */
	switch( header.pixel_size ) {
	case 8:
	case 15:
	case 16:
	case 24:
	case 32:
		break;
	default:
		common->Printf( "%s : Only 8, 15, 16, 24 or 32 bit images (with colormaps) supported\n", name );
		return;
	}
	r = g = b = a = l = 0;
	/* if required, read the color map information. */
	ColorMap = NULL;
	mapped = ( header.image_type == TGA_Map || header.image_type == TGA_RLEMap ) && header.colormap_type == 1;
	if( mapped ) {
		/* validate colormap size */
		switch( header.colormap_size ) {
		case 8:
		case 15:
		case 16:
		case 32:
		case 24:
			break;
		default:
			common->Printf( "%s : Only 8, 15, 16, 24 or 32 bit colormaps supported\n", name );
			return;
		}
		temp1 = header.colormap_index;
		temp2 = header.colormap_length;
		if( ( temp1 + temp2 + 1 ) >= TGA_MAXCOLORS ) {
			return;
		}
		ColorMap = ( byte * ) R_StaticAlloc( TGA_MAXCOLORS * 4 );
		map_idx = 0;
		for( i = temp1 ; i < temp1 + temp2 ; ++i, map_idx += 4 ) {
			/* read appropriate number of bytes, break into rgb & put in map. */
			switch( header.colormap_size ) {
			case 8:	/* grey scale, read and triplicate. */
				r = g = b = *buf_p++;
				a = 255;
				break;
			case 15:	/* 5 bits each of red green and blue. */
				/* watch byte order. */
				j = *buf_p++;
				k = *buf_p++;
				l = ( ( unsigned int ) k << 8 ) + j;
				r = ( byte )( ( ( k & 0x7C ) >> 2 ) << 3 );
				g = ( byte )( ( ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 ) ) << 3 );
				b = ( byte )( ( j & 0x1F ) << 3 );
				a = 255;
				break;
			case 16:	/* 5 bits each of red green and blue, 1 alpha bit. */
				/* watch byte order. */
				j = *buf_p++;
				k = *buf_p++;
				l = ( ( unsigned int ) k << 8 ) + j;
				r = ( byte )( ( ( k & 0x7C ) >> 2 ) << 3 );
				g = ( byte )( ( ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 ) ) << 3 );
				b = ( byte )( ( j & 0x1F ) << 3 );
				a = ( k & 0x80 ) ? 255 : 0;
				break;
			case 24:	/* 8 bits each of blue, green and red. */
				b = *buf_p++;
				g = *buf_p++;
				r = *buf_p++;
				a = 255;
				l = 0;
				break;
			case 32:	/* 8 bits each of blue, green, red and alpha. */
				b = *buf_p++;
				g = *buf_p++;
				r = *buf_p++;
				a = *buf_p++;
				l = 0;
				break;
			}
			ColorMap[map_idx + 0] = r;
			ColorMap[map_idx + 1] = g;
			ColorMap[map_idx + 2] = b;
			ColorMap[map_idx + 3] = a;
		}
	}
	/* check run-length encoding. */
	rlencoded = ( header.image_type == TGA_RLEMap || header.image_type == TGA_RLERGB || header.image_type == TGA_RLEMono );
	RLE_count = RLE_flag = 0;
	w = header.width;
	h = header.height;
	size = w * h * 4;
	if( width ) {
		*width = w;
	}
	if( height ) {
		*height = h;
	}
	data = ( byte * ) R_StaticAlloc( size );
	*pic = data;
	/* read the Targa file body and convert to portable format. */
	pixel_size = header.pixel_size;
	origin = ( header.attributes & 0x20 ) >> 5;
	interleave = ( header.attributes & 0xC0 ) >> 6;
	truerow = 0;
	baserow = 0;
	for( y = 0 ; y < h ; y++ ) {
		realrow = truerow;
		if( origin == TGA_O_UPPER )	{
			realrow = h - realrow - 1;
		}
		dst = data + realrow * w * 4;
		for( x = 0 ; x < w ; x++ ) {
			/* check if run length encoded. */
			if( rlencoded )	{
				if( !RLE_count ) {
					/* have to restart run. */
					i = *buf_p++;
					RLE_flag = ( i & 0x80 );
					if( !RLE_flag )	{
						// stream of unencoded pixels
						RLE_count = i + 1;
					} else {
						// single pixel replicated
						RLE_count = i - 127;
					}
					/* decrement count & get pixel. */
					--RLE_count;
				} else {
					/* have already read count & (at least) first pixel. */
					--RLE_count;
					if( RLE_flag ) {
						/* replicated pixels. */
						goto PixEncode;
					}
				}
			}
			/* read appropriate number of bytes, break into RGB. */
			switch( pixel_size ) {
			case 8:	/* grey scale, read and triplicate. */
				r = g = b = l = *buf_p++;
				a = 255;
				break;
			case 15:	/* 5 bits each of red green and blue. */
				/* watch byte order. */
				j = *buf_p++;
				k = *buf_p++;
				l = ( ( unsigned int ) k << 8 ) + j;
				r = ( byte )( ( ( k & 0x7C ) >> 2 ) << 3 );
				g = ( byte )( ( ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 ) ) << 3 );
				b = ( byte )( ( j & 0x1F ) << 3 );
				a = 255;
				break;
			case 16:	/* 5 bits each of red green and blue, 1 alpha bit. */
				/* watch byte order. */
				j = *buf_p++;
				k = *buf_p++;
				l = ( ( unsigned int ) k << 8 ) + j;
				r = ( byte )( ( ( k & 0x7C ) >> 2 ) << 3 );
				g = ( byte )( ( ( ( k & 0x03 ) << 3 ) + ( ( j & 0xE0 ) >> 5 ) ) << 3 );
				b = ( byte )( ( j & 0x1F ) << 3 );
				a = ( k & 0x80 ) ? 255 : 0;
				break;
			case 24:	/* 8 bits each of blue, green and red. */
				b = *buf_p++;
				g = *buf_p++;
				r = *buf_p++;
				a = 255;
				l = 0;
				break;
			case 32:	/* 8 bits each of blue, green, red and alpha. */
				b = *buf_p++;
				g = *buf_p++;
				r = *buf_p++;
				a = *buf_p++;
				l = 0;
				break;
			default:
				common->Printf( "%s : Illegal pixel_size '%d'\n", name, pixel_size );
				R_StaticFree( data );
				if( mapped ) {
					R_StaticFree( ColorMap );
				}
				return;
			}
PixEncode:
			if( mapped ) {
				map_idx = l * 4;
				*dst++ = ColorMap[map_idx + 0];
				*dst++ = ColorMap[map_idx + 1];
				*dst++ = ColorMap[map_idx + 2];
				*dst++ = ColorMap[map_idx + 3];
			} else {
				*dst++ = r;
				*dst++ = g;
				*dst++ = b;
				*dst++ = a;
			}
		}
		if( interleave == TGA_IL_Four )	{
			truerow += 4;
		} else if( interleave == TGA_IL_Two )	{
			truerow += 2;
		} else {
			truerow++;
		}
		if( truerow >= h ) {
			truerow = ++baserow;
		}
	}
	if( mapped ) {
		R_StaticFree( ColorMap );
	}
	fileSystem->FreeFile( buffer );
}

/*
=========================================================

JPG LOADING

Interfaces with the huge libjpeg
=========================================================
*/
/*
 * Include file for users of JPEG library.
 * You will need to have included system headers that define at least
 * the typedefs FILE and size_t before you can include jpeglib.h.
 * (stdio.h is sufficient on ANSI-conforming systems.)
 * You may also wish to include "jerror.h".
 */
#define JPEG_INTERNALS 1
#include "../libs/jpeg9a/jpeglib.h"

/*
* Include file for users of JPEG library.
* You will need to have included system headers that define at least
* the typedefs FILE and size_t before you can include jpeglib.h.
* (stdio.h is sufficient on ANSI-conforming systems.)
* You may also wish to include "jerror.h".
*/
METHODDEF( void )
jpg_Error( jpeg_common_struct *cinfo ) {
	char buffer[JMSG_LENGTH_MAX] = {0};
	( *cinfo->err->format_message )( cinfo, buffer );
	throw idException( buffer );
}

METHODDEF( void )
jpg_Printf( jpeg_common_struct *cinfo ) {
	char buffer[JMSG_LENGTH_MAX] = {0};
	( *cinfo->err->format_message )( cinfo, buffer );
	idLib::common->Printf( "%s\n", buffer );
}

/*
=============
LoadJPG
=============
*/
static void LoadJPG( const char *filename, unsigned char **pic, int *width, int *height, ID_TIME_T *timestamp ) {
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo = {NULL};
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	/* This struct represents a JPEG error handler.  It is declared separately
	 * because applications often want to supply a specialized error handler
	 * (see the second half of this file for an example).  But here we just
	 * take the easy way out and use the standard error handler, which will
	 * print a message on stderr and call exit() if compression fails.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct jpeg_error_mgr jerr;
	/* More stuff */
	JSAMPARRAY		buffer;		/* Output row buffer */
	unsigned int	row_stride;	/* physical row width in output buffer */
	unsigned int	pixelcount, memcount;
	unsigned int	sindex, dindex;
	byte			*out;
	int				len;
	union {
		byte *b;
		void *v;
	} fbuffer;
	byte			*buf;
	if( !pic ) {
		fileSystem->ReadFile( filename, NULL, timestamp );
		return;	// just getting timestamp
	}
	*pic = NULL;
	/* In this example we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */
	len = fileSystem->ReadFile( filename, &fbuffer.v, timestamp );
	if( !fbuffer.b || len <= 0 ) {
		return;
	}
	/* Step 1: allocate and initialize JPEG decompression object */
	/* We have to set up the error handler first, in case the initialization
	 * step fails.  (Unlikely, but it could happen if you are out of memory.)
	 * This routine fills in the contents of struct jerr, and returns jerr's
	 * address which we place into the link field in cinfo.
	 */
	cinfo.err = jpeg_std_error( &jerr );
	cinfo.err->error_exit = jpg_Error;
	cinfo.err->output_message = jpg_Printf;
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress( &cinfo );
	/* Step 2: specify data source (eg, a file) */
	jpeg_mem_src( &cinfo, fbuffer.b, len );
	/* Step 3: read file parameters with jpeg_read_header() */
	( void ) jpeg_read_header( &cinfo, TRUE );
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */
	/* Step 4: set parameters for decompression */
	/*
	 * Make sure it always converts images to RGB color space. This will
	 * automatically convert 8-bit greyscale images to RGB as well.
	 */
	cinfo.out_color_space = JCS_RGB;
	/* Step 5: Start decompressor */
	( void ) jpeg_start_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */
	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */
	/* JSAMPLEs per row in output buffer */
	pixelcount = cinfo.output_width * cinfo.output_height;
	if( !cinfo.output_width || !cinfo.output_height || ( ( pixelcount * 4 ) / cinfo.output_width ) / 4 != cinfo.output_height || pixelcount > 0x1FFFFFFF || cinfo.output_components != 3 ) {
		// Free the memory to make sure we don't leak memory
		fileSystem->FreeFile( fbuffer.v );
		jpeg_destroy_decompress( &cinfo );
		common->FatalError( "LoadJPG: %s has an invalid image format: %dx%d*4=%d, components: %d", filename, cinfo.output_width, cinfo.output_height, pixelcount * 4, cinfo.output_components );
	}
	memcount = pixelcount * 4;
	row_stride = cinfo.output_width * cinfo.output_components;
	out = ( byte * ) R_StaticAlloc( memcount );
	*width = cinfo.output_width;
	*height = cinfo.output_height;
	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */
	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while( cinfo.output_scanline < cinfo.output_height ) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		buf = ( ( out + ( row_stride * cinfo.output_scanline ) ) );
		buffer = &buf;
		( void ) jpeg_read_scanlines( &cinfo, buffer, 1 );
	}
	buf = out;
	// Expand from RGB to RGBA
	sindex = pixelcount * cinfo.output_components;
	dindex = memcount;
	do {
		buf[--dindex] = 255;
		buf[--dindex] = buf[--sindex];
		buf[--dindex] = buf[--sindex];
		buf[--dindex] = buf[--sindex];
	} while( sindex );
	*pic = out;
	/* Step 7: Finish decompression */
	jpeg_finish_decompress( &cinfo );
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */
	/* Step 8: Release JPEG decompression object */
	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress( &cinfo );
	/* After finish_decompress, we can close the input file.
	 * Here we postpone it until after no more JPEG errors are possible,
	 * so as to simplify the setjmp error logic above.  (Actually, I don't
	 * think that jpeg_destroy can do an error exit, but why assume anything...)
	 */
	fileSystem->FreeFile( fbuffer.v );
	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */
	/* And we're done! */
}

//===================================================================

/*
=================
R_LoadImage

Loads any of the supported image types into a cannonical
32 bit format.

Automatically attempts to load .jpg files if .tga files fail to load.

*pic will be NULL if the load failed.

Anything that is going to make this into a texture would use
makePowerOf2 = true, but something loading an image as a lookup
table of some sort would leave it in identity form.

It is important to do this at image load time instead of texture load
time for bump maps.

Timestamp may be NULL if the value is going to be ignored

If pic is NULL, the image won't actually be loaded, it will just find the
timestamp.
=================
*/
void R_LoadImage( const char *cname, byte **pic, int *width, int *height, ID_TIME_T *timestamp, bool makePowerOf2 ) {
	idStr name = cname;
	if( pic ) {
		*pic = NULL;
	}
	if( timestamp ) {
		*timestamp = 0xFFFFFFFF;
	}
	if( width ) {
		*width = 0;
	}
	if( height ) {
		*height = 0;
	}
	name.DefaultFileExtension( ".tga" );
	if( name.Length() < 5 ) {
		return;
	}
	name.ToLower();
	idStr ext;
	name.ExtractFileExtension( ext );
	if( ext == "tga" ) {
		LoadTGA( name.c_str(), pic, width, height, timestamp );            // try tga first
		if( ( pic && *pic == 0 ) || ( timestamp && *timestamp == -1 ) ) {
			name.StripFileExtension();
			name.DefaultFileExtension( ".jpg" );
			LoadJPG( name.c_str(), pic, width, height, timestamp );
		}
	} else if( ext == "pcx" ) {
		LoadPCX32( name.c_str(), pic, width, height, timestamp );
	} else if( ext == "bmp" ) {
		LoadBMP( name.c_str(), pic, width, height, timestamp );
	} else if( ext == "jpg" ) {
		LoadJPG( name.c_str(), pic, width, height, timestamp );
	}
	if( ( width && *width < 1 ) || ( height && *height < 1 ) ) {
		if( pic && *pic ) {
			R_StaticFree( *pic );
			*pic = 0;
		}
	}
	//
	// convert to exact power of 2 sizes
	//
	if( pic && *pic && makePowerOf2 ) {
		int		w, h;
		int		scaled_width, scaled_height;
		byte	*pic_p;
		w = *width;
		h = *height;
		IMAGE_ROUND_POWER2( w, scaled_width );
		IMAGE_ROUND_POWER2( h, scaled_height );
		if( scaled_width != w || scaled_height != h ) {
			if( globalImages->image_roundDown.GetBool() && scaled_width > w ) {
				scaled_width >>= 1;
			}
			if( globalImages->image_roundDown.GetBool() && scaled_height > h ) {
				scaled_height >>= 1;
			}
			pic_p = ( byte* )R_StaticAlloc( scaled_width * scaled_height * 4 );
			R_ResampleTexture( *pic, w, h, pic_p, scaled_width, scaled_height );
			R_StaticFree( *pic );
			*pic = pic_p;
			*width = scaled_width;
			*height = scaled_height;
		}
	}
}

/*
=======================
R_LoadCubeImages

Loads six files with proper extensions
=======================
*/
bool R_LoadCubeImages( const char *imgName, cubeFiles_t extensions, byte *pics[6], int *outSize, ID_TIME_T *timestamp ) {
	int		i, j;
	const char	*cameraSides[6] =  { "_forward.tga", "_back.tga", "_left.tga", "_right.tga",
									 "_up.tga", "_down.tga"
								  };
	const char	*axisSides[6] =  { "_px.tga", "_nx.tga", "_py.tga", "_ny.tga",
								   "_pz.tga", "_nz.tga"
								};
	const char	**sides;
	char		fullName[MAX_IMAGE_NAME];
	int			width, height, size = 0;
	if( extensions == CF_CAMERA ) {
		sides = cameraSides;
	} else {
		sides = axisSides;
	}
	// FIXME: precompressed cube map files
	if( pics ) {
		memset( pics, 0, 6 * sizeof( pics[0] ) );
	}
	if( timestamp ) {
		*timestamp = 0;
	}
	for( i = 0 ; i < 6 ; i++ ) {
		idStr::snPrintf( fullName, sizeof( fullName ), "%s%s", imgName, sides[i] );
		ID_TIME_T thisTime;
		if( !pics ) {
			// just checking timestamps
			R_LoadImageProgram( fullName, NULL, &width, &height, &thisTime );
		} else {
			R_LoadImageProgram( fullName, &pics[i], &width, &height, &thisTime );
		}
		if( thisTime == FILE_NOT_FOUND_TIMESTAMP ) {
			break;
		}
		if( i == 0 ) {
			size = width;
		}
		if( width != size || height != size ) {
			common->Warning( "Mismatched sizes on cube map '%s'", imgName );
			break;
		}
		if( timestamp ) {
			if( thisTime > *timestamp ) {
				*timestamp = thisTime;
			}
		}
		if( pics && extensions == CF_CAMERA ) {
			// convert from "camera" images to native cube map images
			switch( i ) {
			case 0:	// forward
				R_RotatePic( pics[i], width );
				break;
			case 1:	// back
				R_RotatePic( pics[i], width );
				R_HorizontalFlip( pics[i], width, height );
				R_VerticalFlip( pics[i], width, height );
				break;
			case 2:	// left
				R_VerticalFlip( pics[i], width, height );
				break;
			case 3:	// right
				R_HorizontalFlip( pics[i], width, height );
				break;
			case 4:	// up
				R_RotatePic( pics[i], width );
				break;
			case 5: // down
				R_RotatePic( pics[i], width );
				break;
			}
		}
	}
	if( i != 6 ) {
		// we had an error, so free everything
		if( pics ) {
			for( j = 0 ; j < i ; j++ ) {
				R_StaticFree( pics[j] );
			}
		}
		if( timestamp ) {
			*timestamp = 0;
		}
		return false;
	}
	if( outSize ) {
		*outSize = size;
	}
	return true;
}
