/**
 * @file
 * @addtogroup Core Core
 *
 * Copyright Broken Singularity, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef GUID_H
#define GUID_H

#include <string>

#include "Misc/Types.h"
#include "Containers/String.h"
#include "System/Archive.h"
#include "CoreDefines.h"

/**
 * @ingroup Core
 * Implementation of GUID
 */
class FGuid
{
public:
	/**
	 * @brief Functions to extract the GUID as a key for std::unordered_map and std::unordered_set
	 */
	struct FGuidKeyFunc
	{
		/**
		 * @brief Calculate hash of the GUID
		 *
		 * @param InGUID GUID
		 * @return Return hash of this GUID
		 */
		FORCEINLINE std::size_t operator()( const FGuid& InGUID ) const
		{
			return InGUID.GetTypeHash();
		}

		/**
		 * @brief Compare FGuid
		 *
		 * @param InA First GUID
		 * @param InB Second GUID
		 * @return Return true if InA and InB equal, else returning false
		 */
		FORCEINLINE bool operator()( const FGuid& InA, const FGuid& InB ) const
		{
			return InA < InB;
		}
	};

	/**
	 * Constructor
	 */
	FGuid()
		: a( 0 ), b( 0 ), c( 0 ), d( 0 )
	{}

	/**
	 * Constructor
	 * 
	 * @param InA A section in GUID
	 * @param InB B section in GUID
	 * @param InC C section in GUID
	 * @param InD D section in GUID
	 */
	FGuid( uint32 InA, uint32 InB, uint32 InC, uint32 InD )
		: a( InA ), b( InB ), c( InC ), d( InD )
	{}

	/**
	 * Constructor
	 * 
	 * @param InGUID Other GUID
	 */
	FGuid( const FGuid& InGUID )
		: a( InGUID.a ), b( InGUID.b ), c( InGUID.c ), d( InGUID.d )
	{}

	/**
	 * Is valid GUID
	 * 
	 * @return Return true if valid, false otherwise
	 */
	FORCEINLINE bool IsValid() const
	{
		return ( a | b | c | d ) != 0;
	}

	/**
	 * Invalidates the GUID
	 */
	FORCEINLINE void Invalidate()
	{
		a = b = c = d = 0;
	}

	/**
	 * Overrload operator ==
	 */
	FORCEINLINE friend bool operator==( const FGuid& InX, const FGuid& InY )
	{
		return ( ( InX.a ^ InY.a ) | ( InX.b ^ InY.b ) | ( InX.c ^ InY.c ) | ( InX.d ^ InY.d ) ) == 0;
	}

	/**
	 * Overrload operator !=
	 */
	FORCEINLINE friend bool operator!=( const FGuid& InX, const FGuid& InY )
	{
		return ( ( InX.a ^ InY.a ) | ( InX.b ^ InY.b ) | ( InX.c ^ InY.c ) | ( InX.d ^ InY.d ) ) != 0;
	}

	/**
	 * Operator <
	 */
	FORCEINLINE bool operator<( const FGuid& InOther ) const
	{
		if ( a < InOther.a )
		{
			return true;
		}
		if ( b < InOther.b )
		{
			return true;
		}
		if ( c < InOther.c )
		{
			return true;
		}
		if ( d < InOther.d )
		{
			return true;
		}
		return false;
	}

	/**
	 * Set GUID
	 * 
	 * @param InA A section in GUID
	 * @param InB B section in GUID
	 * @param InC C section in GUID
	 * @param InD D section in GUID
	 */
	FORCEINLINE void Set( uint32 InA, uint32 InB, uint32 InC, uint32 InD )
	{
		a = InA;
		b = InB;
		c = InC;
		d = InD;
	}

	/**
	 * Overload operator << for serialize
	 */
	FORCEINLINE friend FArchive& operator<<( FArchive& InAr, FGuid& InGuid )
	{
		return InAr << InGuid.a << InGuid.b << InGuid.c << InGuid.d;
	}

	/**
	 * Overload operator << for serialize
	 */
	FORCEINLINE friend FArchive& operator<<( FArchive& InAr, const FGuid& InGuid )
	{
		return InAr << InGuid.a << InGuid.b << InGuid.c << InGuid.d;
	}

	/**
	 * Print to string the GUID
	 * @return Return GUID in string format
	 */
	FORCEINLINE std::wstring String() const
	{
		return FString::Format( TEXT( "%08X%08X%08X%08X" ), a, b, c, d );
	}

	/**
	 * Init GUID from string
	 * 
	 * @param InString String
	 * @return Return true if GUID is initialized seccussed, else return false
	 */
	FORCEINLINE bool InitFromString( const std::wstring& InString )
	{
		bool		bSuccessful = false;

		// Size matches, try to parse it
		if ( InString.size() == 32 )
		{
			swscanf( InString.c_str(), TEXT( "%08X%08X%08X%08X" ), &a, &b, &c, &d );
			bSuccessful = true;
		}
		// Size mis-match, invalidate the Guid
		else
		{
			Invalidate();
		}
		return bSuccessful;
	}

	/**
	 * Get hash of type
	 * @return Return hash of this GUID
	 */
	FORCEINLINE uint32 GetTypeHash() const
	{
		return appMemFastHash( *this );
	}

private:
	uint32		a;		/**< A section in GUID */
	uint32		b;		/**< B section in GUID */
	uint32		c;		/**< C section in GUID */
	uint32		d;		/**< D section in GUID */
};

/**
 * @ingroup Core
 * @return Return created GUID
 */
FGuid appCreateGuid();

#endif // !GUID_H