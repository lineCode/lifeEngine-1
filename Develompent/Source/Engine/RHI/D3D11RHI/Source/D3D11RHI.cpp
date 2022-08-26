#include <d3d9.h>

#include "Core.h"
#include "Logger/LoggerMacros.h"
#include "Render/RenderResource.h"
#include "Render/RenderUtils.h"
#include "Render/GlobalConstantsHelper.h"
#include "Render/SceneUtils.h"
#include "D3D11RHI.h"
#include "D3D11Viewport.h"
#include "D3D11DeviceContext.h"
#include "D3D11Surface.h"
#include "D3D11Shader.h"
#include "D3D11Buffer.h"
#include "D3D11ImGUI.h"
#include "D3D11State.h"

/**
 * Get vertex count for primitive count
 */
static FORCEINLINE uint32 GetVertexCountForPrimitiveCount( uint32 InNumPrimitives, EPrimitiveType InPrimitiveType )
{
	uint32		vertexCount = 0;
	switch ( InPrimitiveType )
	{
	case PT_PointList:			vertexCount = InNumPrimitives;		break;
	case PT_TriangleList:		vertexCount = InNumPrimitives * 3;	break;
	case PT_TriangleStrip:		vertexCount = InNumPrimitives + 2;	break;
	case PT_LineList:			vertexCount = InNumPrimitives * 2;	break;

	default:
		appErrorf( TEXT( "Unknown primitive type: %u" ), ( uint32 )InPrimitiveType );
	}

	return vertexCount;
}

/**
 * Get D3D11 primitive type from engine primitive type
 */
static FORCEINLINE D3D11_PRIMITIVE_TOPOLOGY GetD3D11PrimitiveType( uint32 InPrimitiveType, bool InIsUsingTessellation )
{
	checkMsg( !InIsUsingTessellation, TEXT( "Tessellation not supported!" ) );
	switch ( InPrimitiveType )
	{
	case PT_PointList:			return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	case PT_TriangleList:		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case PT_TriangleStrip:		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case PT_LineList:			return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;

	default: 
		appErrorf( TEXT( "Unknown primitive type: %u" ), ( uint32 )InPrimitiveType );
	};

	return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}

/**
 * Constructor
 */
CD3D11RHI::CD3D11RHI() :
	isInitialize( false ),
	immediateContext( nullptr ),
	globalConstantBuffer( nullptr ),
	psConstantBuffer( nullptr ),
	d3d11Device( nullptr )
{
	appMemzero( vsConstantBuffers, sizeof( vsConstantBuffers ) );
}

/**
 * Destructor
 */
CD3D11RHI::~CD3D11RHI()
{
	Destroy();
}

/**
 * Initialize RHI
 */
void CD3D11RHI::Init( bool InIsEditor )
{
	if ( IsInitialize() )			return;

	uint32					deviceFlags = 0;

	// In Direct3D 11, if you are trying to create a hardware or a software device, set pAdapter != NULL which constrains the other inputs to be:
	//		DriverType must be D3D_DRIVER_TYPE_UNKNOWN 
	//		Software must be NULL. 
	D3D_DRIVER_TYPE			driverType = D3D_DRIVER_TYPE_UNKNOWN;

#if DEBUG
	deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // DEBUG

	// Create DXGI factory and adapter
	HRESULT		result = CreateDXGIFactory( __uuidof( IDXGIFactory ), ( void** ) &dxgiFactory );
	check( result == S_OK );

	uint32			currentAdapter = 0;
	while ( dxgiFactory->EnumAdapters( currentAdapter, &dxgiAdapter ) == DXGI_ERROR_NOT_FOUND )
	{
		++currentAdapter;
	}
	checkMsg( dxgiAdapter, TEXT( "GPU adapter not found" ) );

	D3D_FEATURE_LEVEL				maxFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL				featureLevel;
	ID3D11DeviceContext*			d3d11DeviceContext = nullptr;
	result = D3D11CreateDevice( dxgiAdapter, driverType, nullptr, deviceFlags, &maxFeatureLevel, 1, D3D11_SDK_VERSION, &d3d11Device, &featureLevel, &d3d11DeviceContext );
	check( result == S_OK );

	immediateContext = new CD3D11DeviceContext( d3d11DeviceContext );
	
	// Create constant buffers for shaders
	// Global constant buffer
	globalConstantBuffer = new class CD3D11ConstantBuffer( GConstantBufferSizes[ SOB_GlobalConstants ], TEXT( "GlobalConstantBuffer" ) );
	{
		ID3D11Buffer*		d3d11GlobalConstantBuffer = globalConstantBuffer->GetD3D11Buffer();
		d3d11DeviceContext->VSSetConstantBuffers( SOB_GlobalConstants, 1, &d3d11GlobalConstantBuffer );
		d3d11DeviceContext->PSSetConstantBuffers( SOB_GlobalConstants, 1, &d3d11GlobalConstantBuffer );
		d3d11DeviceContext->GSSetConstantBuffers( SOB_GlobalConstants, 1, &d3d11GlobalConstantBuffer );
		d3d11DeviceContext->CSSetConstantBuffers( SOB_GlobalConstants, 1, &d3d11GlobalConstantBuffer );
	}

	// Vertex constant buffer
	vsConstantBuffers[ SOB_ShaderConstants ] = new CD3D11ConstantBuffer( GConstantBufferSizes[ SOB_ShaderConstants ], TEXT( "ShaderConstantBuffer" ) );
	vsConstantBuffers[ SOB_GlobalConstants ] = nullptr;
	{
		for ( uint32 index = 0, num = ARRAY_COUNT( vsConstantBuffers ); index < num; ++index )
		{
			CD3D11ConstantBuffer*		constantBuffer = vsConstantBuffers[ index ];
			if ( constantBuffer )
			{
				ID3D11Buffer*		d3d11ConstantBuffer = constantBuffer->GetD3D11Buffer();
				d3d11DeviceContext->VSSetConstantBuffers( SOB_ShaderConstants, 1, &d3d11ConstantBuffer );
			}
		}
	}

	// Pixel constant buffer
	psConstantBuffer = new CD3D11ConstantBuffer( GConstantBufferSizes[ SOB_ShaderConstants ], TEXT( "ShaderConstantBuffer" ) );
	{
		ID3D11Buffer*		d3d11ConstantBuffer = psConstantBuffer->GetD3D11Buffer();
		d3d11DeviceContext->PSSetConstantBuffers( SOB_ShaderConstants, 1, &d3d11ConstantBuffer );
	}

	// Print info adapter
	DXGI_ADAPTER_DESC				adapterDesc;
	dxgiAdapter->GetDesc( &adapterDesc );

	LE_LOG( LT_Log, LC_Init, TEXT( "Found D3D11 adapter: %s" ), adapterDesc.Description );
	LE_LOG( LT_Log, LC_Init, TEXT( "Adapter has %uMB of dedicated video memory, %uMB of dedicated system memory, and %uMB of shared system memory" ),
			adapterDesc.DedicatedVideoMemory / ( 1024 * 1024 ),
			adapterDesc.DedicatedSystemMemory / ( 1024 * 1024 ),
			adapterDesc.SharedSystemMemory / ( 1024 * 1024 ) );

	// Initialize the platform pixel format map
	GPixelFormats[ PF_Unknown ].platformFormat					= DXGI_FORMAT_UNKNOWN;
	GPixelFormats[ PF_A8R8G8B8 ].platformFormat					= DXGI_FORMAT_R8G8B8A8_UNORM;
	GPixelFormats[ PF_DepthStencil ].platformFormat				= DXGI_FORMAT_R32G8X24_TYPELESS;
	GPixelFormats[ PF_DepthStencil ].blockBytes					= 8;
	GPixelFormats[ PF_ShadowDepth ].platformFormat				= DXGI_FORMAT_R16_TYPELESS;
	GPixelFormats[ PF_FilteredShadowDepth ].platformFormat		= DXGI_FORMAT_D32_FLOAT;
	GPixelFormats[ PF_D32 ].platformFormat						= DXGI_FORMAT_R32_TYPELESS;

	isInitialize = true;

	// Initialize all global render resources
	std::set< CRenderResource* >&			globalResourceList = CRenderResource::GetResourceList();
	for ( auto it = globalResourceList.begin(), itEnd = globalResourceList.end(); it != itEnd; ++it )
	{
		( *it )->InitResource();
	}
}

/**
 * Acquire thread ownership
 */
void CD3D11RHI::AcquireThreadOwnership()
{}

/**
 * Release thread ownership
 */
void CD3D11RHI::ReleaseThreadOwnership()
{
}

/**
 * Is initialized RHI
 */
bool CD3D11RHI::IsInitialize() const
{
	return isInitialize;
}

/**
 * Destroy RHI
 */
void CD3D11RHI::Destroy()
{
	if ( !isInitialize )		return;

	// Release all global render resources
	std::set< CRenderResource* >&		globalResourceList = CRenderResource::GetResourceList();
	for ( auto it = globalResourceList.begin(), itEnd = globalResourceList.end(); it != itEnd; ++it )
	{
		( *it )->ReleaseResource();
	}

	for ( uint32 index = 0, num = ARRAY_COUNT( vsConstantBuffers ); index < num; ++index )
	{
		CD3D11ConstantBuffer*		constantBuffer = vsConstantBuffers[ index ];
		if ( constantBuffer )
		{
			delete constantBuffer;
		}
	}

	delete globalConstantBuffer;
	delete psConstantBuffer;
	delete immediateContext;
	d3d11Device->Release();
	dxgiAdapter->Release();
	dxgiFactory->Release();

	isInitialize = false;
	globalConstantBuffer = nullptr;
	psConstantBuffer = nullptr;
	immediateContext = nullptr;
	d3d11Device = nullptr;
	dxgiAdapter = nullptr;
	dxgiFactory = nullptr;

	appMemzero( &stateCache, sizeof( SD3D11StateCache ) );
	appMemzero( vsConstantBuffers, sizeof( vsConstantBuffers ) );
}

/**
 * Create viewport
 */
ViewportRHIRef_t CD3D11RHI::CreateViewport( void* InWindowHandle, uint32 InWidth, uint32 InHeight )
{
	return new CD3D11Viewport( InWindowHandle, InWidth, InHeight );
}

/**
 * Create vertex shader
 */
VertexShaderRHIRef_t CD3D11RHI::CreateVertexShader( const tchar* InShaderName, const byte* InData, uint32 InSize )
{
	return new CD3D11VertexShaderRHI( InData, InSize, InShaderName );
}

/**
 * Create hull shader
 */
HullShaderRHIRef_t CD3D11RHI::CreateHullShader( const tchar* InShaderName, const byte* InData, uint32 InSize )
{
	return new CD3D11HullShaderRHI( InData, InSize, InShaderName );
}

/**
 * Create domain shader
 */
DomainShaderRHIRef_t CD3D11RHI::CreateDomainShader( const tchar* InShaderName, const byte* InData, uint32 InSize )
{
	return new CD3D11DomainShaderRHI( InData, InSize, InShaderName );
}

/**
 * Create pixel shader
 */
PixelShaderRHIRef_t CD3D11RHI::CreatePixelShader( const tchar* InShaderName, const byte* InData, uint32 InSize )
{
	return new CD3D11PixelShaderRHI( InData, InSize, InShaderName );
}

/**
 * Create geometry shader
 */
GeometryShaderRHIRef_t CD3D11RHI::CreateGeometryShader( const tchar* InShaderName, const byte* InData, uint32 InSize )
{
	return new CD3D11GeometryShaderRHI( InData, InSize, InShaderName );
}

/**
 * Create vertex buffer
 */
VertexBufferRHIRef_t CD3D11RHI::CreateVertexBuffer( const tchar* InBufferName, uint32 InSize, const byte* InData, uint32 InUsage )
{
	return new CD3D11VertexBufferRHI( InUsage, InSize, InData, InBufferName );
}

/**
 * Create index buffer
 */
IndexBufferRHIRef_t CD3D11RHI::CreateIndexBuffer( const tchar* InBufferName, uint32 InStride, uint32 InSize, const byte* InData, uint32 InUsage )
{
	return new CD3D11IndexBufferRHI( InUsage, InStride, InSize, InData, InBufferName );
}

/**
 * Create vertex declaration
 */
VertexDeclarationRHIRef_t CD3D11RHI::CreateVertexDeclaration( const VertexDeclarationElementList_t& InElementList )
{
	return new CD3D11VertexDeclarationRHI( InElementList );
}

/**
 * Create bound shader state
 */
BoundShaderStateRHIRef_t CD3D11RHI::CreateBoundShaderState( const tchar* InBoundShaderStateName, VertexDeclarationRHIRef_t InVertexDeclaration, VertexShaderRHIRef_t InVertexShader, PixelShaderRHIRef_t InPixelShader, HullShaderRHIRef_t InHullShader /*= nullptr*/, DomainShaderRHIRef_t InDomainShader /*= nullptr*/, GeometryShaderRHIRef_t InGeometryShader /*= nullptr*/ )
{
	CBoundShaderStateKey		key( InVertexDeclaration, InVertexShader, InPixelShader, InHullShader, InDomainShader, InGeometryShader );
	BoundShaderStateRHIRef_t		boundShaderStateRHI = boundShaderStateHistory.Find( key );
	if ( !boundShaderStateRHI )
	{
		boundShaderStateRHI = new CD3D11BoundShaderStateRHI( InBoundShaderStateName, key, InVertexDeclaration, InVertexShader, InPixelShader, InHullShader, InDomainShader, InGeometryShader );
		boundShaderStateHistory.Add( key, boundShaderStateRHI );
	}

	return boundShaderStateRHI;
}

/**
 * Create rasterizer state
 */
RasterizerStateRHIRef_t CD3D11RHI::CreateRasterizerState( const SRasterizerStateInitializerRHI& InInitializer )
{
	return new CD3D11RasterizerStateRHI( InInitializer );
}

SamplerStateRHIRef_t CD3D11RHI::CreateSamplerState( const SSamplerStateInitializerRHI& InInitializer )
{
	return new CD3D11SamplerStateRHI( InInitializer );
}

DepthStateRHIRef_t CD3D11RHI::CreateDepthState( const SDepthStateInitializerRHI& InInitializer )
{
	return new CD3D11DepthStateRHI( InInitializer );
}

Texture2DRHIRef_t CD3D11RHI::CreateTexture2D( const tchar* InDebugName, uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, uint32 InNumMips, uint32 InFlags, void* InData /*= nullptr*/ )
{
	return new CD3D11Texture2DRHI( InDebugName, InSizeX, InSizeY, InNumMips, InFormat, InFlags, InData );
}

SurfaceRHIRef_t CD3D11RHI::CreateTargetableSurface( const tchar* InDebugName, uint32 InSizeX, uint32 InSizeY, EPixelFormat InFormat, Texture2DRHIParamRef_t InResolveTargetTexture, uint32 InFlags )
{
	return new CD3D11Surface( InResolveTargetTexture );
}

/**
 * Get device context
 */
class CBaseDeviceContextRHI* CD3D11RHI::GetImmediateContext() const
{
	return immediateContext;
}

void CD3D11RHI::SetupInstancing( class CBaseDeviceContextRHI* InDeviceContext, uint32 InStreamIndex, void* InInstanceData, uint32 InInstanceStride, uint32 InInstanceSize, uint32 InNumInstances )
{
	if ( !instanceBuffer || instanceBuffer->GetSize() < InInstanceSize )
	{
		instanceBuffer = new CD3D11VertexBufferRHI( RUF_Dynamic, InInstanceSize, ( byte* )InInstanceData, TEXT( "Instance" ) );
	}
	else
	{
		SLockedData		lockedData;
		LockVertexBuffer( InDeviceContext, instanceBuffer, InInstanceSize, 0, lockedData );
		memcpy( lockedData.data, InInstanceData, InInstanceSize );
		UnlockVertexBuffer( InDeviceContext, instanceBuffer, lockedData );
	}

	SetStreamSource( InDeviceContext, InStreamIndex, instanceBuffer, InInstanceStride, 0 );
}

/**
 * Set viewport
 */
void CD3D11RHI::SetViewport( class CBaseDeviceContextRHI* InDeviceContext, uint32 InMinX, uint32 InMinY, float InMinZ, uint32 InMaxX, uint32 InMaxY, float InMaxZ )
{
	D3D11_VIEWPORT			d3d11Viewport = { ( float )InMinX, ( float )InMinY, ( float )InMaxX - InMinX, ( float )InMaxY - InMinY, ( float )InMinZ, InMaxZ };
	if ( d3d11Viewport.Width > 0 && d3d11Viewport.Height > 0 && d3d11Viewport != stateCache.viewport )
	{
		static_cast< CD3D11DeviceContext* >( InDeviceContext )->GetD3D11DeviceContext()->RSSetViewports( 1, &d3d11Viewport );
		stateCache.viewport = d3d11Viewport;
	}
}

/**
 * Set bound shader state
 */
void CD3D11RHI::SetBoundShaderState( class CBaseDeviceContextRHI* InDeviceContext, BoundShaderStateRHIParamRef_t InBoundShaderState )
{
	ID3D11DeviceContext*			d3d11DeviceContext = ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	CD3D11BoundShaderStateRHI*		boundShaderState = ( CD3D11BoundShaderStateRHI* )InBoundShaderState;

	// Bind input layout
	{
		ID3D11InputLayout*			d3d11InputLayout = boundShaderState ? boundShaderState->GetD3D11InputLayout() : nullptr;
		if ( d3d11InputLayout != stateCache.inputLayout )
		{
			d3d11DeviceContext->IASetInputLayout( d3d11InputLayout );
			stateCache.inputLayout = d3d11InputLayout;
		}
	}

	// Bind vertex shader
	{
		CD3D11VertexShaderRHI*		vertexShader		= boundShaderState ? ( CD3D11VertexShaderRHI* )boundShaderState->GetVertexShader().GetPtr() : nullptr;
		ID3D11VertexShader*			d3d11VertexShader	= vertexShader ? vertexShader->GetResource() : nullptr;
		if ( d3d11VertexShader != stateCache.vertexShader )
		{
			d3d11DeviceContext->VSSetShader( d3d11VertexShader, nullptr, 0 );
			stateCache.vertexShader = d3d11VertexShader;
		}
	}

	// Bind pixel shader
	{
		CD3D11PixelShaderRHI*		pixelShader			= boundShaderState ? ( CD3D11PixelShaderRHI* )boundShaderState->GetPixelShader().GetPtr() : nullptr;
		ID3D11PixelShader*			d3d11PixelShader	= pixelShader ? pixelShader->GetResource() : nullptr;
		if ( d3d11PixelShader != stateCache.pixelShader )
		{
			d3d11DeviceContext->PSSetShader( d3d11PixelShader, nullptr, 0 );
			stateCache.pixelShader = d3d11PixelShader;
		}
	}

	// Bind geometry shader
	{
		CD3D11GeometryShaderRHI*	geometryShader		= boundShaderState ? ( CD3D11GeometryShaderRHI* )boundShaderState->GetGeometryShader().GetPtr() : nullptr;
		ID3D11GeometryShader*		d3d11GeometryShader = geometryShader ? geometryShader->GetResource() : nullptr;
		if ( d3d11GeometryShader != stateCache.geometryShader )
		{
			d3d11DeviceContext->GSSetShader( d3d11GeometryShader, nullptr, 0 );
			stateCache.geometryShader = d3d11GeometryShader;
		}
	}

	// Bind hull shader
	{
		CD3D11HullShaderRHI*		hullShader			= boundShaderState ? ( CD3D11HullShaderRHI* )boundShaderState->GetHullShader().GetPtr() : nullptr;
		ID3D11HullShader*			d3d11HullShader		= hullShader ? hullShader->GetResource() : nullptr;
		if ( d3d11HullShader != stateCache.hullShader )
		{
			d3d11DeviceContext->HSSetShader( d3d11HullShader, nullptr, 0 );
			stateCache.hullShader = d3d11HullShader;
		}
	}

	// Bind domain shader
	{
		CD3D11DomainShaderRHI*		domainShader		= boundShaderState ? ( CD3D11DomainShaderRHI* )boundShaderState->GetDomainShader().GetPtr() : nullptr;
		ID3D11DomainShader*			d3d11DomainShader	= domainShader ? domainShader->GetResource() : nullptr;
		if ( d3d11DomainShader != stateCache.domainShader )
		{
			d3d11DeviceContext->DSSetShader( d3d11DomainShader, nullptr, 0 );
			stateCache.domainShader = d3d11DomainShader;
		}
	}
}

/**
 * Set stream source
 */
void CD3D11RHI::SetStreamSource( class CBaseDeviceContextRHI* InDeviceContext, uint32 InStreamIndex, VertexBufferRHIParamRef_t InVertexBuffer, uint32 InStride, uint32 InOffset )
{
	ID3D11DeviceContext*			d3d11DeviceContext		= ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	ID3D11Buffer*					d3d11Buffer				= InVertexBuffer ? ( ( CD3D11VertexBufferRHI* )InVertexBuffer )->GetD3D11Buffer() : nullptr;
	CD3D11StateVertexBuffer			stateVertexBuffer		= { d3d11Buffer, InStride, InOffset };

	if ( stateVertexBuffer != stateCache.vertexBuffers[ InStreamIndex ] )
	{
		d3d11DeviceContext->IASetVertexBuffers( InStreamIndex, 1, &d3d11Buffer, &InStride, &InOffset );
		stateCache.vertexBuffers[ InStreamIndex ] = stateVertexBuffer;
	}
}

/**
 * Set rasterizer state
 */
void CD3D11RHI::SetRasterizerState( class CBaseDeviceContextRHI* InDeviceContext, RasterizerStateRHIParamRef_t InNewState )
{
	ID3D11DeviceContext*			d3d11DeviceContext		= ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	ID3D11RasterizerState*			d3d11RasterizerState	= InNewState ? ( ( CD3D11RasterizerStateRHI* )InNewState )->GetResource() : nullptr;

	if ( d3d11RasterizerState != stateCache.rasterizerState )
	{
		d3d11DeviceContext->RSSetState( d3d11RasterizerState );
		stateCache.rasterizerState = d3d11RasterizerState;
	}
}

void CD3D11RHI::SetSamplerState( class CBaseDeviceContextRHI* InDeviceContext, PixelShaderRHIParamRef_t InPixelShader, SamplerStateRHIParamRef_t InNewState, uint32 InStateIndex )
{
	ID3D11DeviceContext*			d3d11DeviceContext		= ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	ID3D11SamplerState*				d3d11SamplerState		= InNewState ? ( ( CD3D11SamplerStateRHI* )InNewState )->GetResource() : nullptr;
	
	if ( d3d11SamplerState != stateCache.psSamplerStates[ InStateIndex ] )
	{
		d3d11DeviceContext->PSSetSamplers( InStateIndex, 1, &d3d11SamplerState );
		stateCache.psSamplerStates[ InStateIndex ] = d3d11SamplerState;
	}
}

void CD3D11RHI::SetTextureParameter( class CBaseDeviceContextRHI* InDeviceContext, PixelShaderRHIParamRef_t InPixelShader, TextureRHIParamRef_t InTexture, uint32 InTextureIndex )
{
	ID3D11DeviceContext*			d3d11DeviceContext = ( ( CD3D11DeviceContext* ) InDeviceContext )->GetD3D11DeviceContext();
	ID3D11ShaderResourceView*		d3d11ShaderResourceView = InTexture ? ( ( CD3D11TextureRHI* )InTexture )->GetShaderResourceView() : nullptr;		// BS yehor.pohuliaka - Nullptr shader resource view is probably a code mistake

	if ( d3d11ShaderResourceView != stateCache.psShaderResourceViews[ InTextureIndex ] )
	{
		d3d11DeviceContext->PSSetShaderResources( InTextureIndex, 1, &d3d11ShaderResourceView );
		stateCache.psShaderResourceViews[ InTextureIndex ] = d3d11ShaderResourceView;
	}
}

void CD3D11RHI::SetViewParameters( class CBaseDeviceContextRHI* InDeviceContext, class CSceneView& InSceneView )
{
	check( InDeviceContext );

	SGlobalConstantBufferContents			globalContents;
	SetGlobalConstants( globalContents, InSceneView );

	globalConstantBuffer->Update( ( byte* ) &globalContents, 0, sizeof( globalContents ) );
	globalConstantBuffer->CommitConstantsToDevice( ( CD3D11DeviceContext* )InDeviceContext );
}

void CD3D11RHI::SetVertexShaderParameter( class CBaseDeviceContextRHI* InDeviceContext, uint32 InBufferIndex, uint32 InBaseIndex, uint32 InNumBytes, const void* InNewValue )
{
	vsConstantBuffers[ InBufferIndex ]->Update( ( const byte* )InNewValue, InBaseIndex, InNumBytes );
}

void CD3D11RHI::SetPixelShaderParameter( class CBaseDeviceContextRHI* InDeviceContext, uint32 InBufferIndex, uint32 InBaseIndex, uint32 InNumBytes, const void* InNewValue )
{
	psConstantBuffer->Update( ( const byte* )InNewValue, InBaseIndex, InNumBytes );
}

void CD3D11RHI::SetDepthTest( class CBaseDeviceContextRHI* InDeviceContext, DepthStateRHIParamRef_t InNewState )
{
	ID3D11DeviceContext*			d3d11DeviceContext	= ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	ID3D11DepthStencilState*		d3d11DepthState		= InNewState ? ( ( CD3D11DepthStateRHI* )InNewState )->GetResource() : nullptr;

	if ( d3d11DepthState != stateCache.depthStencilState )
	{
		uint32						oldStencilRef = 0;
		ID3D11DepthStencilState*	oldStencilState = nullptr;
		
		d3d11DeviceContext->OMGetDepthStencilState( &oldStencilState, &oldStencilRef );
		d3d11DeviceContext->OMSetDepthStencilState( d3d11DepthState, oldStencilRef );
		stateCache.depthStencilState = d3d11DepthState;
	}
}

void CD3D11RHI::CommitConstants( class CBaseDeviceContextRHI* InDeviceContext )
{
	// Commit vertex shader constants
	for ( uint32 index = 0, num = ARRAY_COUNT( vsConstantBuffers ); index < num; ++index )
	{
		CD3D11ConstantBuffer*		constantBuffer = vsConstantBuffers[ index ];
		if ( constantBuffer )
		{
			constantBuffer->CommitConstantsToDevice( ( CD3D11DeviceContext* )InDeviceContext );
		}
	}

	// Commit pixel shader constants
	psConstantBuffer->CommitConstantsToDevice( ( CD3D11DeviceContext* )InDeviceContext );
}

/**
 * Lock vertex buffer
 */
void CD3D11RHI::LockVertexBuffer( class CBaseDeviceContextRHI* InDeviceContext, const VertexBufferRHIRef_t InVertexBuffer, uint32 InSize, uint32 InOffset, SLockedData& OutLockedData )
{
	check( OutLockedData.data == nullptr && InOffset < InSize );

	D3D11_MAP						writeMode = D3D11_MAP_WRITE_DISCARD;
	D3D11_MAPPED_SUBRESOURCE		mappedSubresource;
	
	static_cast< CD3D11DeviceContext* >( InDeviceContext )->GetD3D11DeviceContext()->Map( static_cast< CD3D11VertexBufferRHI* >( InVertexBuffer.GetPtr() )->GetD3D11Buffer(), 0, writeMode, 0, &mappedSubresource );
	OutLockedData.data			= ( byte* )mappedSubresource.pData + InOffset;
	OutLockedData.pitch			= mappedSubresource.RowPitch;
	OutLockedData.isNeedFree	= false;
}

/**
 * Unlock vertex buffer
 */
void CD3D11RHI::UnlockVertexBuffer( class CBaseDeviceContextRHI* InDeviceContext, const VertexBufferRHIRef_t InVertexBuffer, SLockedData& InLockedData )
{
	check( InLockedData.data );

	static_cast< CD3D11DeviceContext* >( InDeviceContext )->GetD3D11DeviceContext()->Unmap( static_cast< CD3D11VertexBufferRHI* >( InVertexBuffer.GetPtr() )->GetD3D11Buffer(), 0 );
	InLockedData.data = nullptr;
}

/**
 * Lock index buffer
 */
void CD3D11RHI::LockIndexBuffer( class CBaseDeviceContextRHI* InDeviceContext, const IndexBufferRHIRef_t InIndexBuffer, uint32 InSize, uint32 InOffset, SLockedData& OutLockedData )
{
	check( OutLockedData.data == nullptr && InOffset < InSize );

	D3D11_MAP						writeMode = D3D11_MAP_WRITE_DISCARD;
	D3D11_MAPPED_SUBRESOURCE		mappedSubresource;

	static_cast< CD3D11DeviceContext* >( InDeviceContext )->GetD3D11DeviceContext()->Map( static_cast< CD3D11IndexBufferRHI* >( InIndexBuffer.GetPtr() )->GetD3D11Buffer(), 0, writeMode, 0, &mappedSubresource );
	OutLockedData.data = ( byte* )mappedSubresource.pData + InOffset;
	OutLockedData.pitch = mappedSubresource.RowPitch;
	OutLockedData.isNeedFree = false;
}

/**
 * Unlock index buffer
 */
void CD3D11RHI::UnlockIndexBuffer( class CBaseDeviceContextRHI* InDeviceContext, const IndexBufferRHIRef_t InIndexBuffer, SLockedData& InLockedData )
{
	check( InLockedData.data );

	static_cast< CD3D11DeviceContext* >( InDeviceContext )->GetD3D11DeviceContext()->Unmap( static_cast< CD3D11IndexBufferRHI* >( InIndexBuffer.GetPtr() )->GetD3D11Buffer(), 0 );
	InLockedData.data = nullptr;
}

void CD3D11RHI::LockTexture2D( class CBaseDeviceContextRHI* InDeviceContext, Texture2DRHIParamRef_t InTexture, uint32 InMipIndex, bool InIsDataWrite, SLockedData& OutLockedData, bool InIsUseCPUShadow /*= false*/ )
{
	( ( CD3D11Texture2DRHI* )InTexture )->Lock( InDeviceContext, InMipIndex, InIsDataWrite, InIsUseCPUShadow, OutLockedData );
}

void CD3D11RHI::UnlockTexture2D( class CBaseDeviceContextRHI* InDeviceContext, Texture2DRHIParamRef_t InTexture, uint32 InMipIndex, SLockedData& InLockedData )
{
	( ( CD3D11Texture2DRHI* )InTexture )->Unlock( InDeviceContext, InMipIndex, InLockedData );
}

/**
 * Draw primitive
 */
void CD3D11RHI::DrawPrimitive( class CBaseDeviceContextRHI* InDeviceContext, EPrimitiveType InPrimitiveType, uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumInstances /* = 1 */ )
{
	ID3D11DeviceContext*			d3d11DeviceContext = ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	uint32							vertexCount = GetVertexCountForPrimitiveCount( InNumPrimitives, InPrimitiveType );

	// Set primitive topology
	{
		D3D11_PRIMITIVE_TOPOLOGY		d3d11PrimitiveTopology = GetD3D11PrimitiveType( InPrimitiveType, false );
		if ( d3d11PrimitiveTopology != stateCache.primitiveTopology )
		{
			d3d11DeviceContext->IASetPrimitiveTopology( d3d11PrimitiveTopology );
			stateCache.primitiveTopology = d3d11PrimitiveTopology;
		}
	}

	// Draw primitive
	if ( InNumInstances > 1 )
	{
		d3d11DeviceContext->DrawInstanced( vertexCount, InNumInstances, InBaseVertexIndex, 0 );
	}
	else
	{
		d3d11DeviceContext->Draw( vertexCount, InBaseVertexIndex );
	}
}

void CD3D11RHI::DrawIndexedPrimitive( class CBaseDeviceContextRHI* InDeviceContext, class CBaseIndexBufferRHI* InIndexBuffer, EPrimitiveType InPrimitiveType, uint32 InBaseVertexIndex, uint32 InStartIndex, uint32 InNumPrimitives, uint32 InNumInstances /* = 1 */ )
{
	check( InIndexBuffer );
	ID3D11DeviceContext*			d3d11DeviceContext = ( ( CD3D11DeviceContext* )InDeviceContext )->GetD3D11DeviceContext();
	CD3D11IndexBufferRHI*			indexBuffer = ( CD3D11IndexBufferRHI* )InIndexBuffer;

	// Bind index buffer
	{
		const DXGI_FORMAT				format = ( indexBuffer->GetStride() == sizeof( uint16 ) ) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		ID3D11Buffer*					d3d11IndexBuffer = indexBuffer->GetD3D11Buffer();
		CD3D11StateIndexBuffer			stateIndexBuffer = { d3d11IndexBuffer, format, 0 };
		
		if ( stateIndexBuffer != stateCache.indexBuffer )
		{
			d3d11DeviceContext->IASetIndexBuffer( d3d11IndexBuffer, format, 0 );
			stateCache.indexBuffer = stateIndexBuffer;
		}
	}

	// Set primitive topology
	{
		D3D11_PRIMITIVE_TOPOLOGY		d3d11PrimitiveTopology = GetD3D11PrimitiveType( InPrimitiveType, false );
		if ( d3d11PrimitiveTopology != stateCache.primitiveTopology )
		{
			d3d11DeviceContext->IASetPrimitiveTopology( d3d11PrimitiveTopology );
			stateCache.primitiveTopology = d3d11PrimitiveTopology;
		}
	}

	// Draw indexed primitive	
	uint32							indexCount = GetVertexCountForPrimitiveCount( InNumPrimitives, InPrimitiveType );
	if ( InNumInstances > 1 )
	{
		d3d11DeviceContext->DrawIndexedInstanced( indexCount, InNumInstances, InStartIndex, InBaseVertexIndex, 0 );
	}
	else
	{
		d3d11DeviceContext->DrawIndexed( indexCount, InStartIndex, InBaseVertexIndex );
	}
}

void CD3D11RHI::DrawPrimitiveUP( class CBaseDeviceContextRHI* InDeviceContext, EPrimitiveType InPrimitiveType, uint32 InBaseVertexIndex, uint32 InNumPrimitives, const void* InVertexData, uint32 InVertexDataStride, uint32 InNumInstances /* = 1 */ )
{
	uint32										vertexCount		= GetVertexCountForPrimitiveCount( InNumPrimitives, InPrimitiveType );
	TRefCountPtr< CD3D11VertexBufferRHI >		vertexBuffer	= CreateVertexBuffer( TEXT( "DrawPrimitiveUP" ), InVertexDataStride * vertexCount, ( const byte* )InVertexData, RUF_Static );
	
	SetStreamSource( InDeviceContext, 0, vertexBuffer, InVertexDataStride, 0 );
	DrawPrimitive( InDeviceContext, InPrimitiveType, InBaseVertexIndex, InNumPrimitives, InNumInstances );
}

void CD3D11RHI::DrawIndexedPrimitiveUP( class CBaseDeviceContextRHI* InDeviceContext, EPrimitiveType InPrimitiveType, uint32 InBaseVertexIndex, uint32 InNumPrimitives, uint32 InNumVertices, const void* InIndexData, uint32 InIndexDataStride, const void* InVertexData, uint32 InVertexDataStride, uint32 InNumInstances /* = 1 */ )
{
	uint32										indexCount		= GetVertexCountForPrimitiveCount( InNumPrimitives, InPrimitiveType );
	TRefCountPtr< CD3D11VertexBufferRHI >		vertexBuffer	= CreateVertexBuffer( TEXT( "DrawIndexedPrimitiveUP" ), InVertexDataStride * InNumVertices, ( const byte* )InVertexData, RUF_Static );
	TRefCountPtr< CD3D11IndexBufferRHI >		indexBuffer		= CreateIndexBuffer( TEXT( "DrawIndexedPrimitiveUP" ), InIndexDataStride, InIndexDataStride * indexCount, ( const byte* )InIndexData, RUF_Static );

	SetStreamSource( InDeviceContext, 0, vertexBuffer, InVertexDataStride, 0 );
	DrawIndexedPrimitive( InDeviceContext, indexBuffer, InPrimitiveType, InBaseVertexIndex, 0, InNumPrimitives, InNumInstances );
}

/**
 * Begin drawing viewport
 */
void CD3D11RHI::BeginDrawingViewport( class CBaseDeviceContextRHI* InDeviceContext, class CBaseViewportRHI* InViewport )
{
	check( InDeviceContext && InViewport );
	CD3D11DeviceContext*			deviceContext = ( CD3D11DeviceContext* )InDeviceContext;
	CD3D11Viewport*					viewport = ( CD3D11Viewport* )InViewport;
	check( viewport );

#if FRAME_CAPTURE_MARKERS
	BeginDrawEvent( InDeviceContext, DEC_SCENE_ITEMS, TEXT( "Viewport" ) );
#endif // FRAME_CAPTURE_MARKERS

	// Clear state cache
	appMemzero( &stateCache, sizeof( SD3D11StateCache ) );

	SetRenderTarget( InDeviceContext, viewport->GetSurface(), nullptr );
	SetViewport( InDeviceContext, 0, 0, 0.f, viewport->GetWidth(), viewport->GetHeight(), 1.f );
}

void CD3D11RHI::SetRenderTarget( class CBaseDeviceContextRHI* InDeviceContext, SurfaceRHIParamRef_t InNewRenderTarget, SurfaceRHIParamRef_t InNewDepthStencilTarget )
{
	check( InDeviceContext );
	CD3D11DeviceContext*			deviceContext				= ( CD3D11DeviceContext* )InDeviceContext;
	CD3D11Surface*					renderTarget				= ( CD3D11Surface* )InNewRenderTarget;
	CD3D11Surface*					depthStencilTarget			= ( CD3D11Surface* )InNewDepthStencilTarget;

	// Set render target
	{
		ID3D11RenderTargetView*		d3d11RenderTargetView		= renderTarget ? renderTarget->GetRenderTargetView() : nullptr;
		ID3D11DepthStencilView*		d3d11DepthStencilView		= depthStencilTarget ? depthStencilTarget->GetDepthStencilView() : nullptr;
		
		if ( d3d11RenderTargetView != stateCache.renderTargetViews[ 0 ] || d3d11DepthStencilView != stateCache.depthStencilView )
		{
			deviceContext->GetD3D11DeviceContext()->OMSetRenderTargets( 1, &d3d11RenderTargetView, d3d11DepthStencilView );
			stateCache.renderTargetViews[ 0 ] = d3d11RenderTargetView;
			stateCache.depthStencilView = d3d11DepthStencilView;
		}
	}
}

void CD3D11RHI::SetMRTRenderTarget( class CBaseDeviceContextRHI* InDeviceContext, SurfaceRHIParamRef_t InNewRenderTarget, uint32 InTargetIndex )
{
	check( InDeviceContext && InTargetIndex < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT );
	CD3D11DeviceContext*	deviceContext	= ( CD3D11DeviceContext* )InDeviceContext;
	CD3D11Surface*			renderTarget	= ( CD3D11Surface* )InNewRenderTarget;

	// Set render target
	{
		ID3D11RenderTargetView*		d3d11RenderTargetView	= renderTarget ? renderTarget->GetRenderTargetView() : nullptr;
		if ( d3d11RenderTargetView != stateCache.renderTargetViews[InTargetIndex] )
		{
			stateCache.renderTargetViews[InTargetIndex] = d3d11RenderTargetView;
			deviceContext->GetD3D11DeviceContext()->OMSetRenderTargets( D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, &stateCache.renderTargetViews[0], stateCache.depthStencilView );
		}
	}
}

/**
 * End drawing viewport
 */
void CD3D11RHI::EndDrawingViewport( class CBaseDeviceContextRHI* InDeviceContext, class CBaseViewportRHI* InViewport, bool InIsPresent, bool InLockToVsync )
{
	check( InViewport );

#if FRAME_CAPTURE_MARKERS
	EndDrawEvent( InDeviceContext );
#endif // FRAME_CAPTURE_MARKERS

	if ( InIsPresent )
	{
		InViewport->Present( InLockToVsync );
	}
}

#if WITH_EDITOR
#include <d3dcompiler.h>
#include <string>

#include "Containers/StringConv.h"
#include "Misc/CoreGlobals.h"
#include "Misc/Misc.h"
#include "System/Archive.h"
#include "System/BaseFileSystem.h"

/**
 * @ingroup D3D11RHI
 * An implementation of the D3DX include interface to access a SShaderCompilerEnvironment
 */
class CD3D11IncludeEnvironment : public ID3DInclude
{
public:
	/**
	 * Constructor
	 * 
	 * @param[in] InEnvironment Shader compiler environment
	 */
	CD3D11IncludeEnvironment( const SShaderCompilerEnvironment& InEnvironment ) :
		environment( InEnvironment )
	{}

	/**
	 * Open file
	 * 
	 * @param[in] InType Include type
	 * @param[in] InName File name
	 * @param[in] InParentData ???
	 * @param[Out] OutData Readed data from file
	 * @param[Out] OutBytes Count readed bytes from file
	 */
	STDMETHOD( Open )( D3D_INCLUDE_TYPE InType, LPCSTR InName, LPCVOID InParentData, LPCVOID* OutData, UINT* OutBytes )
	{
		std::wstring		filename( ANSI_TO_TCHAR( InName ) );

		// For including 'VertexFactory.hlsl' we take path from Environment
		if ( filename == TEXT( "VertexFactory.hlsl" ) )
		{
			filename = appShaderDir() + TEXT( "VertexFactory/" ) + environment.vertexFactoryFileName;
		}
		// Else we search in root shader dir
		else
		{
			filename = appShaderDir() + filename;
		}

		CArchive*		archive = GFileSystem->CreateFileReader( filename );
		if ( !archive )
		{
			LE_LOG( LT_Error, LC_Shader, TEXT( "Not found included shader file '%s'" ), filename.c_str() );
			return E_FAIL;
		}

		// Create data buffer and fill '\0'
		*OutBytes = archive->GetSize() + 1;
		byte*		data = new byte[ *OutBytes ];
		appMemzero( data, *OutBytes );

		// Serialize data to buffer
		archive->Serialize( data, *OutBytes );

		*OutData = ( LPCVOID )data;
		delete archive;
		return S_OK;
	}

	/**
	 * Close file
	 * 
	 * @param[in] InData Readed data from file
	 */
	STDMETHOD( Close )( LPCVOID InData )
	{
		delete[] InData;
		return S_OK;
	}

private:
	SShaderCompilerEnvironment			environment;		/**< Shader compiler environment */
};

/**
 * TranslateCompilerFlag - Translates the platform-independent compiler flags into D3DX defines
 * @param[in] CompilerFlag - The platform-independent compiler flag to translate
 * @return DWORD - The value of the appropriate D3DX enum
 */
static DWORD TranslateCompilerFlagD3D11( ECompilerFlags CompilerFlag )
{
	switch ( CompilerFlag )
	{
	case CF_PreferFlowControl:			return D3D10_SHADER_PREFER_FLOW_CONTROL;
	case CF_Debug:						return D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION;
	case CF_AvoidFlowControl:			return D3D10_SHADER_AVOID_FLOW_CONTROL;
	default:							return 0;
	};
}

/**
 * Compile shader
 */
bool CD3D11RHI::CompileShader( const tchar* InSourceFileName, const tchar* InFunctionName, EShaderFrequency InFrequency, const SShaderCompilerEnvironment& InEnvironment, SShaderCompilerOutput& OutOutput, bool InDebugDump /* = false */, const tchar* InShaderSubDir /* = TEXT( "" ) */ )
{
	CArchive*		shaderArchive = GFileSystem->CreateFileReader( InSourceFileName );
	if ( !shaderArchive )
	{
		return false;
	}

	// Create string buffer and fill '\0'
	uint32				archiveSize = shaderArchive->GetSize() + 1;
	byte*				buffer = new byte[ archiveSize ];
	memset( buffer, '\0', archiveSize );

	// Serialize data to string buffer
	shaderArchive->Serialize( buffer, archiveSize );

	// Getting shader profile
	std::string			shaderProfile;
	switch ( InFrequency )
	{
	case SF_Vertex:
		shaderProfile = "vs_5_0";
		break;

	case SF_Hull:
		shaderProfile = "hs_5_0";
		break;

	case SF_Domain:
		shaderProfile = "ds_5_0";
		break;

	case SF_Geometry:
		shaderProfile = "gs_5_0";
		break;

	case SF_Pixel:
		shaderProfile = "ps_5_0";
		break;

	case SF_Compute:
		shaderProfile = "cs_5_0";
		break;

	default:
		appErrorf( TEXT( "Unknown shader frequency %i" ), InFrequency );
		return false;
	}

	// Translate the input environment's definitions to D3DXMACROs
	std::vector< D3D_SHADER_MACRO >				macros;
	for ( auto it = InEnvironment.difinitions.begin(), itEnd = InEnvironment.difinitions.end(); it != itEnd; ++it )
	{
		const std::wstring&			name = it->first;
		const std::wstring&			difinition = it->second;

		// Create temp C-string of name and value difinition
		achar*						tName = new achar[ name.size() + 1 ];
		achar*						tDifinition = new achar[ difinition.size() + 1 ];
		strncpy( tName, TCHAR_TO_ANSI( name.c_str() ), name.size() + 1 );
		strncpy( tDifinition, TCHAR_TO_ANSI( difinition.c_str() ), difinition.size() + 1 );

		D3D_SHADER_MACRO			d3dMacro;
		d3dMacro.Name = tName;
		d3dMacro.Definition = tDifinition;
		macros.push_back( d3dMacro );
	}

	// Terminate the Macros list
	macros.push_back( D3D_SHADER_MACRO{ nullptr, nullptr } );

	// Getting compile flags
	DWORD				compileFlags = D3D10_SHADER_OPTIMIZATION_LEVEL3;
	for ( uint32 indexFlag = 0, countFlags = ( uint32 )InEnvironment.compilerFlags.size(); indexFlag < countFlags; ++indexFlag )
	{
		// Accumulate flags set by the shader
		compileFlags |= TranslateCompilerFlagD3D11( InEnvironment.compilerFlags[ indexFlag ] );
	}

	CD3D11IncludeEnvironment		includeEnvironment( InEnvironment );
	ID3DBlob*						shaderBlob = nullptr;
	ID3DBlob*						errorsBlob = nullptr;
	HRESULT							result = D3DCompile( buffer, archiveSize, TCHAR_TO_ANSI( InSourceFileName ), macros.data(), &includeEnvironment, TCHAR_TO_ANSI( InFunctionName ), shaderProfile.c_str(), compileFlags, 0, &shaderBlob, &errorsBlob );
	if ( FAILED( result ) )
	{
		// Copy the error text to the output.
		void*		errorBuffer = errorsBlob ? errorsBlob->GetBufferPointer() : nullptr;
		if ( errorBuffer )
		{
			LE_LOG( LT_Error, LC_Shader, ANSI_TO_TCHAR( errorBuffer ) );
			errorsBlob->Release();
		}
		else
		{
			LE_LOG( LT_Error, LC_Shader, TEXT( "Compile Failed without warnings!" ) );
		}

		return false;
	}

	// Save code of shader and getting reflector of shader
	uint32		numShaderBytes = ( uint32 )shaderBlob->GetBufferSize();
	OutOutput.code.resize( numShaderBytes );
	memcpy( OutOutput.code.data(), shaderBlob->GetBufferPointer(), numShaderBytes );

	ID3D11ShaderReflection*				reflector = nullptr;
	result = D3DReflect( OutOutput.code.data(), OutOutput.code.size(), IID_ID3D11ShaderReflection, ( void** ) &reflector );
	check( result == S_OK );

	// Read the constant table description.
	D3D11_SHADER_DESC					shaderDesc;
	reflector->GetDesc( &shaderDesc );

	// Add parameters for shader resources (constant buffers, textures, samplers, etc)
	for ( uint32 resourceIndex = 0; resourceIndex < shaderDesc.BoundResources; ++resourceIndex )
	{
		D3D11_SHADER_INPUT_BIND_DESC 		bindDesc;
		reflector->GetResourceBindingDesc( resourceIndex, &bindDesc );

		if ( bindDesc.Type == D3D10_SIT_CBUFFER || bindDesc.Type == D3D10_SIT_TBUFFER )
		{
			const uint32 								cbIndex = bindDesc.BindPoint;
			ID3D11ShaderReflectionConstantBuffer* 		constantBuffer = reflector->GetConstantBufferByName( bindDesc.Name );
			D3D11_SHADER_BUFFER_DESC 					cbDesc;
			constantBuffer->GetDesc( &cbDesc );

			if ( cbDesc.Size > GConstantBufferSizes[ cbIndex ] )
			{
				appErrorf( TEXT( "Set GConstantBufferSizes[%d] to >= %d" ), cbIndex, cbDesc.Size) ;
			}

			// Track all of the variables in this constant buffer
			for ( uint32 constantIndex = 0; constantIndex < cbDesc.Variables; ++constantIndex )
			{
				ID3D11ShaderReflectionVariable* 		variable = constantBuffer->GetVariableByIndex( constantIndex );
				D3D11_SHADER_VARIABLE_DESC 				variableDesc;
				variable->GetDesc( &variableDesc );

				if ( variableDesc.uFlags & D3D10_SVF_USED )
				{
					OutOutput.parameterMap.AddParameterAllocation(
						ANSI_TO_TCHAR( variableDesc.Name ),
						cbIndex,
						variableDesc.StartOffset,
						variableDesc.Size,
						0
						);
				}
				else
				{
					LE_LOG( LT_Warning, LC_Shader, TEXT( "Found Unused shader parameter: %s at offset: %i size: %i" ), ANSI_TO_TCHAR( variableDesc.Name ), variableDesc.StartOffset, variableDesc.Size );
				}
			}
		}
		else if ( bindDesc.Type == D3D10_SIT_TEXTURE )
		{
			std::wstring		officialName = ANSI_TO_TCHAR( bindDesc.Name );
			uint32				bindCount = 1;

			// Assign the name and optionally strip any "[#]" suffixes
			std::size_t			bracketLocation = officialName.find( TEXT( '[' ) );
			if ( bracketLocation != std::wstring::npos )
			{
				officialName.erase( bracketLocation, officialName.size() );
				const uint32 numCharactersBeforeArray	= officialName.size();

				// In SM5, for some reason, array suffixes are included in Name, i.e. "LightMapTextures[0]", rather than "LightMapTextures"
				// Additionally elements in an array are listed as SEPERATE bound resources.
				// However, they are always contiguous in resource index, so iterate over the samplers and textures of the initial association
				// and count them, identifying the bindpoint and bindcounts
				while ( resourceIndex + 1 < shaderDesc.BoundResources )
				{
					D3D11_SHADER_INPUT_BIND_DESC		bindDesc2;
					reflector->GetResourceBindingDesc( resourceIndex + 1, &bindDesc2 );

					if ( bindDesc2.Type == D3D10_SIT_TEXTURE && strncmp( bindDesc2.Name, bindDesc.Name, numCharactersBeforeArray ) == 0 )
					{
						// Skip over this resource since it is part of an array
						bindCount++;						
						resourceIndex++;
					}
					else
					{
						break;
					}
				}
			}

			// Add a parameter for the texture only, the sampler index will be invalid
			OutOutput.parameterMap.AddParameterAllocation(
				officialName.c_str(),
				0,
				bindDesc.BindPoint,
				bindCount,
				-1
			);
		}
		else if ( bindDesc.Type == D3D11_SIT_UAV_RWTYPED || bindDesc.Type >= D3D11_SIT_STRUCTURED )
		{
			// TODO BS yehor.pohuliaka: Arrays are not yet supported
			uint32		bindCount = 1;

			OutOutput.parameterMap.AddParameterAllocation(
				ANSI_TO_TCHAR( bindDesc.Name ),
				0,
				bindDesc.BindPoint,
				bindCount,
				-1
			);
		}
		else if ( bindDesc.Type == D3D10_SIT_SAMPLER )
		{ 
			std::wstring		officialName = ANSI_TO_TCHAR( bindDesc.Name );
			uint32				bindCount = 1;

			// Assign the name and optionally strip any "[#]" suffixes
			std::size_t			bracketLocation = officialName.find( TEXT( '[' ) );
			if ( bracketLocation != std::wstring::npos )
			{
				officialName.erase( bracketLocation, officialName.size() );
				const uint32	numCharactersBeforeArray = officialName.size();

				// In SM5, for some reason, array suffixes are included in Name, i.e. "LightMapTextures[0]", rather than "LightMapTextures"
				// Additionally elements in an array are listed as SEPERATE bound resources.
				// However, they are always contiguous in resource index, so iterate over the samplers and textures of the initial association
				// and count them, identifying the bindpoint and bindcounts
				while ( resourceIndex + 1 < shaderDesc.BoundResources )
				{
					D3D11_SHADER_INPUT_BIND_DESC	bindDesc2;
					reflector->GetResourceBindingDesc( resourceIndex + 1, &bindDesc2 );

					if ( bindDesc2.Type == D3D10_SIT_SAMPLER && strncmp( bindDesc2.Name, bindDesc.Name, numCharactersBeforeArray ) == 0 )
					{
						// Skip over this resource since it is part of an array
						bindCount++;
						resourceIndex++;
					}
					else
					{
						break;
					}
				}
			}

			// Add a parameter for the sampler only, the texture index will be invalid
			OutOutput.parameterMap.AddParameterAllocation(
				officialName.c_str(),
				0,
				-1,
				bindCount,
				bindDesc.BindPoint
			);
		}
	}

	// Set the number of instructions
	OutOutput.numInstructions = shaderDesc.InstructionCount;

	// Reflector is a com interface, so it needs to be released.
	reflector->Release();

	// We don't need the reflection data anymore, and it just takes up space
	if ( !InDebugDump )
	{
		ID3DBlob*		strippedShader = nullptr;
		result = D3DStripShader( OutOutput.code.data(), OutOutput.code.size(), D3DCOMPILER_STRIP_REFLECTION_DATA | D3DCOMPILER_STRIP_DEBUG_INFO | D3DCOMPILER_STRIP_TEST_BLOBS, &strippedShader );
		if ( result == S_OK )
		{
			uint32		numShaderBytes = strippedShader->GetBufferSize();

			OutOutput.code.resize( numShaderBytes );
			memcpy( OutOutput.code.data(), strippedShader->GetBufferPointer(), numShaderBytes );
			strippedShader->Release();
		}
	}

	// Free temporary strings allocated for the macros
	for ( uint32 indexMacro = 0, countMacros = ( uint32 ) macros.size(); indexMacro < countMacros; ++indexMacro )
	{
		D3D_SHADER_MACRO&			macro = macros[ indexMacro ];
		delete[] macro.Name;
		delete[] macro.Definition;
	}

	shaderBlob->Release();
	delete[] buffer;
	delete shaderArchive;
	return true;
}
#endif // WITH_EDITOR

EShaderPlatform CD3D11RHI::GetShaderPlatform() const
{
	return SP_PCD3D_SM5;
}

#if WITH_IMGUI
/**
 * Initialize ImGUI
 */
void CD3D11RHI::InitImGUI( class CBaseDeviceContextRHI* InDeviceContext )
{
	check( d3d11Device && InDeviceContext );
	CD3D11DeviceContext* deviceContext = ( CD3D11DeviceContext* ) InDeviceContext;

	bool		result = ImGui_ImplDX11_Init( d3d11Device, deviceContext->GetD3D11DeviceContext() );
	check( result );
	ImGui_ImplDX11_NewFrame();
}

/**
 * Shutdown render of ImGUI
 */
void CD3D11RHI::ShutdownImGUI( class CBaseDeviceContextRHI* InDeviceContext )
{
	ImGui_ImplDX11_Shutdown();
}

/**
 * Draw ImGUI
 */
void CD3D11RHI::DrawImGUI( class CBaseDeviceContextRHI* InDeviceContext, ImDrawData* InImGUIDrawData )
{
	ImGui_ImplDX11_RenderDrawData( InImGUIDrawData );
}
#endif // WITH_IMGUI

#if FRAME_CAPTURE_MARKERS
void CD3D11RHI::BeginDrawEvent( class CBaseDeviceContextRHI* InDeviceContext, const CColor& InColor, const tchar* InName )
{
	D3DPERF_BeginEvent( D3DCOLOR_RGBA( InColor.r, InColor.g, InColor.b, InColor.a ), InName );
}

void CD3D11RHI::EndDrawEvent( class CBaseDeviceContextRHI* InDeviceContext )
{
	D3DPERF_EndEvent();
}
#endif // FRAME_CAPTURE_MARKERS

/**
 * Get RHI name
 */
const tchar* CD3D11RHI::GetRHIName() const
{
	return TEXT( "D3D11RHI" );
}

/**
 * Set debug name fore DirectX 11 resource
 */
void D3D11SetDebugName( ID3D11DeviceChild* InObject, const achar* InName )
{
	if ( !InName )			return;
	InObject->SetPrivateData( WKPDID_D3DDebugObjectName, ( uint32 )strlen( InName ), InName );
}

/**
 * Set debug name fore DirectX 11 resource
 */
void D3D11SetDebugName( IDXGIObject* InObject, const achar* InName )
{
	if ( !InName )			return;
	InObject->SetPrivateData( WKPDID_D3DDebugObjectName, ( uint32 )strlen( InName ), InName );
}