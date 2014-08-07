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

//====================================================================

/*
======================
idScreenRect::Clear
======================
*/
void idScreenRect::Clear() {
	x1 = y1 = 32000;
	x2 = y2 = -32000;
	zmin = 0.0f;
	zmax = 1.0f;
}

/*
======================
idScreenRect::AddPoint
======================
*/
void idScreenRect::AddPoint( float x, float y ) {
	int	ix = idMath::FtoiFast( x );
	int iy = idMath::FtoiFast( y );
	if( ix < x1 ) {
		x1 = ix;
	}
	if( ix > x2 ) {
		x2 = ix;
	}
	if( iy < y1 ) {
		y1 = iy;
	}
	if( iy > y2 ) {
		y2 = iy;
	}
}

/*
======================
idScreenRect::Expand
======================
*/
void idScreenRect::Expand() {
	x1--;
	y1--;
	x2++;
	y2++;
}

/*
======================
idScreenRect::Intersect
======================
*/
void idScreenRect::Intersect( const idScreenRect &rect ) {
	if( rect.x1 > x1 ) {
		x1 = rect.x1;
	}
	if( rect.x2 < x2 ) {
		x2 = rect.x2;
	}
	if( rect.y1 > y1 ) {
		y1 = rect.y1;
	}
	if( rect.y2 < y2 ) {
		y2 = rect.y2;
	}
}

/*
======================
idScreenRect::Union
======================
*/
void idScreenRect::Union( const idScreenRect &rect ) {
	if( rect.x1 < x1 ) {
		x1 = rect.x1;
	}
	if( rect.x2 > x2 ) {
		x2 = rect.x2;
	}
	if( rect.y1 < y1 ) {
		y1 = rect.y1;
	}
	if( rect.y2 > y2 ) {
		y2 = rect.y2;
	}
}

/*
======================
idScreenRect::Equals
======================
*/
bool idScreenRect::Equals( const idScreenRect &rect ) const {
	return ( x1 == rect.x1 && x2 == rect.x2 && y1 == rect.y1 && y2 == rect.y2 );
}

/*
======================
idScreenRect::IsEmpty
======================
*/
bool idScreenRect::IsEmpty() const {
	return ( x1 > x2 || y1 > y2 );
}

/*
======================
R_ScreenRectFromViewFrustumBounds
======================
*/
idScreenRect R_ScreenRectFromViewFrustumBounds( const idBounds &bounds ) {
	idScreenRect screenRect;
	screenRect.Clear();
	screenRect.x1 = idMath::FtoiFast( 0.5f * ( 1.0f - bounds[1].y ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 ) );
	screenRect.x2 = idMath::FtoiFast( 0.5f * ( 1.0f - bounds[0].y ) * ( tr.viewDef->viewport.x2 - tr.viewDef->viewport.x1 ) );
	screenRect.y1 = idMath::FtoiFast( 0.5f * ( 1.0f + bounds[0].z ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 ) );
	screenRect.y2 = idMath::FtoiFast( 0.5f * ( 1.0f + bounds[1].z ) * ( tr.viewDef->viewport.y2 - tr.viewDef->viewport.y1 ) );
	if( r_useDepthBoundsTest.GetInteger() ) {
		R_TransformEyeZToWin( -bounds[0].x, tr.viewDef->projectionMatrix, screenRect.zmin );
		R_TransformEyeZToWin( -bounds[1].x, tr.viewDef->projectionMatrix, screenRect.zmax );
	}
	return screenRect;
}

/*
======================
R_ShowColoredScreenRect
======================
*/
void R_ShowColoredScreenRect( const idScreenRect &rect, int colorIndex ) {
	if( !rect.IsEmpty() ) {
		static idVec4 colors[] = { colorRed, colorGreen, colorBlue, colorYellow, colorMagenta, colorCyan, colorWhite, colorPurple };
		tr.viewDef->renderWorld->DebugScreenRect( colors[colorIndex & 7], rect, tr.viewDef );
	}
}

/*
====================
R_ToggleSmpFrame
====================
*/
void R_ToggleSmpFrame( void ) {
	if( r_lockSurfaces.GetBool() ) {
		return;
	}
	R_FreeDeferredTriSurfs( frameData );
	// clear frame-temporary data
	frameData_t		*frame;
	frameMemoryBlock_t	*block;
	// update the highwater mark
	R_CountFrameData();
	frame = frameData;
	// reset the memory allocation to the first block
	frame->alloc = frame->memory;
	// clear all the blocks
	for( block = frame->memory ; block ; block = block->next ) {
		block->used = 0;
	}
	R_ClearCommandChain();
}


//=====================================================

#define	MEMORY_BLOCK_SIZE	0x100000

/*
=====================
R_ShutdownFrameData
=====================
*/
void R_ShutdownFrameData( void ) {
	frameData_t *frame;
	frameMemoryBlock_t *block;
	// free any current data
	frame = frameData;
	if( !frame ) {
		return;
	}
	R_FreeDeferredTriSurfs( frame );
	frameMemoryBlock_t *nextBlock;
	for( block = frame->memory ; block ; block = nextBlock ) {
		nextBlock = block->next;
		Mem_Free( block );
	}
	Mem_Free( frame );
	frameData = NULL;
}

/*
=====================
R_InitFrameData
=====================
*/
void R_InitFrameData( void ) {
	int size;
	frameData_t *frame;
	frameMemoryBlock_t *block;
	R_ShutdownFrameData();
	frameData = ( frameData_t * )Mem_ClearedAlloc( sizeof( *frameData ) );
	frame = frameData;
	size = MEMORY_BLOCK_SIZE;
	block = ( frameMemoryBlock_t * )Mem_Alloc( size + sizeof( *block ) );
	if( !block ) {
		common->FatalError( "R_InitFrameData: Mem_Alloc() failed" );
	}
	block->size = size;
	block->used = 0;
	block->next = NULL;
	frame->memory = block;
	frame->memoryHighwater = 0;
	R_ToggleSmpFrame();
}

/*
================
R_CountFrameData
================
*/
int R_CountFrameData( void ) {
	frameData_t		*frame;
	frameMemoryBlock_t	*block;
	int				count;
	count = 0;
	frame = frameData;
	for( block = frame->memory ; block ; block = block->next ) {
		count += block->used;
		if( block == frame->alloc ) {
			break;
		}
	}
	// note if this is a new highwater mark
	if( count > frame->memoryHighwater ) {
		frame->memoryHighwater = count;
	}
	return count;
}

/*
=================
R_StaticAlloc
=================
*/
void *R_StaticAlloc( int bytes ) {
	void	*buf;
	tr.pc.c_alloc++;
	tr.staticAllocCount += bytes;
	buf = Mem_Alloc( bytes );
	// don't exit on failure on zero length allocations since the old code didn't
	if( !buf && ( bytes != 0 ) ) {
		common->FatalError( "R_StaticAlloc failed on %i bytes", bytes );
	}
	return buf;
}

/*
=================
R_ClearedStaticAlloc
=================
*/
void *R_ClearedStaticAlloc( int bytes ) {
	void	*buf;
	buf = R_StaticAlloc( bytes );
	SIMDProcessor->Memset( buf, 0, bytes );
	return buf;
}

/*
=================
R_StaticFree
=================
*/
void R_StaticFree( void *data ) {
	tr.pc.c_free++;
	Mem_Free( data );
}

/*
================
R_FrameAlloc

This data will be automatically freed when the
current frame's back end completes.

This should only be called by the front end.  The
back end shouldn't need to allocate memory.

If we passed smpFrame in, the back end could
alloc memory, because it will always be a
different frameData than the front end is using.

All temporary data, like dynamic tesselations
and local spaces are allocated here.

The memory will not move, but it may not be
contiguous with previous allocations even
from this frame.

The memory is NOT zero filled.
Should part of this be inlined in a macro?
================
*/
void *R_FrameAlloc( int bytes ) {
	frameData_t		*frame;
	frameMemoryBlock_t	*block;
	void			*buf;
	bytes = ( bytes + 16 )&~15;
	// see if it can be satisfied in the current block
	frame = frameData;
	block = frame->alloc;
	if( block->size - block->used >= bytes ) {
		buf = block->base + block->used;
		block->used += bytes;
		return buf;
	}
	// advance to the next memory block if available
	block = block->next;
	// create a new block if we are at the end of
	// the chain
	if( !block ) {
		int		size;
		size = MEMORY_BLOCK_SIZE;
		block = ( frameMemoryBlock_t * )Mem_Alloc( size + sizeof( *block ) );
		if( !block ) {
			common->FatalError( "R_FrameAlloc: Mem_Alloc() failed" );
		}
		block->size = size;
		block->used = 0;
		block->next = NULL;
		frame->alloc->next = block;
	}
	// we could fix this if we needed to...
	if( bytes > block->size ) {
		common->FatalError( "R_FrameAlloc of %i exceeded MEMORY_BLOCK_SIZE",
							bytes );
	}
	frame->alloc = block;
	block->used = bytes;
	return block->base;
}

/*
==================
R_ClearedFrameAlloc
==================
*/
void *R_ClearedFrameAlloc( int bytes ) {
	void	*r;
	r = R_FrameAlloc( bytes );
	SIMDProcessor->Memset( r, 0, bytes );
	return r;
}

//==========================================================================

void R_AxisToModelMatrix( const idMat3 &axis, const idVec3 &origin, float modelMatrix[16] ) {
	modelMatrix[0 * 4 + 0] = axis[0][0];
	modelMatrix[1 * 4 + 0] = axis[1][0];
	modelMatrix[2 * 4 + 0] = axis[2][0];
	modelMatrix[3 * 4 + 0] = origin[0];

	modelMatrix[0 * 4 + 1] = axis[0][1];
	modelMatrix[1 * 4 + 1] = axis[1][1];
	modelMatrix[2 * 4 + 1] = axis[2][1];
	modelMatrix[3 * 4 + 1] = origin[1];

	modelMatrix[0 * 4 + 2] = axis[0][2];
	modelMatrix[1 * 4 + 2] = axis[1][2];
	modelMatrix[2 * 4 + 2] = axis[2][2];
	modelMatrix[3 * 4 + 2] = origin[2];

	modelMatrix[0 * 4 + 3] = 0.0f;
	modelMatrix[1 * 4 + 3] = 0.0f;
	modelMatrix[2 * 4 + 3] = 0.0f;
	modelMatrix[3 * 4 + 3] = 1.0f;
}

// FIXME: these assume no skewing or scaling transforms
void R_LocalPointToGlobal (const float modelMatrix[16], const idVec3 &in, idVec3 &out)
{
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0] + modelMatrix[3 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1] + modelMatrix[3 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2] + modelMatrix[3 * 4 + 2];
}

void R_PointTimesMatrix( const float modelMatrix[16], const idVec4 &in, idVec4 &out ) {
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0] + modelMatrix[3 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1] + modelMatrix[3 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2] + modelMatrix[3 * 4 + 2];
	out[3] = in[0] * modelMatrix[0 * 4 + 3] + in[1] * modelMatrix[1 * 4 + 3] + in[2] * modelMatrix[2 * 4 + 3] + modelMatrix[3 * 4 + 3];
}

void R_GlobalPointToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out ) {
	idVec3 temp;

	temp[0] = in[0] - modelMatrix[3 * 4 + 0];
	temp[1] = in[1] - modelMatrix[3 * 4 + 1];
	temp[2] = in[2] - modelMatrix[3 * 4 + 2];

	out[0] = temp[0] * modelMatrix[0 * 4 + 0] + temp[1] * modelMatrix[0 * 4 + 1] + temp[2] * modelMatrix[0 * 4 + 2];
	out[1] = temp[0] * modelMatrix[1 * 4 + 0] + temp[1] * modelMatrix[1 * 4 + 1] + temp[2] * modelMatrix[1 * 4 + 2];
	out[2] = temp[0] * modelMatrix[2 * 4 + 0] + temp[1] * modelMatrix[2 * 4 + 1] + temp[2] * modelMatrix[2 * 4 + 2];
}

void R_LocalVectorToGlobal( const float modelMatrix[16], const idVec3 &in, idVec3 &out ) {
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2];
}

void R_GlobalVectorToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out ) {
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[0 * 4 + 1] + in[2] * modelMatrix[0 * 4 + 2];
	out[1] = in[0] * modelMatrix[1 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[1 * 4 + 2];
	out[2] = in[0] * modelMatrix[2 * 4 + 0] + in[1] * modelMatrix[2 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 2];
}

void R_GlobalPlaneToLocal( const float modelMatrix[16], const idPlane &in, idPlane &out ) {
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[0 * 4 + 1] + in[2] * modelMatrix[0 * 4 + 2];
	out[1] = in[0] * modelMatrix[1 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[1 * 4 + 2];
	out[2] = in[0] * modelMatrix[2 * 4 + 0] + in[1] * modelMatrix[2 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 2];
	out[3] = in[0] * modelMatrix[3 * 4 + 0] + in[1] * modelMatrix[3 * 4 + 1] + in[2] * modelMatrix[3 * 4 + 2] + in[3];
}

void R_LocalPlaneToGlobal( const float modelMatrix[16], const idPlane &in, idPlane &out ) {
	out[0] = in[0] * modelMatrix[0 * 4 + 0] + in[1] * modelMatrix[1 * 4 + 0] + in[2] * modelMatrix[2 * 4 + 0];
	out[1] = in[0] * modelMatrix[0 * 4 + 1] + in[1] * modelMatrix[1 * 4 + 1] + in[2] * modelMatrix[2 * 4 + 1];
	out[2] = in[0] * modelMatrix[0 * 4 + 2] + in[1] * modelMatrix[1 * 4 + 2] + in[2] * modelMatrix[2 * 4 + 2];
	out[3] = in[3] - modelMatrix[3 * 4 + 0] * out[0] - modelMatrix[3 * 4 + 1] * out[1] - modelMatrix[3 * 4 + 2] * out[2];
}

// transform Z in eye coordinates to window coordinates
void R_TransformEyeZToWin( float src_z, const float *projectionMatrix, float &dst_z ) {
	float clip_z, clip_w;
	// projection
	clip_z = src_z * projectionMatrix[ 2 + 2 * 4 ] + projectionMatrix[ 2 + 3 * 4 ];
	clip_w = src_z * projectionMatrix[ 3 + 2 * 4 ] + projectionMatrix[ 3 + 3 * 4 ];
	if( clip_w <= 0.0f ) {
		dst_z = 0.0f;					// clamp to near plane
	} else {
		dst_z = clip_z / clip_w;
		dst_z = dst_z * 0.5f + 0.5f;	// convert to window coords
	}
}

/*
=================
R_RadiusCullLocalBox

A fast, conservative center-to-corner culling test
Returns true if the box is outside the given global frustum, (positive sides are out)
=================
*/
bool R_RadiusCullLocalBox( const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes ) {
	idVec3			worldOrigin;
	float			worldRadius;
	const idPlane	*frust;
	if( r_useCulling.GetInteger() == 0 ) {
		return false;
	}
	// transform the surface bounds into world space
	idVec3	localOrigin = ( bounds[0] + bounds[1] ) * 0.5;
	R_LocalPointToGlobal( modelMatrix, localOrigin, worldOrigin );
	worldRadius = ( bounds[0] - localOrigin ).Length();	// FIXME: won't be correct for scaled objects
	for( int i = 0 ; i < numPlanes ; i++ ) {
		frust = planes + i;
		float d = frust->Distance( worldOrigin );
		if( d > worldRadius ) {
			return true;	// culled
		}
	}
	return false;		// no culled
}

/*
=================
R_CornerCullLocalBox

Tests all corners against the frustum.
Can still generate a few false positives when the box is outside a corner.
Returns true if the box is outside the given global frustum, (positive sides are out)
=================
*/
bool R_CornerCullLocalBox( const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes ) {
	int			i, j;
	idVec3		transformed[8];
	float		dists[8];
	idVec3		v;
	const idPlane *frust;
	// we can disable box culling for experimental timing purposes
	if( r_useCulling.GetInteger() < 2 ) {
		return false;
	}
	// transform into world space
	for( i = 0 ; i < 8 ; i++ ) {
		v[0] = bounds[i & 1][0];
		v[1] = bounds[( i >> 1 ) & 1][1];
		v[2] = bounds[( i >> 2 ) & 1][2];
		R_LocalPointToGlobal( modelMatrix, v, transformed[i] );
	}
	// check against frustum planes
	for( i = 0 ; i < numPlanes ; i++ ) {
		frust = planes + i;
		for( j = 0 ; j < 8 ; j++ ) {
			dists[j] = frust->Distance( transformed[j] );
			if( dists[j] < 0 ) {
				break;
			}
		}
		if( j == 8 ) {
			// all points were behind one of the planes
			tr.pc.c_box_cull_out++;
			return true;
		}
	}
	tr.pc.c_box_cull_in++;
	return false;		// not culled
}

/*
=================
R_CullLocalBox

Performs quick test before expensive test
Returns true if the box is outside the given global frustum, (positive sides are out)
=================
*/
bool R_CullLocalBox( const idBounds &bounds, const float modelMatrix[16], int numPlanes, const idPlane *planes ) {
	if( R_RadiusCullLocalBox( bounds, modelMatrix, numPlanes, planes ) ) {
		return true;
	}
	return R_CornerCullLocalBox( bounds, modelMatrix, numPlanes, planes );
}

/*
==========================
R_TransformModelToClip
==========================
*/
void R_TransformModelToClip( const idVec3 &src, const float *modelMatrix, const float *projectionMatrix, idPlane &eye, idPlane &dst ) {
	for( int i = 0; i < 4; i++ ) {
		eye[i] = 	modelMatrix[i + 0 * 4] * src[0] +
					modelMatrix[i + 1 * 4] * src[1] +
					modelMatrix[i + 2 * 4] * src[2] +
					modelMatrix[i + 3 * 4];
	}
	for( int i = 0; i < 4; i++ ) {
		dst[i] = 	projectionMatrix[i + 0 * 4] * eye[0] +
					projectionMatrix[i + 1 * 4] * eye[1] +
					projectionMatrix[i + 2 * 4] * eye[2] +
					projectionMatrix[i + 3 * 4] * eye[3];
	}
}

/*
==========================
R_GlobalToNormalizedDeviceCoordinates

-1 to 1 range in x, y, and z
==========================
*/
void R_GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc ) {
	idPlane	view;
	idPlane	clip;
	// _D3XP use tr.primaryView when there is no tr.viewDef
	const viewDef_t* viewDef = ( tr.viewDef != NULL ) ? tr.viewDef : tr.primaryView;
	for( int i = 0; i < 4; i ++ ) {
		view[i] = 	viewDef->worldSpace.modelViewMatrix[i + 0 * 4] * global[0] +
					viewDef->worldSpace.modelViewMatrix[i + 1 * 4] * global[1] +
					viewDef->worldSpace.modelViewMatrix[i + 2 * 4] * global[2] +
					viewDef->worldSpace.modelViewMatrix[i + 3 * 4];
	}
	for( int i = 0; i < 4; i ++ ) {
		clip[i] = 	viewDef->projectionMatrix[i + 0 * 4] * view[0] +
					viewDef->projectionMatrix[i + 1 * 4] * view[1] +
					viewDef->projectionMatrix[i + 2 * 4] * view[2] +
					viewDef->projectionMatrix[i + 3 * 4] * view[3];
	}
	const float invW = 1.0f / clip[3];
	ndc[0] = clip[0] * invW;
	ndc[1] = clip[1] * invW;
	ndc[2] = clip[2] * invW;		// NOTE: in D3D this is in the range [0,1]
}

/*
==========================
R_TransformClipToDevice

Clip to normalized device coordinates
==========================
*/
void R_TransformClipToDevice( const idPlane &clip, idVec3 &ndc ) {
	const float invW = 1.0f / clip[3];
	ndc[0] = clip[0] * invW;
	ndc[1] = clip[1] * invW;
	ndc[2] = clip[2] * invW;		// NOTE: in D3D this is in the range [0,1]
}

/*
==========================
R_MatrixMultiply
==========================
*/
void R_MatrixMultiply( const float a[16], const float b[16], float out[16] ) {
	__m128 a0 = _mm_loadu_ps( a + 0 * 4 );
	__m128 a1 = _mm_loadu_ps( a + 1 * 4 );
	__m128 a2 = _mm_loadu_ps( a + 2 * 4 );
	__m128 a3 = _mm_loadu_ps( a + 3 * 4 );

	__m128 b0 = _mm_loadu_ps( b + 0 * 4 );
	__m128 b1 = _mm_loadu_ps( b + 1 * 4 );
	__m128 b2 = _mm_loadu_ps( b + 2 * 4 );
	__m128 b3 = _mm_loadu_ps( b + 3 * 4 );

	__m128 t0 = _mm_mul_ps( _mm_splat_ps( a0, 0 ), b0 );
	__m128 t1 = _mm_mul_ps( _mm_splat_ps( a1, 0 ), b0 );
	__m128 t2 = _mm_mul_ps( _mm_splat_ps( a2, 0 ), b0 );
	__m128 t3 = _mm_mul_ps( _mm_splat_ps( a3, 0 ), b0 );

	t0 = _mm_add_ps( t0, _mm_mul_ps( _mm_splat_ps( a0, 1 ), b1 ) );
	t1 = _mm_add_ps( t1, _mm_mul_ps( _mm_splat_ps( a1, 1 ), b1 ) );
	t2 = _mm_add_ps( t2, _mm_mul_ps( _mm_splat_ps( a2, 1 ), b1 ) );
	t3 = _mm_add_ps( t3, _mm_mul_ps( _mm_splat_ps( a3, 1 ), b1 ) );

	t0 = _mm_add_ps( t0, _mm_mul_ps( _mm_splat_ps( a0, 2 ), b2 ) );
	t1 = _mm_add_ps( t1, _mm_mul_ps( _mm_splat_ps( a1, 2 ), b2 ) );
	t2 = _mm_add_ps( t2, _mm_mul_ps( _mm_splat_ps( a2, 2 ), b2 ) );
	t3 = _mm_add_ps( t3, _mm_mul_ps( _mm_splat_ps( a3, 2 ), b2 ) );

	t0 = _mm_add_ps( t0, _mm_mul_ps( _mm_splat_ps( a0, 3 ), b3 ) );
	t1 = _mm_add_ps( t1, _mm_mul_ps( _mm_splat_ps( a1, 3 ), b3 ) );
	t2 = _mm_add_ps( t2, _mm_mul_ps( _mm_splat_ps( a2, 3 ), b3 ) );
	t3 = _mm_add_ps( t3, _mm_mul_ps( _mm_splat_ps( a3, 3 ), b3 ) );

	_mm_storeu_ps( out + 0 * 4, t0 );
	_mm_storeu_ps( out + 1 * 4, t1 );
	_mm_storeu_ps( out + 2 * 4, t2 );
	_mm_storeu_ps( out + 3 * 4, t3 );
}

/*
================
R_TransposeGLMatrix
================
*/
void R_TransposeGLMatrix( const float in[16], float out[16] ) {
	int		i, j;
	for( i = 0 ; i < 4 ; i++ ) {
		for( j = 0 ; j < 4 ; j++ ) {
			out[i * 4 + j] = in[j * 4 + i];
		}
	}
}

/*
=================
R_SetupViewMatrix

Sets up the world to view matrix for a given viewParm
=================
*/
void R_SetupViewMatrix( viewDef_t *viewDef ) {
	// transform by the camera placement
	const idVec3	&origin = viewDef->renderView.vieworg;
	const idMat3	&axis = viewDef->renderView.viewaxis;
	viewEntity_t	*world;
	float			viewerMatrix[16];
	static float	s_flipMatrix[16] = {
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		0, 0, -1, 0,
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	};
	world = &viewDef->worldSpace;
	memset( world, 0, sizeof( *world ) );
	// the model matrix is an identity
	world->modelMatrix[0 * 4 + 0] = 1;
	world->modelMatrix[1 * 4 + 1] = 1;
	world->modelMatrix[2 * 4 + 2] = 1;
	// transform by the camera placement
	viewerMatrix[0 * 4 + 0] = axis[0][0];
	viewerMatrix[1 * 4 + 0] = axis[0][1];
	viewerMatrix[2 * 4 + 0] = axis[0][2];
	viewerMatrix[3 * 4 + 0] = -origin[0] * axis[0][0] - origin[1] * axis[0][1] - origin[2] * axis[0][2];
	viewerMatrix[0 * 4 + 1] = axis[1][0];
	viewerMatrix[1 * 4 + 1] = axis[1][1];
	viewerMatrix[2 * 4 + 1] = axis[1][2];
	viewerMatrix[3 * 4 + 1] = - origin[0] * axis[1][0] - origin[1] * axis[1][1] - origin[2] * axis[1][2];
	viewerMatrix[0 * 4 + 2] = axis[2][0];
	viewerMatrix[1 * 4 + 2] = axis[2][1];
	viewerMatrix[2 * 4 + 2] = axis[2][2];
	viewerMatrix[3 * 4 + 2] = -origin[0] * axis[2][0] - origin[1] * axis[2][1] - origin[2] * axis[2][2];
	viewerMatrix[0 * 4 + 3] = 0.0f;
	viewerMatrix[1 * 4 + 3] = 0.0f;
	viewerMatrix[2 * 4 + 3] = 0.0f;
	viewerMatrix[3 * 4 + 3] = 1.0f;
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	R_MatrixMultiply( viewerMatrix, s_flipMatrix, world->modelViewMatrix );
}

/*
===============
R_SetupProjectionMatrix

This uses the "infinite far z" trick
===============
*/
void R_SetupProjectionMatrix( viewDef_t* viewDef ) {
	float	xmin, xmax, ymin, ymax;
	float	width, height;
	float	zNear;
	float	jitterx, jittery;
	static	idRandom random;
	// random jittering is usefull when multiple
	// frames are going to be blended together
	// for motion blurred anti-aliasing
	if( r_jitter.GetBool() ) {
		jitterx = random.RandomFloat();
		jittery = random.RandomFloat();
	} else {
		jitterx = jittery = 0;
	}
	//
	// set up projection matrix
	//
	zNear	= r_znear.GetFloat();
	if( viewDef->renderView.cramZNear ) {
		zNear *= 0.25;
	}
	ymax = zNear * tan( viewDef->renderView.fov_y * idMath::PI / 360.0f );
	ymin = -ymax;
	xmax = zNear * tan( viewDef->renderView.fov_x * idMath::PI / 360.0f );
	xmin = -xmax;
	width = xmax - xmin;
	height = ymax - ymin;
	jitterx = jitterx * width / ( viewDef->viewport.x2 - viewDef->viewport.x1 + 1 );
	xmin += jitterx;
	xmax += jitterx;
	jittery = jittery * height / ( viewDef->viewport.y2 - viewDef->viewport.y1 + 1 );
	ymin += jittery;
	ymax += jittery;

	viewDef->projectionMatrix[0 * 4 + 0] = 2.0f * zNear / width;
	viewDef->projectionMatrix[1 * 4 + 0] = 0.0f;
	viewDef->projectionMatrix[2 * 4 + 0] = ( xmax + xmin ) / width;	// normally 0
	viewDef->projectionMatrix[3 * 4 + 0] = 0.0f;

	viewDef->projectionMatrix[0 * 4 + 1] = 0.0f;
	viewDef->projectionMatrix[1 * 4 + 1] = 2.0f * zNear / height;
	viewDef->projectionMatrix[2 * 4 + 1] = ( ymax + ymin ) / height;	// normally 0
	viewDef->projectionMatrix[3 * 4 + 1] = 0.0f;
	// this is the far-plane-at-infinity formulation, and
	// crunches the Z range slightly so w=0 vertexes do not
	// rasterize right at the wraparound point
	viewDef->projectionMatrix[0 * 4 + 2] = 0.0f;
	viewDef->projectionMatrix[1 * 4 + 2] = 0.0f;
	viewDef->projectionMatrix[2 * 4 + 2] = -0.999f; // adjust value to prevent imprecision issues
	viewDef->projectionMatrix[3 * 4 + 2] = -2.0f * zNear;

	viewDef->projectionMatrix[0 * 4 + 3] = 0.0f;
	viewDef->projectionMatrix[1 * 4 + 3] = 0.0f;
	viewDef->projectionMatrix[2 * 4 + 3] = -1.0f;
	viewDef->projectionMatrix[3 * 4 + 3] = 0.0f;
}

/*
=================
R_SetupViewFrustum

Setup that culling frustum planes for the current view
FIXME: derive from modelview matrix times projection matrix
=================
*/
static void R_SetupViewFrustum( viewDef_t* viewDef ) {
	int		i;
	float	xs, xc;
	float	ang;
	ang = DEG2RAD( viewDef->renderView.fov_x ) * 0.5f;
	idMath::SinCos( ang, xs, xc );
	viewDef->frustum[0] = xs * viewDef->renderView.viewaxis[0] + xc * viewDef->renderView.viewaxis[1];
	viewDef->frustum[1] = xs * viewDef->renderView.viewaxis[0] - xc * viewDef->renderView.viewaxis[1];
	ang = DEG2RAD( tr.viewDef->renderView.fov_y ) * 0.5f;
	idMath::SinCos( ang, xs, xc );
	viewDef->frustum[2] = xs * viewDef->renderView.viewaxis[0] + xc * viewDef->renderView.viewaxis[2];
	viewDef->frustum[3] = xs * viewDef->renderView.viewaxis[0] - xc * viewDef->renderView.viewaxis[2];
	// plane four is the front clipping plane
	viewDef->frustum[4] = viewDef->renderView.viewaxis[0];
	for( i = 0; i < 5; i++ ) {
		// flip direction so positive side faces out (FIXME: globally unify this)
		viewDef->frustum[i] = -viewDef->frustum[i].Normal();
		viewDef->frustum[i][3] = -( viewDef->renderView.vieworg * viewDef->frustum[i].Normal() );
	}
	// eventually, plane five will be the rear clipping plane for fog
	float dNear, dFar, dLeft, dUp;
	dNear = r_znear.GetFloat();
	if( viewDef->renderView.cramZNear ) {
		dNear *= 0.25f;
	}
	dFar = MAX_WORLD_SIZE;
	dLeft = dFar * tan( DEG2RAD( viewDef->renderView.fov_x * 0.5f ) );
	dUp = dFar * tan( DEG2RAD( viewDef->renderView.fov_y * 0.5f ) );
	viewDef->viewFrustum.SetOrigin( viewDef->renderView.vieworg );
	viewDef->viewFrustum.SetAxis( viewDef->renderView.viewaxis );
	viewDef->viewFrustum.SetSize( dNear, dFar, dLeft, dUp );
}

/*
===================
R_ConstrainViewFrustum
===================
*/
static void R_ConstrainViewFrustum( void ) {
	idBounds bounds;
	// constrain the view frustum to the total bounds of all visible lights and visible entities
	bounds.Clear();
	for( viewLight_t *vLight = tr.viewDef->viewLights; vLight; vLight = vLight->next ) {
		bounds.AddBounds( vLight->lightDef->frustumTris->bounds );
	}
	for( viewEntity_t *vEntity = tr.viewDef->viewEntitys; vEntity; vEntity = vEntity->next ) {
		bounds.AddBounds( vEntity->entityDef->referenceBounds );
	}
	tr.viewDef->viewFrustum.ConstrainToBounds( bounds );
	if( r_useFrustumFarDistance.GetFloat() > 0.0f ) {
		tr.viewDef->viewFrustum.MoveFarDistance( r_useFrustumFarDistance.GetFloat() );
	}
}

/*
==========================================================================================

DRAWSURF SORTING

==========================================================================================
*/

/*
=======================
R_QsortSurfaces

=======================
*/
static int R_QsortSurfaces( const void *a, const void *b ) {
	const drawSurf_t	*ea, *eb;
	ea = *( drawSurf_t ** )a;
	eb = *( drawSurf_t ** )b;
	if( ea->sort < eb->sort ) {
		return -1;
	}
	if( ea->sort > eb->sort ) {
		return 1;
	}
	return 0;
}

/*
=================
R_SortDrawSurfs
=================
*/
static void R_SortDrawSurfs( void ) {
	// sort the drawsurfs by sort type, then orientation, then shader
	qsort( tr.viewDef->drawSurfs, tr.viewDef->numDrawSurfs, sizeof( tr.viewDef->drawSurfs[0] ), R_QsortSurfaces );
}



//========================================================================


//==============================================================================



/*
================
R_RenderView

A view may be either the actual camera view,
a mirror / remote location, or a 3D view on a gui surface.

Parms will typically be allocated with R_FrameAlloc
================
*/
void R_RenderView( viewDef_t *parms ) {
	viewDef_t		*oldView;
	if( parms->renderView.width <= 0 || parms->renderView.height <= 0 ) {
		return;
	}
	tr.viewCount++;
	// save view in case we are a subview
	oldView = tr.viewDef;
	tr.viewDef = parms;
	tr.sortOffset = 0;
	// set the matrix for world space to eye space
	R_SetupViewMatrix( tr.viewDef );
	// the four sides of the view frustum are needed
	// for culling and portal visibility
	R_SetupViewFrustum( tr.viewDef );
	// we need to set the projection matrix before doing
	// portal-to-screen scissor box calculations
	R_SetupProjectionMatrix( tr.viewDef );
	// identify all the visible portalAreas, and the entityDefs and
	// lightDefs that are in them and pass culling.
	static_cast<idRenderWorldLocal *>( parms->renderWorld )->FindViewLightsAndEntities();
	// constrain the view frustum to the view lights and entities
	R_ConstrainViewFrustum();
	// make sure that interactions exist for all light / entity combinations
	// that are visible
	// add any pre-generated light shadows, and calculate the light shader values
	R_AddLightSurfaces();
	// adds ambient surfaces and create any necessary interaction surfaces to add to the light
	// lists
	R_AddModelSurfaces();
	// any viewLight that didn't have visible surfaces can have it's shadows removed
	R_RemoveUnecessaryViewLights();
	// sort all the ambient surfaces for translucency ordering
	R_SortDrawSurfs();
	// generate any subviews (mirrors, cameras, etc) before adding this view
	if( R_GenerateSubViews() ) {
		// if we are debugging subviews, allow the skipping of the
		// main view draw
		if( r_subviewOnly.GetBool() ) {
			return;
		}
	}
	// write everything needed to the demo file
	if( session->writeDemo ) {
		static_cast<idRenderWorldLocal *>( parms->renderWorld )->WriteVisibleDefs( tr.viewDef );
	}
	// add the rendering commands for this viewDef
	R_AddDrawViewCmd( parms );
	// restore view in case we are a subview
	tr.viewDef = oldView;
}
