/*
 * Copyright (C) 2026 The GPlates developers
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 */

#include <QAction>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QTabWidget>
#include <QTextBrowser>
#include <QToolBar>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

#include "ProjectDocumentsDockWidget.h"

#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/PlanetaryParameters.h"
#include "app-logic/ProjectDocumentRegistry.h"

#include "presentation/SessionManagement.h"
#include "presentation/ViewState.h"


namespace
{
	const char *DEFAULT_PROJECT_DOCUMENT =
			"---\n"
			"gplates:\n"
			"  schema_version: 1\n"
			"  planet:\n"
			"    radius_m: 6378137\n"
			"---\n\n"
			"# Project\n\n"
			"## Purpose\n\n"
			"## Reconstruction assumptions\n\n"
			"## Data sources\n\n"
			"## Decisions\n\n"
			"## Known gaps\n";
}


GPlatesQtWidgets::ProjectDocumentsDockWidget::ProjectDocumentsDockWidget(
		GPlatesGui::DockState &dock_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow &main_window) :
	DockWidget(tr("Project Documents"), dock_state, main_window, QString("project_documents")),
	d_view_state(&view_state),
	d_document_registry(&view_state.get_application_state().get_project_document_registry()),
	d_planetary_parameters(&view_state.get_application_state().get_planetary_parameters()),
	d_updating_editor(false)
{
	QWidget *contents = new QWidget(this);
	QVBoxLayout *layout = new QVBoxLayout(contents);
	layout->setContentsMargins(4, 4, 4, 4);

	QLabel *introduction = new QLabel(
			tr("Associate ordinary Markdown files with this project. Create or Add one, then Set"
				" Primary on the one whose front matter GPlates should read (planet radius,"
				" resolution, required timestamps, time step, subduction rates) - Save it"
				" whenever you edit that front matter."),
			contents);
	introduction->setWordWrap(true);
	layout->addWidget(introduction);

	QToolBar *toolbar = new QToolBar(contents);
	toolbar->setIconSize(QSize(16, 16));
	d_create_action = toolbar->addAction(tr("Create"), this, SLOT(create_document()));
	d_create_action->setToolTip(tr("Create Markdown Document"));
	d_add_action = toolbar->addAction(tr("Add"), this, SLOT(add_existing_document()));
	d_add_action->setToolTip(tr("Add Existing Markdown Document"));
	d_remove_action = toolbar->addAction(tr("Remove"), this, SLOT(remove_document()));
	d_remove_action->setToolTip(tr("Remove from Project (does not delete the file)"));
	toolbar->addSeparator();
	d_set_primary_action = toolbar->addAction(tr("Set Primary"), this, SLOT(set_primary_document()));
	d_set_primary_action->setToolTip(tr(
			"Make this the Primary Project Document - the one whose front matter GPlates reads"
			" for planet radius, digitising resolution, the required timestamp schedule, the"
			" intended time step, and the subduction rates."
			" A project can have other associated documents, but only one primary."));
	d_save_action = toolbar->addAction(tr("Save"), this, SLOT(save_document()));
	d_save_action->setShortcut(QKeySequence::Save);
	d_save_action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	d_save_as_action = toolbar->addAction(tr("Save As"), this, SLOT(save_document_as()));
	d_reload_action = toolbar->addAction(tr("Reload"), this, SLOT(reload_document()));
	toolbar->addSeparator();
	d_locate_action = toolbar->addAction(tr("Locate"), this, SLOT(locate_missing_document()));
	d_external_editor_action = toolbar->addAction(tr("External Editor"), this, SLOT(open_in_external_editor()));
	d_file_browser_action = toolbar->addAction(tr("Show File"), this, SLOT(show_in_file_browser()));
	layout->addWidget(toolbar);

	QSplitter *splitter = new QSplitter(Qt::Vertical, contents);
	d_document_list = new QTreeWidget(splitter);
	d_document_list->setColumnCount(2);
	d_document_list->setHeaderLabels(QStringList() << tr("Document") << tr("Status"));
	d_document_list->setRootIsDecorated(false);
	d_document_list->setAlternatingRowColors(true);
	d_document_list->setContextMenuPolicy(Qt::ActionsContextMenu);
	d_document_list->addActions(QList<QAction *>()
			<< d_create_action << d_add_action << d_remove_action << d_set_primary_action
			<< d_save_action << d_save_as_action << d_reload_action << d_locate_action
			<< d_external_editor_action << d_file_browser_action);

	d_editor_tabs = new QTabWidget(splitter);
	d_editor = new QPlainTextEdit(d_editor_tabs);
	d_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
	d_preview = new QTextBrowser(d_editor_tabs);
	d_preview->setOpenExternalLinks(true);
	d_editor_tabs->addTab(d_editor, tr("Edit"));
	d_editor_tabs->addTab(d_preview, tr("Preview"));
	splitter->addWidget(d_document_list);
	splitter->addWidget(d_editor_tabs);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 3);
	layout->addWidget(splitter, 1);

	d_metadata_status = new QLabel(contents);
	d_metadata_status->setWordWrap(true);
	d_metadata_status->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(d_metadata_status);

	d_file_watcher = new QFileSystemWatcher(this);
	setWidget(contents);

	QObject::connect(d_document_list, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
			this, SLOT(handle_current_document_changed(QTreeWidgetItem *, QTreeWidgetItem *)));
	QObject::connect(d_editor, SIGNAL(textChanged()), this, SLOT(handle_editor_text_changed()));
	QObject::connect(d_editor_tabs, SIGNAL(currentChanged(int)), this, SLOT(handle_tab_changed(int)));
	QObject::connect(d_document_registry, SIGNAL(documents_changed()), this, SLOT(refresh_document_list()));
	QObject::connect(d_document_registry, SIGNAL(document_content_changed(int)), this, SLOT(update_current_document_status(int)));
	QObject::connect(d_document_registry, SIGNAL(dirty_state_changed(bool)), this, SLOT(refresh_document_list()));
	QObject::connect(d_planetary_parameters, SIGNAL(metadata_changed()), this, SLOT(update_metadata_status()));
	QObject::connect(d_file_watcher, SIGNAL(fileChanged(const QString &)), this, SLOT(handle_file_changed(const QString &)));

	refresh_document_list();
	update_metadata_status();
}


int
GPlatesQtWidgets::ProjectDocumentsDockWidget::current_document_index() const
{
	return d_document_list->indexOfTopLevelItem(d_document_list->currentItem());
}


QString
GPlatesQtWidgets::ProjectDocumentsDockWidget::suggested_project_directory() const
{
	const boost::optional<GPlatesPresentation::SessionManagement::ProjectInfo> project =
			d_view_state->get_session_management().is_current_session_a_project();
	if (project)
	{
		return QFileInfo(project->get_project_filename()).absolutePath();
	}
	return QDir::currentPath();
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::create_document()
{
	const QString file_path = QFileDialog::getSaveFileName(
			this,
			tr("Create Markdown Document"),
			QDir(suggested_project_directory()).filePath("project.md"),
			tr("Markdown documents (*.md)"));
	if (file_path.isEmpty())
	{
		return;
	}

	const int index = d_document_registry->add_document(file_path);
	d_document_registry->set_document_text(index, QString::fromUtf8(DEFAULT_PROJECT_DOCUMENT));
	if (!save_document_at(index))
	{
		d_document_registry->remove_document(index);
		return;
	}
	refresh_document_list();
	if (index < d_document_list->topLevelItemCount())
	{
		d_document_list->setCurrentItem(d_document_list->topLevelItem(index));
	}
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::add_existing_document()
{
	const QStringList file_paths = QFileDialog::getOpenFileNames(
			this,
			tr("Add Existing Markdown Document"),
			suggested_project_directory(),
			tr("Markdown documents (*.md)"));
	for (int path_index = 0; path_index < file_paths.size(); ++path_index)
	{
		d_document_registry->add_document(file_paths[path_index]);
	}
	refresh_document_list();
}


bool
GPlatesQtWidgets::ProjectDocumentsDockWidget::confirm_discard_or_save(
		int index,
		const QString &action_description)
{
	if (index < 0 || !d_document_registry->document_at(index).is_dirty)
	{
		return true;
	}

	const QMessageBox::StandardButton button = QMessageBox::warning(
			this,
			tr("Unsaved Project Document"),
			tr("%1 has unsaved changes. Save them before %2?")
					.arg(d_document_registry->document_at(index).display_name, action_description),
			QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
			QMessageBox::Save);
	if (button == QMessageBox::Cancel)
	{
		return false;
	}
	if (button == QMessageBox::Save)
	{
		return save_document_at(index);
	}
	return true;
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::remove_document()
{
	const int index = current_document_index();
	if (index < 0 || !confirm_discard_or_save(index, tr("removing it from the project")))
	{
		return;
	}

	if (QMessageBox::question(
			this,
			tr("Remove Project Document"),
			tr("Remove '%1' from the project? The file will remain on disk.")
					.arg(d_document_registry->document_at(index).display_name),
			QMessageBox::Yes | QMessageBox::No,
			QMessageBox::No) == QMessageBox::Yes)
	{
		d_document_registry->remove_document(index);
	}
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::set_primary_document()
{
	const int index = current_document_index();
	if (index >= 0)
	{
		d_document_registry->set_primary_document(index);
	}
}


bool
GPlatesQtWidgets::ProjectDocumentsDockWidget::save_document_at(
		int index)
{
	if (index < 0)
	{
		return false;
	}
	d_ignore_next_file_change.insert(d_document_registry->document_at(index).file_path);
	QString error_message;
	if (!d_document_registry->save_document(index, &error_message))
	{
		d_ignore_next_file_change.remove(d_document_registry->document_at(index).file_path);
		QMessageBox::critical(this, tr("Unable to Save Project Document"), error_message);
		return false;
	}
	refresh_file_watcher();
	return true;
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::save_document()
{
	save_document_at(current_document_index());
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::save_document_as()
{
	const int index = current_document_index();
	if (index < 0)
	{
		return;
	}
	const QString file_path = QFileDialog::getSaveFileName(
			this,
			tr("Save Markdown Document As"),
			d_document_registry->document_at(index).file_path,
			tr("Markdown documents (*.md)"));
	if (file_path.isEmpty())
	{
		return;
	}

	const QString normalised_file_path = QDir::cleanPath(QFileInfo(file_path).absoluteFilePath());
	d_ignore_next_file_change.insert(normalised_file_path);
	QString error_message;
	if (!d_document_registry->save_document_as(index, file_path, &error_message))
	{
		d_ignore_next_file_change.remove(normalised_file_path);
		QMessageBox::critical(this, tr("Unable to Save Project Document"), error_message);
		return;
	}
	refresh_file_watcher();
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::reload_document()
{
	const int index = current_document_index();
	if (index < 0 || !confirm_discard_or_save(index, tr("reloading it from disk")))
	{
		return;
	}

	QString error_message;
	if (!d_document_registry->reload_document(index, &error_message))
	{
		QMessageBox::critical(this, tr("Unable to Reload Project Document"), error_message);
		return;
	}
	load_current_document();
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::locate_missing_document()
{
	const int index = current_document_index();
	if (index < 0)
	{
		return;
	}
	const QString file_path = QFileDialog::getOpenFileName(
			this,
			tr("Locate Missing Markdown Document"),
			QFileInfo(d_document_registry->document_at(index).file_path).absolutePath(),
			tr("Markdown documents (*.md)"));
	if (!file_path.isEmpty())
	{
		d_document_registry->relocate_document(index, file_path);
		load_current_document();
	}
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::open_in_external_editor()
{
	const int index = current_document_index();
	if (index >= 0)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(d_document_registry->document_at(index).file_path));
	}
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::show_in_file_browser()
{
	const int index = current_document_index();
	if (index >= 0)
	{
		QDesktopServices::openUrl(QUrl::fromLocalFile(
				QFileInfo(d_document_registry->document_at(index).file_path).absolutePath()));
	}
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::handle_current_document_changed(
		QTreeWidgetItem *,
		QTreeWidgetItem *)
{
	load_current_document();
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::load_current_document()
{
	const int index = current_document_index();
	d_updating_editor = true;
	if (index < 0)
	{
		d_editor->clear();
		d_editor->setEnabled(false);
	}
	else
	{
		QString error_message;
		if (!d_document_registry->load_document(index, &error_message))
		{
			d_editor->clear();
			d_editor->setPlaceholderText(tr("The document is missing or unreadable. Use Locate or Reload.\n%1").arg(error_message));
		}
		else
		{
			d_editor->setPlaceholderText(QString());
			d_editor->setPlainText(d_document_registry->document_at(index).text);
		}
		d_editor->setEnabled(true);
	}
	d_updating_editor = false;
	update_preview();
	update_actions();
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::handle_editor_text_changed()
{
	if (d_updating_editor)
	{
		return;
	}
	const int index = current_document_index();
	if (index >= 0)
	{
		d_document_registry->set_document_text(index, d_editor->toPlainText());
		if (d_editor_tabs->currentIndex() == 1)
		{
			update_preview();
		}
	}
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::handle_tab_changed(
		int index)
{
	if (index == 1)
	{
		update_preview();
	}
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::update_preview()
{
	const int index = current_document_index();
	if (index < 0)
	{
		d_preview->clear();
		return;
	}

	// QTextDocument Markdown support is available in the minimum supported Qt 5.15.
	d_preview->setMarkdown(d_editor->toPlainText());
	d_preview->document()->setBaseUrl(QUrl::fromLocalFile(
			QFileInfo(d_document_registry->document_at(index).file_path).absolutePath() + QDir::separator()));
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::refresh_document_list()
{
	const int selected_index = current_document_index();
	d_document_list->blockSignals(true);
	d_document_list->clear();

	const boost::optional<int> primary_index = d_document_registry->primary_document_index();
	for (int index = 0; index < d_document_registry->document_count(); ++index)
	{
		const GPlatesAppLogic::ProjectDocumentRegistry::Document &document = d_document_registry->document_at(index);
		QStringList status;
		if (primary_index && primary_index.get() == index)
		{
			status << tr("Primary");
		}
		if (document.is_dirty)
		{
			status << tr("Modified");
		}
		if (!QFileInfo::exists(document.file_path))
		{
			status << tr("Missing");
		}
		if (document.is_externally_modified)
		{
			status << tr("Changed on disk");
		}
		if (primary_index && primary_index.get() == index &&
			d_planetary_parameters->radius_source() ==
					GPlatesAppLogic::PlanetaryParameters::INVALID_PROJECT_METADATA_USING_EARTH_DEFAULT)
		{
			status << tr("Metadata warning");
		}

		QTreeWidgetItem *item = new QTreeWidgetItem(
				QStringList() << document.display_name << status.join(", "));
		item->setToolTip(0, document.file_path);
		item->setToolTip(1, status.join(", "));
		d_document_list->addTopLevelItem(item);
	}

	if (selected_index >= 0 && selected_index < d_document_list->topLevelItemCount())
	{
		d_document_list->setCurrentItem(d_document_list->topLevelItem(selected_index));
	}
	else if (d_document_list->topLevelItemCount() > 0)
	{
		d_document_list->setCurrentItem(d_document_list->topLevelItem(0));
	}
	d_document_list->resizeColumnToContents(0);
	d_document_list->blockSignals(false);
	refresh_file_watcher();
	load_current_document();
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::update_current_document_status(
		int index)
{
	if (index < 0 || index >= d_document_list->topLevelItemCount())
	{
		return;
	}
	const GPlatesAppLogic::ProjectDocumentRegistry::Document &document = d_document_registry->document_at(index);
	QStringList status;
	if (d_document_registry->primary_document_index() && d_document_registry->primary_document_index().get() == index)
	{
		status << tr("Primary");
	}
	if (document.is_dirty)
	{
		status << tr("Modified");
	}
	if (!QFileInfo::exists(document.file_path))
	{
		status << tr("Missing");
	}
	if (document.is_externally_modified)
	{
		status << tr("Changed on disk");
	}
	if (d_document_registry->primary_document_index() &&
		d_document_registry->primary_document_index().get() == index &&
		d_planetary_parameters->radius_source() ==
				GPlatesAppLogic::PlanetaryParameters::INVALID_PROJECT_METADATA_USING_EARTH_DEFAULT)
	{
		status << tr("Metadata warning");
	}
	d_document_list->topLevelItem(index)->setText(1, status.join(", "));
	update_actions();
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::update_metadata_status()
{
	switch (d_planetary_parameters->radius_source())
	{
	case GPlatesAppLogic::PlanetaryParameters::PROJECT_MARKDOWN:
		d_metadata_status->setText(
				tr("Planet radius: %1 m (Primary Project Document)")
						.arg(d_planetary_parameters->effective_radius_metres(), 0, 'g', 15));
		break;

	case GPlatesAppLogic::PlanetaryParameters::INVALID_PROJECT_METADATA_USING_EARTH_DEFAULT:
		d_metadata_status->setText(d_planetary_parameters->radius_diagnostic());
		d_metadata_status->setStyleSheet("QLabel { color: #a05000; }");
		return;

	case GPlatesAppLogic::PlanetaryParameters::EARTH_DEFAULT:
	default:
		d_metadata_status->setText(
				tr("Planet radius: %1 m (Earth default)")
						.arg(d_planetary_parameters->effective_radius_metres(), 0, 'g', 15));
		break;
	}
	d_metadata_status->setStyleSheet(QString());
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::update_actions()
{
	const int index = current_document_index();
	const bool has_document = index >= 0;
	const bool file_exists = has_document && QFileInfo::exists(d_document_registry->document_at(index).file_path);
	d_remove_action->setEnabled(has_document);
	d_set_primary_action->setEnabled(has_document &&
			(!d_document_registry->primary_document_index() || d_document_registry->primary_document_index().get() != index));
	d_save_action->setEnabled(has_document && d_document_registry->document_at(index).is_dirty);
	d_save_as_action->setEnabled(has_document);
	d_reload_action->setEnabled(has_document && file_exists);
	d_locate_action->setEnabled(has_document && !file_exists);
	d_external_editor_action->setEnabled(has_document && file_exists);
	d_file_browser_action->setEnabled(has_document && file_exists);
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::refresh_file_watcher()
{
	if (!d_file_watcher->files().isEmpty())
	{
		d_file_watcher->removePaths(d_file_watcher->files());
	}
	QStringList paths;
	for (int index = 0; index < d_document_registry->document_count(); ++index)
	{
		const QString path = d_document_registry->document_at(index).file_path;
		if (QFileInfo::exists(path))
		{
			paths.append(path);
		}
	}
	if (!paths.isEmpty())
	{
		d_file_watcher->addPaths(paths);
	}
}


void
GPlatesQtWidgets::ProjectDocumentsDockWidget::handle_file_changed(
		const QString &file_path)
{
	if (d_ignore_next_file_change.remove(file_path))
	{
		refresh_file_watcher();
		return;
	}

	for (int index = 0; index < d_document_registry->document_count(); ++index)
	{
		if (d_document_registry->document_at(index).file_path == file_path)
		{
			if (d_document_registry->document_at(index).is_dirty)
			{
				// Do not overwrite in-editor changes. The user can explicitly
				// choose Save or Reload after seeing the conflict indicator.
				d_document_registry->set_document_externally_modified(index, true);
			}
			else
			{
				// A clean document can be refreshed safely. Reloading also
				// reparses metadata when this is the primary document.
				QString error_message;
				if (!d_document_registry->reload_document(index, &error_message))
				{
					d_document_registry->set_document_externally_modified(index, true);
				}
				else if (current_document_index() == index)
				{
					load_current_document();
				}
			}
			break;
		}
	}
	refresh_file_watcher();
}
