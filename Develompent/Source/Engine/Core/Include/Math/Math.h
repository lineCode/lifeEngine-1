/**
 * @file
 * @addtogroup Core Core
 *
 * Copyright Broken Singularity, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef MATH_H
#define MATH_H

#include <glm.hpp>
#include <gtx/quaternion.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/transform.hpp>

#include "System/Archive.h"
#include "Core.h"

/**
 * @ingroup Core
 * Typedef glm::vec2
 */
typedef glm::vec2		FVector2D;

/**
 * @ingroup Core
 * Typedef glm::vec3
 */
typedef glm::vec3		FVector;

/**
 * @ingroup Core
 * Typedef glm::vec4
 */
typedef glm::vec4		FVector4D;

/**
 * @ingroup Core
 * Typedef glm::mat4
 */
typedef glm::mat4		FMatrix;

/**
 * @ingroup Core
 * Typedef glm::quat
 */
typedef glm::quat		FQuaternion;

/**
 * @ingroup Core
 * Structure for all math helper functions
 */
struct FMath
{
	/**
	 * @ingroup Core
	 * Convert from euler angles to quaternion
	 * 
	 * @param[in] InEulerAngleX Euler angle by X
	 * @param[in] InEulerAngleY Euler angle by Y
	 * @param[in] InEulerAngleZ Euler angle by Z
	 */
	static FORCEINLINE FQuaternion AnglesToQuaternion( float InEulerAngleX, float InEulerAngleY, float InEulerAngleZ )
	{
		return 
			glm::angleAxis( InEulerAngleX, FVector( 1.f, 0.f, 0.f ) ) *
			glm::angleAxis( InEulerAngleY, FVector( 0.f, 1.f, 0.f ) ) *
			glm::angleAxis( InEulerAngleZ, FVector( 0.f, 0.f, 1.f ) );
	}

	/**
	 * @ingroup Core
	 * Convert from euler angles to quaternion
	 * 
	 * @param[in] InEulerAngles Euler angles
	 */
	static FORCEINLINE FQuaternion AnglesToQuaternion( const FVector& InEulerAngles )
	{
		return AnglesToQuaternion( InEulerAngles.x, InEulerAngles.y, InEulerAngles.z );
	}

	/**
	 * @ingroup Core
	 * Convert from quaternion to euler angles
	 * 
	 * @param[in] InQuaternion Quaternion
	 * @return Return euler angles in radians
	 */
	static FORCEINLINE FVector QuaternionToAngles( const FQuaternion& InQuaternion )
	{
		return glm::eulerAngles( InQuaternion );
	}

    /**
     * @brief Convert from quaternion to matrix
     *
     * @param InQuaternion Quaternion
     * @return Return matrix of ratation
     */
    static FORCEINLINE FMatrix QuaternionToMatrix( const FQuaternion& InQuaternion )
    {
        return glm::mat4_cast( InQuaternion );
    }

	/**
	 * @ingroup Core
	 * Convert radians to degrees
	 * 
	 * @param[in] InRadians Radians
	 */
	static FORCEINLINE float RadiansToDegrees( float InRadians )
	{
		return glm::degrees( InRadians );
	}

	/**
	 * @ingroup Core
	 * Convert degrees to radians
	 * 
	 * @param[in] InDegrees Degrees
	 */
	static FORCEINLINE float DegreesToRadians( float InDegrees )
	{
		return glm::radians( InDegrees );
	}

    /**
     * @brief Create translate matrix
     *
     * @param InLocation Location
     * @return Return created translate matrix
     */
    static FORCEINLINE FMatrix CreateTranslateMatrix( const FVector& InLocation )
    {
        return glm::translate( InLocation );
    }

    /**
     * @brief Create scale matrix
     *
     * @param InScale Scale
     * @return Return created scale matrix
     */
    static FORCEINLINE FMatrix CreateScaleMatrix( const FVector& InScale )
    {
        return glm::scale( InScale );
    }

	/**
	 * @brief Normalize vector
	 * 
	 * @param InVector Vector
	 * @return Return normalized vector
	 */
	static FORCEINLINE FVector2D NormalizeVector( const FVector2D& InVector )
	{
		return glm::normalize( InVector );
	}

	/**
	 * @brief Normalize vector
	 *
	 * @param InVector Vector
	 * @return Return normalized vector
	 */
	static FORCEINLINE FVector NormalizeVector( const FVector& InVector )
	{
		return glm::normalize( InVector );
	}

	/**
	 * @brief Normalize vector
	 *
	 * @param InVector Vector
	 * @return Return normalized vector
	 */
	static FORCEINLINE FVector4D NormalizeVector( const FVector4D& InVector )
	{
		return glm::normalize( InVector );
	}

	static const FVector				vectorZero;			/**< Zero 3D vector */
	static const FVector				vectorOne;			/**< One 3D vector */
	static const FQuaternion			quaternionZero;		/**< Quaternion zero */
	static const FMatrix				matrixIdentity;		/**< Identity matrix */
	static const class FRotator			rotatorZero;		/**< A rotator of zero degrees on each axis */
	static const struct FTransform		transformZero;		/**< Transform zero */
	static const FVector				vectorForward;		/**< Forward vector */
	static const FVector				vectorRight;		/**< Right vector */
	static const FVector				vectorUp;			/**< Up vector */
};

//
// Serialization
//

FORCEINLINE FArchive& operator<<( FArchive& InArchive, FVector2D& InValue )
{
	InArchive.Serialize( &InValue, sizeof( InValue ) );
	return InArchive;
}

FORCEINLINE FArchive& operator<<( FArchive& InArchive, const FVector2D& InValue )
{
	check( InArchive.IsSaving() );
	InArchive.Serialize( ( void* ) &InValue, sizeof( InValue ) );
	return InArchive;
}

FORCEINLINE FArchive& operator<<( FArchive& InArchive, FVector& InValue )
{
	InArchive.Serialize( &InValue, sizeof( InValue ) );
	return InArchive;
}

FORCEINLINE FArchive& operator<<( FArchive& InArchive, const FVector& InValue )
{
	check( InArchive.IsSaving() );
	InArchive.Serialize( ( void* ) &InValue, sizeof( InValue ) );
	return InArchive;
}

FORCEINLINE FArchive& operator<<( FArchive& InArchive, FVector4D& InValue )
{
	InArchive.Serialize( &InValue, sizeof( InValue ) );
	return InArchive;
}

FORCEINLINE FArchive& operator<<( FArchive& InArchive, const FVector4D& InValue )
{
	check( InArchive.IsSaving() );
	InArchive.Serialize( ( void* ) &InValue, sizeof( InValue ) );
	return InArchive;
}

FORCEINLINE FArchive& operator<<( FArchive& InArchive, FMatrix& InValue )
{
	InArchive.Serialize( &InValue, sizeof( InValue ) );
	return InArchive;
}

FORCEINLINE FArchive& operator<<( FArchive& InArchive, const FMatrix& InValue )
{
	check( InArchive.IsSaving() );
	InArchive.Serialize( ( void* ) &InValue, sizeof( InValue ) );
	return InArchive;
}

FORCEINLINE FArchive& operator<<( FArchive& InArchive, FQuaternion& InValue )
{
	InArchive.Serialize( &InValue, sizeof( InValue ) );
	return InArchive;
}

FORCEINLINE FArchive& operator<<( FArchive& InArchive, const FQuaternion& InValue )
{
	check( InArchive.IsSaving() );
	InArchive.Serialize( ( void* ) &InValue, sizeof( InValue ) );
	return InArchive;
}

#endif // !MATH_H
