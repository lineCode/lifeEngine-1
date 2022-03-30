#include "Render/Shaders/ShaderCache.h"
#include "System/Archive.h"

#define SHADER_CACHE_VERSION			4

bool FShaderParameterMap::FindParameterAllocation( const tchar* InParameterName, uint32& OutBufferIndex, uint32& OutBaseIndex, uint32& OutSize, uint32& OutSamplerIndex ) const
{
	auto		itParamAllocation = parameterMap.find( InParameterName );
	if ( itParamAllocation != parameterMap.end() )
	{
		const FParameterAllocation&		allocation = itParamAllocation->second;
		OutBufferIndex = allocation.bufferIndex;
		OutBaseIndex = allocation.baseIndex;
		OutSize = allocation.size;
		OutSamplerIndex = allocation.samplerIndex;
		allocation.isBound = true;
		return true;
	}
	else
	{
		return false;
	}
}

void FShaderParameterMap::AddParameterAllocation( const tchar* InParameterName, uint32 InBufferIndex, uint32 InBaseIndex, uint32 InSize, uint32 InSamplerIndex )
{
	FParameterAllocation 		allocation;
	allocation.bufferIndex = InBufferIndex;
	allocation.baseIndex = InBaseIndex;
	allocation.size = InSize;
	allocation.samplerIndex = InSamplerIndex;
	parameterMap[ InParameterName ] = allocation;
}

/**
 * Serialize of FShaderCacheItem
 */
void FShaderCache::FShaderCacheItem::Serialize( FArchive& InArchive )
{
	InArchive << frequency;

	if ( InArchive.Ver() < VER_HashUInt64 )
	{
		uint32		tmpVertexFactoryHash = vertexFactoryHash;
		InArchive << tmpVertexFactoryHash;
		vertexFactoryHash = tmpVertexFactoryHash;
	}
	else
	{
		InArchive << vertexFactoryHash;
	}

	InArchive << numInstructions;
	InArchive << name;

	if ( InArchive.Ver() >= VER_ShaderParameterMap )
	{
		InArchive << parameterMap;
	}

	if ( InArchive.Ver() < VER_CompressedZlib )
	{
		std::vector<byte>		tmpCode;
		InArchive << tmpCode;
		code = tmpCode;
	}
	else
	{
		InArchive << code;
	}
}

/**
 * Serialize
 */
void FShaderCache::Serialize( FArchive& InArchive )
{
	check( InArchive.Type() == AT_ShaderCache );

	if ( InArchive.IsLoading() )
	{
		// Check version of shader cache
		uint32			shaderCacheVersion = 0;
		InArchive << shaderCacheVersion;
		if ( shaderCacheVersion != SHADER_CACHE_VERSION )
		{
			appErrorf( TEXT( "Not supported version of shader cache. In archive version %i, need %i" ), shaderCacheVersion, SHADER_CACHE_VERSION );
			return;
		}

		uint32			countItems = 0;
		InArchive << countItems;
		items.resize( countItems );

		// Loading all items from archive
		for ( uint32 indexItem = 0; indexItem < countItems; ++indexItem )
		{
			FShaderCacheItem&		item = items[ indexItem ];
			item.Serialize( InArchive );
			itemsMap[ item.vertexFactoryHash ].insert( item.name );
		}
	}
	else if ( InArchive.IsSaving() )
	{
		uint32		countItems = ( uint32 )items.size();
		InArchive << SHADER_CACHE_VERSION;
		InArchive << countItems;

		// Save all items to archive
		for ( uint32 indexItem = 0; indexItem < countItems; ++indexItem )
		{
			items[ indexItem ].Serialize( InArchive );
		}
	}
}