#include <unordered_set>

#include "System/Config.h"
#include "Misc/EngineGlobals.h"
#include "Logger/LoggerMacros.h"
#include "Components/InputComponent.h"
#include "System/InputSystem.h"

IMPLEMENT_CLASS( LInputComponent )

void LInputComponent::BeginPlay()
{
	Super::BeginPlay();

	// Get mapping of buttons
	// Actions
	std::vector< FConfigValue >		configActions = GInputConfig.GetValue( TEXT( "InputSystem.InputSettings" ), TEXT( "Actions" ) ).GetArray();
	for ( uint32 indexAction = 0, countActions = configActions.size(); indexAction < countActions; ++indexAction )
	{
		// Get JSON object of action item
		const FConfigValue&		configAction = configActions[ indexAction ];
		check( configAction.GetType() == FConfigValue::T_Object );
		FConfigObject			configObject = configAction.GetObject();

		// Get name of action
		FInputAction			inputAction;
		inputAction.name = configObject.GetValue( TEXT( "Name" ) ).GetString();
		if ( inputAction.name.empty() )
		{
			continue;
		}

		// Get buttons
		std::vector< FConfigValue >		configButtons = configObject.GetValue( TEXT( "Buttons" ) ).GetArray();
		for ( uint32 indexButton = 0, countButtons = configButtons.size(); indexButton < countButtons; ++indexButton )
		{
			// Get JSON item
			const FConfigValue& configButton = configButtons[ indexButton ];
			check( configButton.GetType() == FConfigValue::T_String );
			std::wstring		buttonName = configButton.GetString();

			// Get button code from name of button
			EButtonCode			buttonCode = appGetButtonCodeByName( buttonName.c_str() );
			if ( buttonCode == BC_None )
			{
				LE_LOG( LT_Warning, LC_Input, TEXT( "Action '%s': unknown button name '%s'" ), inputAction.name.c_str(), buttonName.c_str() );
				continue;
			}
			inputAction.buttons.push_back( buttonCode );
		}

		// If array of buttons not empty - add to input action map
		if ( !inputAction.buttons.empty() )
		{
			inputActionMap.insert( std::make_pair( inputAction.name, inputAction ) );
		}	
	}

	// Axis
	std::vector< FConfigValue >		configArrayAxis = GInputConfig.GetValue( TEXT( "InputSystem.InputSettings" ), TEXT( "Axis" ) ).GetArray();
	for ( uint32 indexAxis = 0, countAxis = configArrayAxis.size(); indexAxis < countAxis; ++indexAxis )
	{
		// Get JSON object of action item
		const FConfigValue&		configAxis = configArrayAxis[ indexAxis ];
		check( configAxis.GetType() == FConfigValue::T_Object );
		FConfigObject			configObject = configAxis.GetObject();

		// Get name of axis
		FInputAxis			inputAxis;
		inputAxis.name = configObject.GetValue( TEXT( "Name" ) ).GetString();
		if ( inputAxis.name.empty() )
		{
			continue;
		}

		// Get buttons
		std::vector< FConfigValue >		configButtons = configObject.GetValue( TEXT( "Buttons" ) ).GetArray();
		for ( uint32 indexButton = 0, countButtons = configButtons.size(); indexButton < countButtons; ++indexButton )
		{
			// Get JSON item
			const FConfigValue&		configButton = configButtons[ indexButton ];
			check( configAxis.GetType() == FConfigValue::T_Object );
			FConfigObject			configButtonObject = configButton.GetObject();
			
			std::wstring			buttonName = configButtonObject.GetValue( TEXT( "Name" ) ).GetString();
			float					scale = configButtonObject.GetValue( TEXT( "Scale" ) ).GetNumber();

			// Get button code from name of button
			EButtonCode			buttonCode = appGetButtonCodeByName( buttonName.c_str() );
			if ( buttonCode == BC_None )
			{
				LE_LOG( LT_Warning, LC_Input, TEXT( "Axis '%s' with scale %f: unknown button name '%s'" ), inputAxis.name.c_str(), scale, buttonName.c_str() );
				continue;
			}
			inputAxis.buttons.push_back( std::make_pair( buttonCode, scale ) );
		}

		// If array of buttons not empty - add to input axis map
		if ( !inputAxis.buttons.empty() )
		{
			inputAxisMap.insert( std::make_pair( inputAxis.name, inputAxis ) );
		}
	}
}

/**
 * Enumeration of flags for call input action
 */
enum EInputActionFlags
{
	IAF_None		= 0,								/**< No call */
	IAF_Pressed		= 1 << 0,							/**< Call event of pressed */
	IAF_Released	= 1 << 1,							/**< Call event of released */
	IAF_All			= IAF_Pressed | IAF_Released		/**< Need call press and release events */
};

void LInputComponent::TickComponent( float InDeltaTime )
{
	Super::TickComponent( InDeltaTime );
	
	// Find triggered actions and execute it
	for ( FInputActionMap::iterator it = inputActionMap.begin(), itEnd = inputActionMap.end(); it != itEnd; ++it )
	{
		uint32		executeFlags = IAF_None;
		for ( uint32 indexButton = 0, countButtons = it->second.buttons.size(); indexButton < countButtons; ++indexButton )
		{
			switch ( GInputSystem->GetButtonEvent( it->second.buttons[ indexButton ] ) )
			{
			case BE_Pressed:	executeFlags |= IAF_Pressed;	break;
			case BE_Released:	executeFlags |= IAF_Released;	break;
			}

			if ( executeFlags & IAF_All )
			{
				break;
			}
		}

		// If not need execute action - continue to next step
		if ( executeFlags & IAF_None )
		{
			continue;
		}

		// Call delegate for pressed buttons
		if ( executeFlags & IAF_Pressed )
		{
			it->second.inputActionDelegate[ IE_Pressed ].Execute();
		}

		// Call delegate for released buttons
		if ( executeFlags & IAF_Released )
		{
			it->second.inputActionDelegate[ IE_Released ].Execute();
		}
	}

	// Find triggered axis and execute it
	for ( FInputAxisMap::iterator it = inputAxisMap.begin(), itEnd = inputAxisMap.end(); it != itEnd; ++it )
	{
		std::unordered_set< float >		triggeredScales;
		for ( uint32 indexButton = 0, countButtons = it->second.buttons.size(); indexButton < countButtons; ++indexButton )
		{
			const FInputAxis::FPairAxisButton&		pairAxisButton = it->second.buttons[ indexButton ];
			switch ( GInputSystem->GetButtonEvent( pairAxisButton.first ) )
			{
			case BE_Pressed:
			case BE_Released:
			case BE_Scrolled:
				triggeredScales.insert( pairAxisButton.second );
				break;

			default:	continue;
			}
		}

		// Trigger all scales
		for ( auto itTriggerScale = triggeredScales.begin(), itTriggerScaleEnd = triggeredScales.end(); itTriggerScale != itTriggerScaleEnd; ++itTriggerScale )
		{
			it->second.inputAxisDelegate.Execute( *itTriggerScale );
		}
	}
}