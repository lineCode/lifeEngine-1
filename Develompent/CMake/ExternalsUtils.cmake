function( IncludeExternals MODULE_NAME )
	# SDL2
	find_package( SDL2 REQUIRED )
	if ( SDL2_FOUND )
	    include_directories( ${SDL2_INCLUDE} )
		target_link_libraries( ${MODULE_NAME} ${SDL2_LIB} ${SDL2MAIN_LIB} )	
	else()
		message( SEND_ERROR "Failed to find SDL2" )
	endif()
	
	# RapidJSON
	find_package( Rapidjson REQUIRED )
	if ( RAPIDJSON_FOUND )
		include_directories( ${RAPIDJSON_INCLUDE} )
	else()
		message( SEND_ERROR "Failed to find RapidJSON" )
	endif()
	
	# LuaJIT
	find_package( LuaJIT REQUIRED )
	if ( LUAJIT_FOUND )
	    include_directories( ${LUAJIT_INCLUDE} )
		target_link_libraries( ${MODULE_NAME} ${LUAJIT_LIB} )			
	else()
		message( SEND_ERROR "Failed to find LuaJIT" )
	endif()
	
	# LuaBridge
	find_package( LuaBridge REQUIRED )
	if ( LUABRIDGE_FOUND )
		include_directories( ${LUABRIDGE_INCLUDE} )
	else()
		message( SEND_ERROR "Failed to find LuaBridge" )
	endif()
	
	# STB
	find_package( STB REQUIRED )
	if ( STB_FOUND )
		include_directories( ${STB_INCLUDE} )
	else()
		message( SEND_ERROR "Failed to find STB" )
	endif()
	
	# Qt6
	if ( WITH_EDITOR )
		find_package( Qt6 COMPONENTS Widgets REQUIRED )
		target_link_libraries( ${MODULE_NAME} Qt6::Widgets Qt6::Core )
		set( CMAKE_AUTOUIC ON )
		set( CMAKE_AUTOMOC ON )
		set( CMAKE_AUTORCC ON )
	endif()
	
	if ( PLATFORM_WINDOWS )
		target_link_libraries( ${MODULE_NAME} d3d11 dxgi dxguid )
	endif()
endfunction()

function( InstallExternals INSTALL_DIR )
	if ( PLATFORM_WINDOWS )
		if ( PLATFORM_64BIT )
			set( BINARES "${SDL2_PATH}/lib/Win64/SDL2.dll" )
		elseif( PLATFORM_32BIT )
			set( BINARES "${SDL2_PATH}/lib/Win32/SDL2.dll" )
		endif()
	else()
		message( SEND_ERROR "Unknown platform" )
	endif()
	
	install( FILES ${BINARES} DESTINATION ${INSTALL_DIR} )
endfunction()