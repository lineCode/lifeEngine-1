/**
 * @file
 * @addtogroup Physics Physics
 *
 * Copyright Broken Singularity, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef PHYSICSENGINE_H
#define PHYSICSENGINE_H

#include <string>
#include <unordered_map>

#include "Logger/LoggerMacros.h"
#include "System/PhysicsMaterial.h"
#include "Core.h"

/**
 * @ingroup Physics
 * @brief Main class of physics engine
 */
class FPhysicsEngine
{
public:
	/**
	 * @brief Constructor
	 */
	FPhysicsEngine();

	/**
	 * @brief Destructor
	 */
	~FPhysicsEngine();

	/**
	 * @brief Initialize engine
	 */
	void Init();

	/**
	 * @brief Tick engine
	 * 
	 * @param InDeltaTime The time since the last tick
	 */
	void Tick( float InDeltaTime );

	/**
	 * @brief Shutdown engine
	 */
	void Shutdown();

	/**
	 * @brief Find collision profile
	 *
	 * @param InName Name of collision profile
	 * @return Return pointer to finded collision profile. If not founded retulr nullptr
	 */
	FORCEINLINE FCollisionProfile* FindCollisionProfile( const std::wstring& InName ) const
	{
		auto	itProfile = collisionProfiles.find( InName );
		if ( itProfile == collisionProfiles.end() )
		{
			LE_LOG( LT_Warning, LC_Physics, TEXT( "Collision profile '%s' not founded" ), InName.c_str() );
			return nullptr;
		}

		return &itProfile->second;
	}

	/**
	 * @brief Get default physics material
	 * @return Return default physics material
	 */
	FORCEINLINE TWeakPtr<FPhysicsMaterial> GetDefaultPhysMaterial() const
	{
		return defaultPhysMaterial;
	}

private:
	TWeakPtr<FPhysicsMaterial>											defaultPhysMaterial;			/**< Default physics material */
	mutable std::unordered_map< std::wstring,  FCollisionProfile >		collisionProfiles;				/**< Collision profiles map */
};

#endif // !PHYSICSENGINE_H