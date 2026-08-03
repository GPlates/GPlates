/*
 * Copyright (C) 2026 The GPlates developers
 *
 * This file is part of GPlates.
 */

#ifndef GPLATES_QTWIDGETS_PROJECTDOCUMENTSDOCKWIDGET_H
#define GPLATES_QTWIDGETS_PROJECTDOCUMENTSDOCKWIDGET_H

#include <QPointer>
#include <QSet>

#include "DockWidget.h"


class QAction;
class QFileSystemWatcher;
class QLabel;
class QPlainTextEdit;
class QTabWidget;
class QTextBrowser;
class QTreeWidget;
class QTreeWidgetItem;

namespace GPlatesAppLogic
{
	class PlanetaryParameters;
	class ProjectDocumentRegistry;
}

namespace GPlatesGui
{
	class DockState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;

	/**
	 * Dockable plain-text Markdown editor and rendered project-document preview.
	 */
	class ProjectDocumentsDockWidget :
			public DockWidget
	{
		Q_OBJECT

	public:
		ProjectDocumentsDockWidget(
				GPlatesGui::DockState &dock_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow &main_window);

	private Q_SLOTS:
		void create_document();
		void add_existing_document();
		void remove_document();
		void set_primary_document();
		void save_document();
		void save_document_as();
		void reload_document();
		void locate_missing_document();
		void open_in_external_editor();
		void show_in_file_browser();
		void handle_current_document_changed(QTreeWidgetItem *current, QTreeWidgetItem *previous);
		void handle_editor_text_changed();
		void handle_tab_changed(int index);
		void refresh_document_list();
		void update_current_document_status(int index);
		void update_metadata_status();
		void handle_file_changed(const QString &file_path);

	private:
		int current_document_index() const;
		QString suggested_project_directory() const;
		bool save_document_at(int index);
		bool confirm_discard_or_save(int index, const QString &action_description);
		void load_current_document();
		void update_preview();
		void update_actions();
		void refresh_file_watcher();

		GPlatesPresentation::ViewState *d_view_state;
		GPlatesAppLogic::ProjectDocumentRegistry *d_document_registry;
		GPlatesAppLogic::PlanetaryParameters *d_planetary_parameters;

		QPointer<QTreeWidget> d_document_list;
		QPointer<QPlainTextEdit> d_editor;
		QPointer<QTextBrowser> d_preview;
		QPointer<QTabWidget> d_editor_tabs;
		QPointer<QLabel> d_metadata_status;
		QPointer<QFileSystemWatcher> d_file_watcher;

		QPointer<QAction> d_create_action;
		QPointer<QAction> d_add_action;
		QPointer<QAction> d_remove_action;
		QPointer<QAction> d_set_primary_action;
		QPointer<QAction> d_save_action;
		QPointer<QAction> d_save_as_action;
		QPointer<QAction> d_reload_action;
		QPointer<QAction> d_locate_action;
		QPointer<QAction> d_external_editor_action;
		QPointer<QAction> d_file_browser_action;

		QSet<QString> d_ignore_next_file_change;
		bool d_updating_editor;
	};
}

#endif // GPLATES_QTWIDGETS_PROJECTDOCUMENTSDOCKWIDGET_H
