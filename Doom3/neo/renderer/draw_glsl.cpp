/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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

shaderProgram_t		interactionShader = { -1};
shaderProgram_t		ambientInteractionShader = { -1};
shaderProgram_t		stencilShadowShader = { -1};

/*
=========================================================================================

GENERAL INTERACTION RENDERING

=========================================================================================
*/

/*
====================
GL_SelectTextureNoClient
====================
*/
static void GL_SelectTextureNoClient( int unit ) {
	backEnd.glState.currenttmu = unit;
	glActiveTextureARB( GL_TEXTURE0_ARB + unit );
}

/*
==================
RB_GLSL_DrawInteraction
==================
*/
static void RB_GLSL_DrawInteraction( const drawInteraction_t *din ) {
	// load all the shader parameters
	if( din->ambientLight ) {
		glUniform4fvARB( ambientInteractionShader.localLightOrigin, 1, din->localLightOrigin.ToFloatPtr() );
		glUniform4fvARB( ambientInteractionShader.lightProjectionS, 1, din->lightProjection[0].ToFloatPtr() );
		glUniform4fvARB( ambientInteractionShader.lightProjectionT, 1, din->lightProjection[1].ToFloatPtr() );
		glUniform4fvARB( ambientInteractionShader.lightProjectionQ, 1, din->lightProjection[2].ToFloatPtr() );
		glUniform4fvARB( ambientInteractionShader.lightFalloff, 1, din->lightProjection[3].ToFloatPtr() );
		glUniform4fvARB( ambientInteractionShader.bumpMatrixS, 1, din->bumpMatrix[0].ToFloatPtr() );
		glUniform4fvARB( ambientInteractionShader.bumpMatrixT, 1, din->bumpMatrix[1].ToFloatPtr() );
		glUniform4fvARB( ambientInteractionShader.diffuseMatrixS, 1, din->diffuseMatrix[0].ToFloatPtr() );
		glUniform4fvARB( ambientInteractionShader.diffuseMatrixT, 1, din->diffuseMatrix[1].ToFloatPtr() );
		static const float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		static const float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		static const float negOne[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
		switch( din->vertexColor ) {
		case SVC_IGNORE:
			glUniform4fvARB( ambientInteractionShader.colorModulate, 1, zero );
			glUniform4fvARB( ambientInteractionShader.colorAdd, 1, one );
			break;
		case SVC_MODULATE:
			glUniform4fvARB( ambientInteractionShader.colorModulate, 1, one );
			glUniform4fvARB( ambientInteractionShader.colorAdd, 1, zero );
			break;
		case SVC_INVERSE_MODULATE:
			glUniform4fvARB( ambientInteractionShader.colorModulate, 1, negOne );
			glUniform4fvARB( ambientInteractionShader.colorAdd, 1, one );
			break;
		}
		// set the constant color
		glUniform4fvARB( ambientInteractionShader.diffuseColor, 1, din->diffuseColor.ToFloatPtr() );
	} else {
		glUniform4fvARB( interactionShader.localLightOrigin, 1, din->localLightOrigin.ToFloatPtr() );
		glUniform4fvARB( interactionShader.localViewOrigin, 1, din->localViewOrigin.ToFloatPtr() );
		glUniform4fvARB( interactionShader.lightProjectionS, 1, din->lightProjection[0].ToFloatPtr() );
		glUniform4fvARB( interactionShader.lightProjectionT, 1, din->lightProjection[1].ToFloatPtr() );
		glUniform4fvARB( interactionShader.lightProjectionQ, 1, din->lightProjection[2].ToFloatPtr() );
		glUniform4fvARB( interactionShader.lightFalloff, 1, din->lightProjection[3].ToFloatPtr() );
		glUniform4fvARB( interactionShader.bumpMatrixS, 1, din->bumpMatrix[0].ToFloatPtr() );
		glUniform4fvARB( interactionShader.bumpMatrixT, 1, din->bumpMatrix[1].ToFloatPtr() );
		glUniform4fvARB( interactionShader.diffuseMatrixS, 1, din->diffuseMatrix[0].ToFloatPtr() );
		glUniform4fvARB( interactionShader.diffuseMatrixT, 1, din->diffuseMatrix[1].ToFloatPtr() );
		glUniform4fvARB( interactionShader.specularMatrixS, 1, din->specularMatrix[0].ToFloatPtr() );
		glUniform4fvARB( interactionShader.specularMatrixT, 1, din->specularMatrix[1].ToFloatPtr() );
		static const float zero[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		static const float one[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		static const float negOne[4] = { -1.0f, -1.0f, -1.0f, -1.0f };
		switch( din->vertexColor ) {
		case SVC_IGNORE:
			glUniform4fvARB( interactionShader.colorModulate, 1, zero );
			glUniform4fvARB( interactionShader.colorAdd, 1, one );
			break;
		case SVC_MODULATE:
			glUniform4fvARB( interactionShader.colorModulate, 1, one );
			glUniform4fvARB( interactionShader.colorAdd, 1, zero );
			break;
		case SVC_INVERSE_MODULATE:
			glUniform4fvARB( interactionShader.colorModulate, 1, negOne );
			glUniform4fvARB( interactionShader.colorAdd, 1, one );
			break;
		}
		// set the constant colors
		glUniform4fvARB( interactionShader.diffuseColor, 1, din->diffuseColor.ToFloatPtr() );
		glUniform4fvARB( interactionShader.specularColor, 1, din->specularColor.ToFloatPtr() );
	}
	// set the textures
	// texture 0 will be the per-surface bump map
	GL_SelectTextureNoClient( 0 );
	din->bumpImage->Bind();
	// texture 1 will be the light falloff texture
	GL_SelectTextureNoClient( 1 );
	din->lightFalloffImage->Bind();
	// texture 2 will be the light projection texture
	GL_SelectTextureNoClient( 2 );
	din->lightImage->Bind();
	// texture 3 is the per-surface diffuse map
	GL_SelectTextureNoClient( 3 );
	din->diffuseImage->Bind();
	if( !din->ambientLight ) {
		// texture 4 is the per-surface specular map
		GL_SelectTextureNoClient( 4 );
		din->specularImage->Bind();
	}
	// draw it
	RB_DrawElementsWithCounters( din->surf->geo );
}


/*
=============
RB_GLSL_CreateDrawInteractions
=============
*/
static void RB_GLSL_CreateDrawInteractions( const drawSurf_t *surf ) {
	if( !surf ) {
		return;
	}
	// perform setup here that will be constant for all interactions
	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE | GLS_DEPTHMASK | backEnd.depthFunc );
	// bind the vertex and fragment program
	if( backEnd.vLight->lightShader->IsAmbientLight() ) {
		if( ambientInteractionShader.program == -1 ) {
			glUseProgramObjectARB( 0 );
		} else {
			glUseProgramObjectARB( ambientInteractionShader.program );
		}
	} else {
		if( interactionShader.program == -1 ) {
			glUseProgramObjectARB( 0 );
		} else {
			glUseProgramObjectARB( interactionShader.program );
		}
	}
	// enable the vertex arrays
	glEnableVertexAttribArrayARB( 8 );
	glEnableVertexAttribArrayARB( 9 );
	glEnableVertexAttribArrayARB( 10 );
	glEnableVertexAttribArrayARB( 11 );
	glEnableClientState( GL_COLOR_ARRAY );
	for( ; surf ; surf = surf->nextOnLight ) {
		// perform setup here that will not change over multiple interaction passes
		// set the vertex pointers
		idDrawVert	*ac = ( idDrawVert * )vertexCache.Position( surf->geo->ambientCache );
		glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof( idDrawVert ), ( const GLvoid * )&ac->color );
		glVertexAttribPointerARB( 11, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->normal.ToFloatPtr() );
		glVertexAttribPointerARB( 10, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[1].ToFloatPtr() );
		glVertexAttribPointerARB( 9, 3, GL_FLOAT, false, sizeof( idDrawVert ), ac->tangents[0].ToFloatPtr() );
		glVertexAttribPointerARB( 8, 2, GL_FLOAT, false, sizeof( idDrawVert ), ac->st.ToFloatPtr() );
		glVertexPointer( 3, GL_FLOAT, sizeof( idDrawVert ), ac->xyz.ToFloatPtr() );
		// set model matrix
		if( backEnd.vLight->lightShader->IsAmbientLight() ) {
			glUniformMatrix4fvARB( ambientInteractionShader.modelMatrix, 1, false, surf->space->modelMatrix );
		} else {
			glUniformMatrix4fvARB( interactionShader.modelMatrix, 1, false, surf->space->modelMatrix );
		}
		// this may cause RB_GLSL_DrawInteraction to be executed multiple
		// times with different colors and images if the surface or light have multiple layers
		RB_CreateSingleDrawInteractions( surf, RB_GLSL_DrawInteraction );
	}
	glDisableVertexAttribArrayARB( 8 );
	glDisableVertexAttribArrayARB( 9 );
	glDisableVertexAttribArrayARB( 10 );
	glDisableVertexAttribArrayARB( 11 );
	glDisableClientState( GL_COLOR_ARRAY );
	// disable features
	GL_SelectTextureNoClient( 4 );
	globalImages->BindNull();
	GL_SelectTextureNoClient( 3 );
	globalImages->BindNull();
	GL_SelectTextureNoClient( 2 );
	globalImages->BindNull();
	GL_SelectTextureNoClient( 1 );
	globalImages->BindNull();
	backEnd.glState.currenttmu = -1;
	GL_SelectTexture( 0 );
	glUseProgramObjectARB( 0 );
}

/*
==================
RB_GLSL_DrawInteractions
==================
*/
void RB_GLSL_DrawInteractions( void ) {
	viewLight_t		*vLight;
	GL_SelectTexture( 0 );
	//
	// for each light, perform adding and shadowing
	//
	for( vLight = backEnd.viewDef->viewLights ; vLight ; vLight = vLight->next ) {
		backEnd.vLight = vLight;
		// do fogging later
		if( vLight->lightShader->IsFogLight() ) {
			continue;
		}
		if( vLight->lightShader->IsBlendLight() ) {
			continue;
		}
		// if there are no interactions, get out!
		if( !vLight->localInteractions && !vLight->globalInteractions && !vLight->translucentInteractions ) {
			continue;
		}
		GL_DepthBoundsTest( vLight->scissorRect.zmin, vLight->scissorRect.zmax );
		// clear the stencil buffer if needed
		if( vLight->globalShadows || vLight->localShadows ) {
			backEnd.currentScissor = vLight->scissorRect;
			if( r_useScissor.GetBool() ) {
				GL_Scissor( backEnd.viewDef->viewport.x1 + backEnd.currentScissor.x1,
							backEnd.viewDef->viewport.y1 + backEnd.currentScissor.y1,
							backEnd.currentScissor.x2 + 1 - backEnd.currentScissor.x1,
							backEnd.currentScissor.y2 + 1 - backEnd.currentScissor.y1 );
			}
			glClear( GL_STENCIL_BUFFER_BIT );
		} else {
			// no shadows, so no need to read or write the stencil buffer
			// we might in theory want to use GL_ALWAYS instead of disabling
			// completely, to satisfy the invarience rules
			glStencilFunc( GL_ALWAYS, 128, 255 );
		}
		if( r_useShadowVertexProgram.GetBool() ) {
			glUseProgramObjectARB( stencilShadowShader.program );
			RB_StencilShadowPass( vLight->globalShadows );
			RB_GLSL_CreateDrawInteractions( vLight->localInteractions );
			glUseProgramObjectARB( stencilShadowShader.program );
			RB_StencilShadowPass( vLight->localShadows );
			RB_GLSL_CreateDrawInteractions( vLight->globalInteractions );
			// if there weren't any globalInteractions, it would have stayed on
			glUseProgramObjectARB( 0 );
		} else {
			RB_StencilShadowPass( vLight->globalShadows );
			RB_GLSL_CreateDrawInteractions( vLight->localInteractions );
			RB_StencilShadowPass( vLight->localShadows );
			RB_GLSL_CreateDrawInteractions( vLight->globalInteractions );
		}
		// translucent surfaces never get stencil shadowed
		if( r_skipTranslucent.GetBool() ) {
			continue;
		}
		// turn off depthbounds testing for translucent surfaces
		GL_DepthBoundsTest( 0.0f, 0.0f );
		glStencilFunc( GL_ALWAYS, 128, 255 );
		backEnd.depthFunc = GLS_DEPTHFUNC_LESS;
		RB_GLSL_CreateDrawInteractions( vLight->translucentInteractions );
		backEnd.depthFunc = GLS_DEPTHFUNC_EQUAL;
	}
	// disable stencil shadow test
	glStencilFunc( GL_ALWAYS, 128, 255 );
	GL_SelectTexture( 0 );
}

//===================================================================================

/*
=================
R_LoadGLSLShader

loads GLSL vertex or fragment shaders
=================
*/
bool R_LoadGLSLShader( const char *name, shaderProgram_t *shaderProgram, GLenum type ) {
	idStr	fullPath = "renderProgs/";
	fullPath += name;
	char	*fileBuffer;
	char	*buffer;
	common->Printf( "%s", fullPath.c_str() );
	// load the program even if we don't support it, so
	// fs_copyfiles can generate cross-platform data dumps
	fileSystem->ReadFile( fullPath.c_str(), ( void ** )&fileBuffer, NULL );
	if( !fileBuffer ) {
		common->Printf( ": File not found\n" );
		return false;
	}
	// copy to stack memory and free
	buffer = ( char * )_alloca( strlen( fileBuffer ) + 1 );
	strcpy( buffer, fileBuffer );
	fileSystem->FreeFile( fileBuffer );
	if( !glConfig.isInitialized ) {
		return false;
	}
	GLuint shader;
	switch( type ) {
	case GL_VERTEX_SHADER_ARB:
		if( shaderProgram->vertexShader != -1 ) {
			glDeleteShader( shaderProgram->vertexShader );
		}
		shaderProgram->vertexShader = -1;
		// create vertex shader
		shader = glCreateShaderObjectARB( GL_VERTEX_SHADER_ARB );
		glShaderSourceARB( shader, 1, ( const GLcharARB ** )&buffer, 0 );
		glCompileShaderARB( shader );
		break;
	case GL_FRAGMENT_SHADER_ARB:
		if( shaderProgram->fragmentShader != -1 ) {
			glDeleteShader( shaderProgram->fragmentShader );
		}
		shaderProgram->fragmentShader = -1;
		// create fragment shader
		shader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );
		glShaderSourceARB( shader, 1, ( const GLcharARB ** )&buffer, 0 );
		glCompileShaderARB( shader );
		break;
	default:
		common->Printf( "R_LoadGLSLShader: no type\n" );
		return false;
	}
	GLint logLength;
	glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &logLength );
	if( logLength > 1 ) {
		GLchar *log = ( GLchar * )malloc( logLength );
		glGetShaderInfoLog( shader, logLength, &logLength, log );
		common->Printf( ( const char * )log );
		free( log );
	}
	GLint status;
	glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
	if( status == 0 ) {
		glDeleteShader( shader );
		return false;
	}
	switch( type ) {
	case GL_VERTEX_SHADER_ARB:
		shaderProgram->vertexShader = shader;
		break;
	case GL_FRAGMENT_SHADER_ARB:
		shaderProgram->fragmentShader = shader;
		break;
	}
	common->Printf( "\n" );
	return true;
}

/*
=================
R_LinkGLSLShader

links the GLSL vertex and fragment shaders together to form a GLSL program
=================
*/
bool R_LinkGLSLShader( shaderProgram_t *shaderProgram, bool needsAttributes ) {
	GLint linked;
	shaderProgram->program = glCreateProgramObjectARB( );
	glAttachObjectARB( shaderProgram->program, shaderProgram->vertexShader );
	glAttachObjectARB( shaderProgram->program, shaderProgram->fragmentShader );
	if( needsAttributes ) {
		glBindAttribLocationARB( shaderProgram->program, 8, "attr_TexCoord" );
		glBindAttribLocationARB( shaderProgram->program, 9, "attr_Tangent" );
		glBindAttribLocationARB( shaderProgram->program, 10, "attr_Bitangent" );
		glBindAttribLocationARB( shaderProgram->program, 11, "attr_Normal" );
	}
	glLinkProgramARB( shaderProgram->program );
	glGetObjectParameterivARB( shaderProgram->program, GL_OBJECT_LINK_STATUS_ARB, &linked );
	if( !linked ) {
		common->Printf( "R_LinkGLSLShader: program failed to link\n" );
		return false;
	}
	return true;
}

/*
=================
R_ValidateGLSLProgram

makes sure GLSL program is valid
=================
*/
bool R_ValidateGLSLProgram( shaderProgram_t *shaderProgram ) {
	GLint validProgram;
	glValidateProgramARB( shaderProgram->program );
	glGetObjectParameterivARB( shaderProgram->program, GL_OBJECT_VALIDATE_STATUS_ARB, &validProgram );
	if( !validProgram ) {
		common->Printf( "R_ValidateGLSLProgram: program invalid\n" );
		return false;
	}
	return true;
}

/*
=================
RB_GLSL_InitShaders
=================
*/
static bool RB_GLSL_InitShaders( ) {
	// initialize to 0 at first
	memset( &interactionShader, 0, sizeof( shaderProgram_t ) );
	memset( &ambientInteractionShader, 0, sizeof( shaderProgram_t ) );
	memset( &stencilShadowShader, 0, sizeof( shaderProgram_t ) );
	// load interation shaders
	R_LoadGLSLShader( "interaction.vp", &interactionShader, GL_VERTEX_SHADER_ARB );
	R_LoadGLSLShader( "interaction.fp", &interactionShader, GL_FRAGMENT_SHADER_ARB );
	// destroy the shaders in case we failed to load them
	if( !R_LinkGLSLShader( &interactionShader, true ) && !R_ValidateGLSLProgram( &interactionShader ) ) {
		if( interactionShader.fragmentShader != -1 ) {
			glDeleteShader( interactionShader.fragmentShader );
		}
		if( interactionShader.vertexShader != -1 ) {
			glDeleteShader( interactionShader.vertexShader );
		}
		interactionShader.fragmentShader = -1;
		interactionShader.vertexShader = -1;
		common->Printf( "GLSL interactionShader failed to init.\n" );
		return false;
	} else {
		// set uniform locations
		interactionShader.u_normalTexture = glGetUniformLocationARB( interactionShader.program, "u_normalTexture" );
		interactionShader.u_lightFalloffTexture = glGetUniformLocationARB( interactionShader.program, "u_lightFalloffTexture" );
		interactionShader.u_lightProjectionTexture = glGetUniformLocationARB( interactionShader.program, "u_lightProjectionTexture" );
		interactionShader.u_diffuseTexture = glGetUniformLocationARB( interactionShader.program, "u_diffuseTexture" );
		interactionShader.u_specularTexture = glGetUniformLocationARB( interactionShader.program, "u_specularTexture" );
		interactionShader.modelMatrix = glGetUniformLocationARB( interactionShader.program, "u_modelMatrix" );
		interactionShader.localLightOrigin = glGetUniformLocationARB( interactionShader.program, "u_lightOrigin" );
		interactionShader.localViewOrigin = glGetUniformLocationARB( interactionShader.program, "u_viewOrigin" );
		interactionShader.lightProjectionS = glGetUniformLocationARB( interactionShader.program, "u_lightProjectionS" );
		interactionShader.lightProjectionT = glGetUniformLocationARB( interactionShader.program, "u_lightProjectionT" );
		interactionShader.lightProjectionQ = glGetUniformLocationARB( interactionShader.program, "u_lightProjectionQ" );
		interactionShader.lightFalloff = glGetUniformLocationARB( interactionShader.program, "u_lightFalloff" );
		interactionShader.bumpMatrixS = glGetUniformLocationARB( interactionShader.program, "u_bumpMatrixS" );
		interactionShader.bumpMatrixT = glGetUniformLocationARB( interactionShader.program, "u_bumpMatrixT" );
		interactionShader.diffuseMatrixS = glGetUniformLocationARB( interactionShader.program, "u_diffuseMatrixS" );
		interactionShader.diffuseMatrixT = glGetUniformLocationARB( interactionShader.program, "u_diffuseMatrixT" );
		interactionShader.specularMatrixS = glGetUniformLocationARB( interactionShader.program, "u_specularMatrixS" );
		interactionShader.specularMatrixT = glGetUniformLocationARB( interactionShader.program, "u_specularMatrixT" );
		interactionShader.colorModulate = glGetUniformLocationARB( interactionShader.program, "u_colorModulate" );
		interactionShader.colorAdd = glGetUniformLocationARB( interactionShader.program, "u_colorAdd" );
		interactionShader.diffuseColor = glGetUniformLocationARB( interactionShader.program, "u_diffuseColor" );
		interactionShader.specularColor = glGetUniformLocationARB( interactionShader.program, "u_specularColor" );
		// set texture locations
		glUseProgramObjectARB( interactionShader.program );
		glUniform1iARB( interactionShader.u_normalTexture, 0 );
		glUniform1iARB( interactionShader.u_lightFalloffTexture, 1 );
		glUniform1iARB( interactionShader.u_lightProjectionTexture, 2 );
		glUniform1iARB( interactionShader.u_diffuseTexture, 3 );
		glUniform1iARB( interactionShader.u_specularTexture, 4 );
		glUseProgramObjectARB( 0 );
	}
	// load ambient interation shaders
	R_LoadGLSLShader( "ambientInteraction.vp", &ambientInteractionShader, GL_VERTEX_SHADER_ARB );
	R_LoadGLSLShader( "ambientInteraction.fp", &ambientInteractionShader, GL_FRAGMENT_SHADER_ARB );
	// destroy the shaders in case we failed to load them
	if( !R_LinkGLSLShader( &ambientInteractionShader, true ) && !R_ValidateGLSLProgram( &ambientInteractionShader ) ) {
		if( ambientInteractionShader.fragmentShader != -1 ) {
			glDeleteShader( ambientInteractionShader.fragmentShader );
		}
		if( ambientInteractionShader.vertexShader != -1 ) {
			glDeleteShader( ambientInteractionShader.vertexShader );
		}
		ambientInteractionShader.fragmentShader = -1;
		ambientInteractionShader.vertexShader = -1;
		common->Printf( "GLSL ambientInteractionShader failed to init.\n" );
		return false;
	} else {
		// set uniform locations
		ambientInteractionShader.u_normalTexture = glGetUniformLocationARB( ambientInteractionShader.program, "u_normalTexture" );
		ambientInteractionShader.u_lightFalloffTexture = glGetUniformLocationARB( ambientInteractionShader.program, "u_lightFalloffTexture" );
		ambientInteractionShader.u_lightProjectionTexture = glGetUniformLocationARB( ambientInteractionShader.program, "u_lightProjectionTexture" );
		ambientInteractionShader.u_diffuseTexture = glGetUniformLocationARB( ambientInteractionShader.program, "u_diffuseTexture" );
		ambientInteractionShader.modelMatrix = glGetUniformLocationARB( ambientInteractionShader.program, "u_modelMatrix" );
		ambientInteractionShader.localLightOrigin = glGetUniformLocationARB( ambientInteractionShader.program, "u_lightOrigin" );
		ambientInteractionShader.lightProjectionS = glGetUniformLocationARB( ambientInteractionShader.program, "u_lightProjectionS" );
		ambientInteractionShader.lightProjectionT = glGetUniformLocationARB( ambientInteractionShader.program, "u_lightProjectionT" );
		ambientInteractionShader.lightProjectionQ = glGetUniformLocationARB( ambientInteractionShader.program, "u_lightProjectionQ" );
		ambientInteractionShader.lightFalloff = glGetUniformLocationARB( ambientInteractionShader.program, "u_lightFalloff" );
		ambientInteractionShader.bumpMatrixS = glGetUniformLocationARB( ambientInteractionShader.program, "u_bumpMatrixS" );
		ambientInteractionShader.bumpMatrixT = glGetUniformLocationARB( ambientInteractionShader.program, "u_bumpMatrixT" );
		ambientInteractionShader.diffuseMatrixS = glGetUniformLocationARB( ambientInteractionShader.program, "u_diffuseMatrixS" );
		ambientInteractionShader.diffuseMatrixT = glGetUniformLocationARB( ambientInteractionShader.program, "u_diffuseMatrixT" );
		ambientInteractionShader.colorModulate = glGetUniformLocationARB( ambientInteractionShader.program, "u_colorModulate" );
		ambientInteractionShader.colorAdd = glGetUniformLocationARB( ambientInteractionShader.program, "u_colorAdd" );
		ambientInteractionShader.diffuseColor = glGetUniformLocationARB( ambientInteractionShader.program, "u_diffuseColor" );
		// set texture locations
		glUseProgramObjectARB( ambientInteractionShader.program );
		glUniform1iARB( ambientInteractionShader.u_normalTexture, 0 );
		glUniform1iARB( ambientInteractionShader.u_lightFalloffTexture, 1 );
		glUniform1iARB( ambientInteractionShader.u_lightProjectionTexture, 2 );
		glUniform1iARB( ambientInteractionShader.u_diffuseTexture, 3 );
		glUseProgramObjectARB( 0 );
	}
	// load stencil shadow extrusion shaders
	R_LoadGLSLShader( "stencilshadow.vp", &stencilShadowShader, GL_VERTEX_SHADER_ARB );
	R_LoadGLSLShader( "stencilshadow.fp", &stencilShadowShader, GL_FRAGMENT_SHADER_ARB );
	// destroy the shaders in case we failed to load them
	if( !R_LinkGLSLShader( &stencilShadowShader, false ) && !R_ValidateGLSLProgram( &stencilShadowShader ) ) {
		if( stencilShadowShader.fragmentShader != -1 ) {
			glDeleteShader( stencilShadowShader.fragmentShader );
		}
		if( stencilShadowShader.vertexShader != -1 ) {
			glDeleteShader( stencilShadowShader.vertexShader );
		}
		stencilShadowShader.fragmentShader = -1;
		stencilShadowShader.vertexShader = -1;
		common->Printf( "GLSL stencilShadowShader failed to init.\n" );
		return false;
	} else {
		// set uniform locations
		stencilShadowShader.localLightOrigin = glGetUniformLocationARB( stencilShadowShader.program, "u_lightOrigin" );
	}
	return true;
}

/*
==================
R_GLSL_Init
==================
*/
void R_GLSL_Init( void ) {
	glConfig.allowGLSLPath = false;
	common->Printf( "---------- R_GLSL_Init -----------\n" );
	if( !glConfig.ARBShadingLanguageAvailable ) {
		common->Printf( "Not available.\n" );
		return;
	} else if( !RB_GLSL_InitShaders() ) {
		common->Printf( "GLSL shaders failed to init.\n" );
		return;
	}
	GL_CheckErrors();
	common->Printf( "Available.\n" );
	common->Printf( "---------------------------------\n" );
	glConfig.allowGLSLPath = true;
}

