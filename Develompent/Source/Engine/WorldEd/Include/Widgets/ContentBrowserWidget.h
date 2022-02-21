/**
 * @file
 * @addtogroup WorldEd World editor
 *
 * Copyright Broken Singularity, All Rights Reserved.
 * Authors: Yehor Pohuliaka (zombiHello)
 */

#ifndef CONTENTBROWSERWIDGET_H
#define CONTENTBROWSERWIDGET_H

#include <QWidget>
#include "Core.h"

namespace Ui
{
	class WeContentBrowserWidget;
}

/**
 * @ingroup WorldEd
 * Widget of showing contents in file system
 */
class WeContentBrowserWidget : public QWidget
{
	Q_OBJECT

public:
	/**
	 * Constructor
	 */
	WeContentBrowserWidget( const tchar* InRootDir, QWidget* InParent = nullptr );

	/**
	 * Destructor
	 */
	virtual ~WeContentBrowserWidget();

private slots:
	/**
	 * File explorer context menu open event
	 * 
	 * @param InPoint Point of click in content browser
	 */
	void on_treeView_contentBrowser_customContextMenuRequested( QPoint InPoint );

	/**
	 * Package explorer context menu selection event 'Import asset'
	 */
	void on_listView_packageBrowser_ImportAsset();

	/**
	 * Package explorer context menu selection event 'Create material'
	 */
	void on_listView_packageBrowser_CreateMaterial();

	/**
	 * Package explorer context menu selection event 'Delete asset'
	 */
	void on_listView_packageBrowser_DeleteAsset();

	/**
	 * Package explorer context menu selection event 'Rename asset'
	 */
	void on_listView_packageBrowser_RenameAsset();

	/**
	 * Package explorer context menu open event
	 */
	void on_listView_packageBrowser_customContextMenuRequested( QPoint InPoint );

	/**
	 * File Explorer context menu selection event "Delete"
	 */
	void on_treeView_contentBrowser_contextMenu_delete();

	/**
	 * File Explorer context menu selection event "Rename"
	 */
	void on_treeView_contentBrowser_contextMenu_rename();

	/**
	 * File Explorer context menu selection event "Create Folder"
	 */
	void on_treeView_contentBrowser_contextMenu_createFolder();

	/**
	 * File explorer context menu selection event "Create package"
	 */
	void on_treeView_contentBrowser_contextMenu_createPackage();

	/**
	 * File explorer context menu selection event "Save package"
	 */
	void on_treeView_contentBrowser_contextMenu_SavePackage();

	/**
	 * File explorer context menu selection event "Open package"
	 */
	void on_treeView_contentBrowser_contextMenu_OpenPackage();

	/**
	 * File explorer context menu selection event "Unload of package"
	 */
	void on_treeView_contentBrowser_contextMenu_UnloadPackage();

	/**
	 * Event open package
	 * @param InPackage Package
	 */
	void on_treeView_contentBrowser_OnOpenPackage( class FPackage* InPackage );

private:
	Ui::WeContentBrowserWidget*				ui;			/**< Qt UI */

};

#endif // !CONTENTBROWSERWIDGET_H