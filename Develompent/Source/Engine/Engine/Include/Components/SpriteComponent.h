/**
 * @file
 * @addtogroup Engine Engine
 *
 * Copyright Broken Singularity, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef SPRITECOMPONENT_H
#define SPRITECOMPONENT_H

#include "Components/PrimitiveComponent.h"
#include "Render/Sprite.h"

enum ESpriteType
{
    ST_Static,                  /**< Static sprite */
    ST_Rotating,                /**< Rotating sprite to player camera */
    ST_RotatingOnlyVertical     /**< Rotating sprite to player camera only by vertical */
};

 /**
  * @ingroup Engine
  * @brief Component for work with sprite
  */
class LSpriteComponent : public LPrimitiveComponent
{
	DECLARE_CLASS( LSpriteComponent, LPrimitiveComponent )

public:
    /**
     * @brief Constructor
     */
    LSpriteComponent();

    /**
     * Begins Play for the component.
     * Called when the owning Actor begins play or when the component is created if the Actor has already begun play.
     */
    virtual void BeginPlay() override;

	/**
	 * @brief Add primitive to draw list
	 *
	 * @param InScene Scene
	 * @param InSceneView Current view of scene
	 */
	virtual void AddToDrawList( class FScene* InScene, const class FSceneView& InSceneView ) override;

	/**
	 * @brief Serialize component
	 * @param[in] InArchive Archive for serialize
	 */
	virtual void Serialize( class FArchive& InArchive ) override;

    /**
     * @brief Set sprite type
     *
     * @param InType Sprite type
     */
    FORCEINLINE void SetType( ESpriteType InType )
    {
        type = InType;
    }

    /**
     * @brief Get sprite type
     * @return Return sprite type
     */
    FORCEINLINE ESpriteType GetType() const
    {
        return type;
    }

	/**
	 * @brief Set texture rect
	 * @param InTextureRect Texture rect
	 */
	FORCEINLINE void SetTextureRect( const FRectFloat& InTextureRect )
	{
		sprite->SetTextureRect( InTextureRect );
	}

	/**
	 * @brief Get texture rect
	 * @return Return texture rect
	 */
	FORCEINLINE const FRectFloat& GetTextureRect() const
	{
		return sprite->GetTextureRect();
	}

	/**
	 * @brief Set sprite size
	 * @param InSpriteSize Sprite size
	 */
	FORCEINLINE void SetSpriteSize( const FVector2D& InSpriteSize )
	{
		sprite->SetSpriteSize( InSpriteSize );
	}

	/**
	 * @brief Get sprite size
	 * @return Return sprite size
	 */
	FORCEINLINE const FVector2D& GetSpriteSize() const
	{
		return sprite->GetSpriteSize();
	}

	/**
	 * @brief Set material
	 * @param InMaterial Material
	 */
	FORCEINLINE void SetMaterial( FMaterial* InMaterial )
	{
		sprite->SetMaterial( InMaterial );
	}

	/**
	 * @brief Get material
	 * @return Return pointer to material. If not setted returning nullptr
	 */
	FORCEINLINE FMaterialRef GetMaterial() const
	{
		return sprite->GetMaterial();
	}

private:
	/**
	 * @brief Calculate transformation matrix
	 *
	 * @param InSceneView Current view of scene
	 * @return Return calculated transformation matrix
	 */
	FMatrix CalcTransformationMatrix( const class FSceneView& InSceneView ) const;

    ESpriteType         type;       /**< Sprite type */
	FSpriteRef			sprite;		/**< Sprite mesh */
};

//
// Serialization
//

/**
 * Overload operator << for serialize ESpriteType
 */
FORCEINLINE FArchive& operator<<( FArchive& InArchive, ESpriteType& InValue )
{
	InArchive.Serialize( &InValue, sizeof( ESpriteType ) );
	return InArchive;
}

/**
 * Overload operator << for serialize ESpriteType
 */
FORCEINLINE FArchive& operator<<( FArchive& InArchive, const ESpriteType& InValue )
{
	check( InArchive.IsSaving() );
	InArchive.Serialize( ( void* ) &InValue, sizeof( ESpriteType ) );
	return InArchive;
}

#endif // !SPRITECOMPONENT_H