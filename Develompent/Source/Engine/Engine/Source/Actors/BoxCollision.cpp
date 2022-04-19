#include "Actors/BoxCollision.h"

IMPLEMENT_CLASS( ABoxCollision )

ABoxCollision::ABoxCollision()
{
	collisionComponent = CreateComponent< LBoxComponent >( TEXT( "BoxComponent0" ) );
	boxComponent = collisionComponent;
	SetStatic( true );
}

#if WITH_EDITOR
#include "Commandlets/CookPackagesCommandlet.h"

bool ABoxCollision::InitProperties( const std::vector<FActorVar>& InActorVars, class LCookPackagesCommandlet* InCooker )
{
	if ( !Super::InitProperties( InActorVars, InCooker ) )
	{
		return false;
	}

	for ( uint32 index = 0, count = InActorVars.size(); index < count; ++index )
	{
		const FActorVar& actorVar = InActorVars[ index ];

		// If property is size
		if ( actorVar.GetName() == TEXT( "Size" ) )
		{
			check( actorVar.GetType() == AVT_Vector3D );
			boxComponent->SetSize( actorVar.GetValueVector3D() );
		}
	}

	return true;
}
#endif // WITH_EDITOR