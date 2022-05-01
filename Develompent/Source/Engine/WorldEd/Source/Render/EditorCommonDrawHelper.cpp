#include "Misc/EngineGlobals.h"
#include "Misc/WorldEdGlobals.h"
#include "Misc/Template.h"
#include "System/World.h"
#include "System/EditorEngine.h"
#include "Render/EditorCommonDrawHelper.h"
#include "Render/Scene.h"
#include "Render/RenderUtils.h"
#include "EngineDefines.h"

FEditorCommonDrawHelper::FEditorCommonDrawHelper()
	: bDrawColoredOrigin( true )
	, bDrawWorldBox( true )
	, perspectiveGridSize( HALF_WORLD_MAX1 )
	, gridColorHi( 127, 127, 127 )
	, gridColorLo( 63, 63, 63 )
	, worldBoxColor( 40, 40, 40 )
{}

void FEditorCommonDrawHelper::DrawGridSection( int32 InViewportLocX, int32 InViewportGridY, FVector& InStart, FVector& InEnd, float& InStartX, float& InEndX, int32 InAxis, bool InIsAlphaCase, const class FSceneView* InSceneView, struct FSceneDepthGroup& InSDG )
{
	if ( !InViewportGridY )
	{
		return;
	}

	const FMatrix&				projectionMatrix = InSceneView->GetProjectionMatrix();
	FMatrix	invViewProjMatrix	= FMath::InverseMatrix( projectionMatrix ) * FMath::InverseMatrix( InSceneView->GetViewMatrix() );
	int32	start				= FMath::Trunc( ( FVector4D( -1.f, -1.f, -1.f, 1.f ) * invViewProjMatrix )[ InAxis ] / InViewportGridY ) + InViewportLocX;
	int32	end					= FMath::Trunc( ( FVector4D( +1.f, +1.f, +1.f, 1.f ) * invViewProjMatrix )[ InAxis ] / InViewportGridY ) - InViewportLocX;
	if ( start > end )
	{
		Swap( start, end );
	}

	float	sizeX		= InSceneView->GetSizeX();
	float	zoom		= ( 1.0f / projectionMatrix[ 0 ].x ) * 2.0f / sizeX;
	int32   dist		= FMath::Trunc( sizeX * zoom / InViewportGridY );

	// Figure out alpha interpolator for fading in the grid lines.
	float	alpha		= 0.f;
	int32	incBits		= 0;
	if ( dist + dist >= sizeX / 4 )
	{
		while ( ( dist >> incBits ) >= sizeX / 4 )
		{
			incBits++;
		}
		alpha			= 2.f - 2.f * ( float )dist / ( float )( ( 1 << incBits ) * sizeX / 4.f );
	}
	else
	{
		alpha			= 1.f;
	}

	int32	iStart		= Max<int32>( start - 1, -HALF_WORLD_MAX / InViewportGridY ) >> incBits;
	int32	iEnd		= Min<int32>( end + 1, +HALF_WORLD_MAX / InViewportGridY ) >> incBits;
	for ( int32 index = iStart; index < iEnd; ++index )
	{
		InStartX		= ( index * InViewportGridY ) << incBits;
		InEndX			= ( index * InViewportGridY ) << incBits;
		if ( ( index & 1 ) != InIsAlphaCase )
		{
			FVector4D		background( InSceneView->GetBackgroundColor().ToNormalizedVector4D() );
			FVector4D		grid( 0.5f, 0.5f, 0.5f, 0.f );
			FVector4D		color = background + ( grid - background ) * ( float )( ( ( index << incBits ) & 7 ) ? 0.5f : 1.f );
			if ( index & 1 )
			{
				color		= background + ( color - background ) * alpha;
			}

			InSDG.simpleElements.AddLine( InStart, InEnd, FColor( color ) );
		}
	}
}

void FEditorCommonDrawHelper::DrawGrid( const class FSceneView* InSceneView, ELevelViewportType InViewportType )
{
	FScene*					scene = ( FScene* )GWorld->GetScene();
	FSceneDepthGroup&		SDG = scene->GetSDG( SDG_WorldEdBackground );
	FVector					origin;
	FVector					a( 0.f, 0.f, 0.f );			// Start line
	FVector					b( 0.f, 0.f, 0.f );			// End line
	FMath::GetOriginMatrix( FMath::InverseMatrix( InSceneView->GetViewMatrix() ), origin );

	// Draw 3D perspective grid
	if ( InViewportType == LVT_Perspective )
	{
		// Index of middle line (axis)
		uint32		midleLine = ( 63 - 1 ) / 2;

		// Draw grid
		for ( uint32 index = 0; index < 63; index++ )
		{
			if ( index == midleLine )
			{
				continue;
			}

			a.x = ( perspectiveGridSize / 4.f ) * ( -1.0 + 2.0 * index / ( 63 - 1 ) );			b.x = a.x;
			a.z = ( perspectiveGridSize / 4.f );												b.z = -( perspectiveGridSize / 4.f );
			a.y = 0.f;																			b.y = 0.f;
			SDG.simpleElements.AddLine( a, b, ( index == midleLine ) ? gridColorHi : gridColorLo );

			a.z = a.x;									b.z = b.x;
			a.x = ( perspectiveGridSize / 4.f );		b.x = -( perspectiveGridSize / 4.f );
			SDG.simpleElements.AddLine( a, b, ( index == midleLine ) ? gridColorHi : gridColorLo );
		}
	}

	// Draw ortho grid
	else
	{
		for ( int32 alphaCase = 0; alphaCase <= 1; ++alphaCase )
		{
			switch ( InViewportType )
			{
			case LVT_OrthoXZ:
				// Do X-Axis lines.
				a.x = +HALF_WORLD_MAX1;			a.y = 0.0;
				b.x = -HALF_WORLD_MAX1;			b.y = 0.0;
				DrawGridSection( origin.z, GEditorEngine->GetConstraints().GetGridSize(), a, b, a.z, b.z, 2, alphaCase, InSceneView, SDG );
				
				// Do Z-Axis lines.
				a.z = +HALF_WORLD_MAX1;			a.y = 0.f;
				b.z = -HALF_WORLD_MAX1;			b.y = 0.f;
				DrawGridSection( origin.x, GEditorEngine->GetConstraints().GetGridSize(), a, b, a.x, b.x, 0, alphaCase, InSceneView, SDG );
				break;

			case LVT_OrthoXY:
				// Do X-Axis lines.
				a.x = +HALF_WORLD_MAX1;			a.z = 0.f;
				b.x = -HALF_WORLD_MAX1;			b.z = 0.f;
				DrawGridSection( origin.y, GEditorEngine->GetConstraints().GetGridSize(), a, b, a.y, b.y, 1, alphaCase, InSceneView, SDG );
				
				// Do Y-Axis lines.
				a.y = +HALF_WORLD_MAX1;			a.z = 0.f;
				b.y = -HALF_WORLD_MAX1;			b.z = 0.f;
				DrawGridSection( origin.x, GEditorEngine->GetConstraints().GetGridSize(), a, b, a.x, b.x, 0, alphaCase, InSceneView, SDG );
				break;

			case LVT_OrthoYZ:
				// Do Y-Axis lines.
				a.y = +HALF_WORLD_MAX1;			a.x = 0.f;
				b.y = -HALF_WORLD_MAX1;			b.x = 0.f;
				DrawGridSection( origin.z, GEditorEngine->GetConstraints().GetGridSize(), a, b, a.z, b.z, 2, alphaCase, InSceneView, SDG );

				// Do Z-Axis lines.
				a.z = +HALF_WORLD_MAX1;			a.x = 0.f;
				b.z = -HALF_WORLD_MAX1;			b.x = 0.f;
				DrawGridSection( origin.y, GEditorEngine->GetConstraints().GetGridSize(), a, b, a.y, b.y, 1, alphaCase, InSceneView, SDG );
				break;
			}
		}
	}

	// Draw axis lines
	if ( bDrawColoredOrigin )
	{
		a = FVector( 0.f, 0.f, ( float )HALF_WORLD_MAX1 );			b = FVector( 0.f, 0.f, 0.f );
		SDG.simpleElements.AddLine( a, b, FColor( 64, 255, 64 ) );

		a = FVector( 0.f, 0.f, 0.f );								b = FVector( 0.f, 0.f, ( float )-HALF_WORLD_MAX1 );
		SDG.simpleElements.AddLine( a, b, FColor( 32, 128, 32 ) );

		a = FVector( 0.f, ( float )HALF_WORLD_MAX1, 0.f );			b = FVector( 0.f, 0.f, 0.f );
		SDG.simpleElements.AddLine( a, b, FColor( 64, 64, 255 ) );

		a = FVector( 0.f, 0.f, 0.f );								b = FVector( 0.f, ( float )-HALF_WORLD_MAX1, 0.f );
		SDG.simpleElements.AddLine( a, b, FColor( 32, 32, 128 ) );

		a = FVector( ( float )HALF_WORLD_MAX1, 0.f, 0.f );			b = FVector( 0.f, 0.f, 0.f );
		SDG.simpleElements.AddLine( a, b, FColor( 255, 64, 64 ) );

		a = FVector( 0.f, 0.f, 0.f );								b = FVector( ( float )-HALF_WORLD_MAX1, 0.f, 0.f );
		SDG.simpleElements.AddLine( a, b, FColor( 128, 32, 32 ) );
	}

	// Draw orthogonal worldframe
	if ( bDrawWorldBox )
	{
		DrawWireframeBox( SDG, FBox( FVector( -HALF_WORLD_MAX1, -HALF_WORLD_MAX1, -HALF_WORLD_MAX1 ), FVector( HALF_WORLD_MAX1, HALF_WORLD_MAX1, HALF_WORLD_MAX1 ) ), worldBoxColor );
	}
}