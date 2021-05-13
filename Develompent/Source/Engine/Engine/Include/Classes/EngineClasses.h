/*=========================================
 C++ class definitions exported from AngelScript.
 This is automatically generated by the tools.
 DO NOT modify this manually! Edit the corresponding .as files instead!
 BSOD-Games, All Rights Reserved.
==========================================*/

#pragma once

#include <angelscript.h>
#include "Scripts/ScriptVar.h"
#include "Scripts/ScriptObject.h"

// ----------------------------------
// TYPEDEFS
// ----------------------------------

// ----------------------------------
// ENUMS
// ----------------------------------

enum EGameMode
{
	GM_Menu			=0,
	GM_Game			=1
};

// ----------------------------------
// FUNCTIONS
// ----------------------------------

// ----------------------------------
// CLASSES
// ----------------------------------

class OGameInfo : public ScriptObject
{
	DECLARE_CLASS( OGameInfo, ScriptObject )

	//## BEGIN PROPS OGameInfo
public:
	ScriptVar< EGameMode > gameMode;
	//## END PROPS OGameInfo

public:
	OGameInfo()
	{
		asIScriptEngine*		scriptEngine = GScriptEngine->GetASScriptEngine();
		asIScriptModule*		scriptModule = scriptEngine->GetModule( "Engine" );
		asIScriptContext*		scriptContext = scriptEngine->CreateContext();
		check( scriptContext && scriptModule );

		asITypeInfo*			objectType = scriptModule->GetObjectTypeByIndex( 0 );
		check( objectType );

		asIScriptFunction*		factory = objectType->GetFactoryByIndex( 0 );
		check( factory );

		int32					result = scriptContext->Prepare( factory );
		check( result >= 0 );

		result = scriptContext->Execute();
		check( result >= 0 );

		Init( *( asIScriptObject** )scriptContext->GetAddressOfReturnValue() );
	}

	OGameInfo( class asIScriptObject* InScriptObject ) { Init( InScriptObject ); }
	OGameInfo( ENoInit ) {}

	std::string execGetGameName()
	{
		asIScriptEngine*		scriptEngine = GScriptEngine->GetASScriptEngine();
		asIScriptContext*		scriptContext = scriptEngine->CreateContext();
		check( scriptContext );
	
		asIScriptFunction*		function = typeInfo->GetMethodByIndex( 0 );
		check( function );
	
		int32	result = scriptContext->Prepare( function );
		check( result >= 0 );
	
		result = scriptContext->SetObject( self );
		check( result >= 0 );
	
		result = scriptContext->Execute();
		check( result >= 0 );
	
		std::string		returnValue = *( ( std::string* )scriptContext->GetReturnObject() );
		scriptContext->Release();
		return returnValue;
	}

	EGameMode execGetGameMode()
	{
		asIScriptEngine*		scriptEngine = GScriptEngine->GetASScriptEngine();
		asIScriptContext*		scriptContext = scriptEngine->CreateContext();
		check( scriptContext );
	
		asIScriptFunction*		function = typeInfo->GetMethodByIndex( 1 );
		check( function );
	
		int32	result = scriptContext->Prepare( function );
		check( result >= 0 );
	
		result = scriptContext->SetObject( self );
		check( result >= 0 );
	
		result = scriptContext->Execute();
		check( result >= 0 );
	
		EGameMode		returnValue = ( EGameMode )scriptContext->GetReturnDWord();
		scriptContext->Release();
		return returnValue;
	}

protected:
	void Init( asIScriptObject* InScriptObject ) override
	{
		Super::Init( InScriptObject );
		gameMode.Init( 0, self );
	}
};

// ----------------------------------
// GLOBAL VALUES
// ----------------------------------

// ----------------------------------
// INITIALIZATION MACROS
// ----------------------------------

#define DECLARATE_GLOBALVALUES_SCRIPTMODULE_Engine

// ----------------------------------
// INITIALIZATION FUNCTION
// ----------------------------------

void InitScriptModule_Engine()
{
}