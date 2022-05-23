#include <qcheckbox.h>
#include <qformlayout.h>
#include <qcombobox.h>
#include <qlabel.h>

#include "ui_StaticMeshEditorWindow.h"
#include "Misc/WorldEdGlobals.h"
#include "System/AssetDataBase.h"
#include "Windows/StaticMeshEditorWindow.h"
#include "Widgets/SectionWidget.h"
#include "Render/StaticMeshPreviewViewportClient.h"
#include "Render/RenderingThread.h"
#include "Widgets/SelectAssetWidget.h"
#include "WorldEd.h"

#include <vector>
#include "Misc/SharedPointer.h"
#include "System/Package.h"

WeStaticMeshEditorWindow::WeStaticMeshEditorWindow( const TSharedPtr<FStaticMesh>& InStaticMesh, QWidget* InParent /* = nullptr */ )
	: QWidget( InParent )
	, bInit( false )
	, ui( new Ui::WeStaticMeshEditorWindow() )
	, staticMesh( InStaticMesh )
	, viewportClient( nullptr )
	, assetsCanDeleteHandle( nullptr )
	, assetsReloadedHandle( nullptr )
	, label_sourceFileValue( nullptr )
	, toolButton_sourceFile( nullptr )
	, toolButton_sourceFileRemove( nullptr )
{
	check( InStaticMesh );

	// Init Qt UI
	ui->setupUi( this );
	InitUI();

	// Init preview viewport
	viewportClient = new FStaticMeshPreviewViewportClient( InStaticMesh );
	ui->viewportPreview->SetViewportClient( viewportClient, false );
	ui->viewportPreview->SetEnabled( true );
	bInit = true;

	// Subscribe to event when assets try destroy of editing static mesh and reload. It need is block
	assetsCanDeleteHandle	= FEditorDelegates::onAssetsCanDelete.Add(	std::bind( &WeStaticMeshEditorWindow::OnAssetsCanDelete, this, std::placeholders::_1, std::placeholders::_2 )	);
	assetsReloadedHandle	= FEditorDelegates::onAssetsReloaded.Add(	std::bind( &WeStaticMeshEditorWindow::OnAssetsReloaded, this, std::placeholders::_1 )							);
}

void WeStaticMeshEditorWindow::InitUI()
{
	// Section of materials
	{
		WeSectionWidget*	materialsSection	= new WeSectionWidget( "Materials", 300, this );
		QGridLayout*		gridLayout			= new QGridLayout( materialsSection );
		gridLayout->setContentsMargins( 3, 3, 3, 3 );
		gridLayout->setVerticalSpacing( 1 );

		for ( uint32 index = 0, numMaterials = staticMesh->GetNumMaterials(); index < numMaterials; ++index )
		{
			WeSelectAssetWidget*		selectAsset = new WeSelectAssetWidget( index, materialsSection );
			gridLayout->addWidget( new QLabel( QString::number( index+1 ) + ":", materialsSection ), index, 0 );
			gridLayout->addWidget( selectAsset, index, 1 );

			// Add to array and connect to slot
			connect( selectAsset, SIGNAL( OnAssetReferenceChanged( uint32, const std::wstring& ) ), this, SLOT( OnSelectedAssetMaterial( uint32, const std::wstring& ) ) );
			selectAsset_materials.push_back( selectAsset );
		}
		
		materialsSection->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );
		materialsSection->setContentLayout( *gridLayout );
		materialsSection->expand( true );
		ui->frame->layout()->addWidget( materialsSection );
	}
	
	// Section of file path
	{
		WeSectionWidget*	filePathSection		= new WeSectionWidget( "File Path", 300, this );
		QGridLayout*		gridLayout			= new QGridLayout( filePathSection );
		QLabel*				label_sourceFile	= new QLabel( "Source File:", filePathSection );

		label_sourceFileValue					= new QLabel( "%Value%", filePathSection );
		toolButton_sourceFile					= new QToolButton( filePathSection );
		toolButton_sourceFileRemove				= new QToolButton( filePathSection );

		toolButton_sourceFile->setText( "..." );
		toolButton_sourceFileRemove->setText( "X" );
		gridLayout->setContentsMargins( 3, 3, 3, 3 );

		gridLayout->addWidget( label_sourceFile, 0, 0 );
		gridLayout->addWidget( label_sourceFileValue, 0, 1 );
		gridLayout->addWidget( toolButton_sourceFile, 0, 2 );
		gridLayout->addWidget( toolButton_sourceFileRemove, 0, 3 );

		connect( toolButton_sourceFile, SIGNAL( clicked() ), this, SLOT( on_toolButton_sourceFile_clicked() ) );
		connect( toolButton_sourceFileRemove, SIGNAL( clicked() ), this, SLOT( on_toolButton_sourceFileRemove_clicked() ) );

		filePathSection->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Preferred );
		filePathSection->setContentLayout( *gridLayout );
		filePathSection->expand( true );
		ui->frame->layout()->addWidget( filePathSection );
	}

	// Spacer
	{
		QSpacerItem*		verticalSpacer = new QSpacerItem( 20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding );
		ui->frame->layout()->addItem( verticalSpacer );
	}

	// Update data in UI
	UpdateUI();
}

void WeStaticMeshEditorWindow::UpdateUI()
{
	// Update info about materials
	const std::vector< TAssetHandle<FMaterial> >&		materials = staticMesh->GetMaterials();
	for ( uint32 index = 0, count = materials.size(); index < count; ++index )
	{
		WeSelectAssetWidget*		selectAssetWidget = selectAsset_materials[ index ];
		check( selectAssetWidget );

		TAssetHandle<FMaterial>		material = materials[ index ];
		if ( material.IsAssetValid() && !GPackageManager->IsDefaultAsset( material ) )
		{
			std::wstring		assetReference;
			MakeReferenceToAsset( material, assetReference );
			selectAssetWidget->SetAssetReference( assetReference );
		}
	}

	// Set info about asset
	const FBulkData<FStaticMeshVertexType>&		verteces	= staticMesh->GetVerteces();
	const FBulkData<uint32>&					indeces		= staticMesh->GetIndeces();
	
	ui->label_numVertecesValue->setText( QString::number( verteces.Num() ) );
	ui->label_numTrianglesValue->setText( QString::number( indeces.Num() / 3 ) );
	ui->label_resourceSizeValue->setText( QString::fromStdWString( FString::Format( TEXT( "%.2f Kb" ), ( verteces.Num() * sizeof( FStaticMeshVertexType ) + indeces.Num() * sizeof( uint32 ) ) / 1024.f ) ) );
	OnSourceFileChanged( QString::fromStdWString( staticMesh->GetAssetSourceFile() ) );
}

WeStaticMeshEditorWindow::~WeStaticMeshEditorWindow()
{
	FlushRenderingCommands();
	ui->viewportPreview->SetViewportClient( nullptr, false );
	delete viewportClient;
	delete ui;

	// Unsubscribe from event when assets try destroy and reload
	FEditorDelegates::onAssetsCanDelete.Remove( assetsCanDeleteHandle );
	FEditorDelegates::onAssetsReloaded.Remove( assetsReloadedHandle );
}

void WeStaticMeshEditorWindow::OnSourceFileChanged( QString InNewSourceFile )
{
	label_sourceFileValue->setText( InNewSourceFile );
	label_sourceFileValue->setToolTip( InNewSourceFile );
	ui->actionReimport->setEnabled( !InNewSourceFile.isEmpty() );
	UICropSourceFileText();
}

void WeStaticMeshEditorWindow::UICropSourceFileText()
{
	QFontMetrics		fontMetrices = label_sourceFileValue->fontMetrics();
	uint32				maxTextWidth = Max( label_sourceFileValue->size().width() - 2, 150 );
	label_sourceFileValue->setText( fontMetrices.elidedText( QString::fromStdWString( staticMesh->GetAssetSourceFile() ), Qt::TextElideMode::ElideRight, maxTextWidth ) );
}

void WeStaticMeshEditorWindow::resizeEvent( QResizeEvent* InEvent )
{
	UICropSourceFileText();
}

void WeStaticMeshEditorWindow::OnSelectedAssetMaterial( uint32 InAssetSlot, const std::wstring& InNewAssetReference )
{
	if ( !staticMesh )
	{
		return;
	}

	// If asset reference is valid, we find asset
	TAssetHandle<FMaterial>		newMaterialRef;
	if ( !InNewAssetReference.empty() )
	{
		newMaterialRef		= GPackageManager->FindAsset( InNewAssetReference, AT_Material );
	}

	// If asset is not valid we clear asset reference
	if ( !newMaterialRef.IsAssetValid() || GPackageManager->IsDefaultAsset( newMaterialRef ) )
	{
		newMaterialRef = nullptr;
		selectAsset_materials[ InAssetSlot ]->ClearAssetReference( false );
	}

	staticMesh->SetMaterial( InAssetSlot, newMaterialRef );
}

void WeStaticMeshEditorWindow::OnAssetsCanDelete( const std::vector< TSharedPtr<class FAsset> >& InAssets, FCanDeleteAssetResult& OutResult )
{
	FAsset::FSetDependentAssets		dependentAssets;
	staticMesh->GetDependentAssets( dependentAssets );

	// If in InAssets exist static mesh who is editing now or him dependent assets - need is block
	for ( uint32 index = 0, count = InAssets.size(); index < count; ++index )
	{
		TSharedPtr<FAsset>		assetRef = InAssets[ index ];
		if ( assetRef == staticMesh )
		{
			OutResult.Set( false );
			return;
		}

		// Look in dependent assets
		for ( auto itDependentAsset = dependentAssets.begin(), itDependentAssetEnd = dependentAssets.end(); itDependentAsset != itDependentAssetEnd; ++itDependentAsset )
		{
			if ( *itDependentAsset == assetRef )
			{
				OutResult.Set( false );
				return;
			}
		}
	}
}

void WeStaticMeshEditorWindow::OnAssetsReloaded( const std::vector< TSharedPtr<class FAsset> >& InAssets )
{
	// If static mesh who is edition reloaded, we update UI
	for ( uint32 index = 0, count = InAssets.size(); index < count; ++index )
	{
		if ( InAssets[ index ] == staticMesh )
		{
			UpdateUI();
			return;
		}
	}
}