/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QtGlobal>
#include <QApplication>
#include <QFontMetrics>
#include <QLocale>
#include <QDebug>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QMetaMethod>
#include <QDesktopServices>

#include "GuiDebug.h"

#include "model/FeatureCollectionHandle.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Serialization.h"
#include "app-logic/SessionManagement.h"
#include "app-logic/UserPreferences.h"
#include "app-logic/ReconstructGraph.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/TaskPanel.h"


#include <boost/foreach.hpp>
namespace
{

	/**
	 * Given a QObject, introspect it for slots that take no arguments (and optionally
	 * only ones that start with a given prefix), and add a menu entry for each slot to
	 * the supplied menu.
	 */
	void
	add_slots_to_menu(
			const QObject *object,
			QString prefix,
			QMenu *menu)
	{
		if ( ! object) {
			return;
		}
		const QMetaObject *introspect = object->metaObject();
		for (int i = introspect->methodOffset(); i < introspect->methodCount(); ++i) {
			QMetaMethod method = introspect->method(i);
			// Aha! A method of ours. Is it a slot which takes no arguments?
			if (method.methodType() == QMetaMethod::Slot && method.parameterTypes().isEmpty()) {
				QString label(method.signature());
				// does it match the given prefix?
				if (prefix.isNull() || prefix.isEmpty() || label.startsWith(prefix)) {
					// Below I use a little hack to emulate the SLOT() macro on a dynamic char*:
					QString slot("1");
					slot.append(method.signature());
					// Add to menu.
					menu->addAction(label, object, slot.toAscii().constData());
				}
			}
		}
	}

	/**
	 * Convenience version of @a add_slots_to_menu that only adds slots with the
	 * prefix 'debug_'.
	 */
	void
	add_debug_slots_to_menu(
			const QObject *object,
			QMenu *menu)
	{
		if ( ! object) {
			return;
		}
		add_slots_to_menu(object, "debug_", menu);
	}
	
	/**
	 * Convenience version of @a add_slots_to_menu that adds menu items under a
	 * submenu with the class name of the object.
	 *
	 * Remember, they have to be defined as slots so we can add a QAction for them.
	 */
	void
	add_slots_as_submenu(
			const QObject *object,
			QString prefix,
			QMenu *menu)
	{
		if ( ! object) {
			return;
		}
		QMenu *submenu = menu->addMenu(object->metaObject()->className());
		// Tearable menus are delicious.
		submenu->setTearOffEnabled(true);
		add_slots_to_menu(object, prefix, submenu);
	}
	
	
	/**
	 * Recursively print out our menu structure.
	 */
	void
	print_menu_structure(
			QWidget *menu,
			QString prefix = "* ",
			QString indentation = "")
	{
		Q_FOREACH(QAction *action, menu->actions()) {
			QString shortcut = "";
			if ( ! action->shortcut().isEmpty()) {
				shortcut = QString(" [ %1 ]").arg(action->shortcut().toString());
			}
			QString flags = "";
			if ( ! action->isVisible()) {
				flags += " (Hidden)";
			}
			if ( ! action->isEnabled()) {
				flags += " (Disabled)";
			}
			qDebug() << QString("%1%2%3%4%5")
					.arg(indentation)
					.arg(prefix)
					.arg(action->text())
					.arg(shortcut)
					.arg(flags)
					.toUtf8().data();
			// Recurse into submenus - but not into our own Debug menu, that'd be a bit much.
			if (action->menu() && ! action->text().endsWith("Debug")) {
				print_menu_structure(action->menu(), prefix, QString(indentation).append("  "));
			}
		}
	}
}



GPlatesGui::GuiDebug::GuiDebug(
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		GPlatesAppLogic::ApplicationState &app_state_,
		QObject *parent_):
	QObject(parent_),
	d_viewport_window_ptr(&viewport_window_),
	d_app_state_ptr(&app_state_)
{
	create_menu();
}


void
GPlatesGui::GuiDebug::create_menu()
{
	// Create and add the main Debug menu.
	QMenu *debug_menu = new QMenu(tr("&Debug"), d_viewport_window_ptr);
	d_viewport_window_ptr->menuBar()->addMenu(debug_menu);
	// Tearable menus should really be the standard everywhere ever.
	debug_menu->setTearOffEnabled(true);
	
	// Add and connect actions to the menu.
	QAction *action_Gui_Debug_Action = new QAction(QIcon(":/info_sign_16.png"), tr("GUI Debug &Action"), this);
	action_Gui_Debug_Action->setShortcutContext(Qt::ApplicationShortcut);
	action_Gui_Debug_Action->setShortcut(tr("Ctrl+Alt+/"));
	debug_menu->addAction(action_Gui_Debug_Action);
	QObject::connect(action_Gui_Debug_Action, SIGNAL(triggered()),
			this, SLOT(handle_gui_debug_action()));
	
	debug_menu->addSeparator();
	
	// Automagically add any slot of ours beginning with 'debug_'.
	// If you don't need a keyboard shortcut for it, this is a fantastic way to
	// quickly add some test code you can trigger at-will at runtime.
	add_debug_slots_to_menu(this, debug_menu);

	// Plus a few 'debug_' methods from specific classes as a submenu:-
	add_slots_as_submenu(&(d_app_state_ptr->get_user_preferences()), "debug_", debug_menu);
	add_slots_as_submenu(&(d_app_state_ptr->get_session_management()), "", debug_menu);
	add_slots_as_submenu(&(d_app_state_ptr->get_serialization()), "debug_", debug_menu);
	add_slots_as_submenu(&(d_app_state_ptr->get_reconstruct_graph()), "debug_", debug_menu);

	// For bonus points, let's add ALL no-argument slots from ViewportWindow and friends.
	add_slots_as_submenu(d_viewport_window_ptr, "", debug_menu);
	add_slots_as_submenu(d_viewport_window_ptr->task_panel_ptr(), "", debug_menu);
	add_slots_as_submenu(find_child_qobject("ManageFeatureCollectionsDialog"), "", debug_menu);
}


QObject *
GPlatesGui::GuiDebug::find_child_qobject(
		QString name)
{
	QObject *found = d_viewport_window_ptr->findChild<QObject *>(name);
	if ( ! found) {
		qDebug() << "GuiDebug::find_child_qobject("<<name<<"): Couldn't find this one. Is it parented"
				<< " (directly or indirectly) to ViewportWindow, and does it have a proper objectName set?";
	}
	return found;
}


void
GPlatesGui::GuiDebug::handle_gui_debug_action()
{
	// Some handy information that may aid debugging:

	// "Where the hell did my keyboard focus go?"
	qDebug() << "Current focus:" << QApplication::focusWidget();

	// "What's the name of the current style so I can test against it?"
	qDebug() << "Current style:" << d_viewport_window_ptr->style()->objectName();


	// "What's this thing doing there?"
	QWidget *cursor_widget = QApplication::widgetAt(QCursor::pos());
	qDebug() << "Current widget under cursor:" << cursor_widget;
	while (cursor_widget && cursor_widget->parentWidget()) {
		cursor_widget = cursor_widget->parentWidget();
		qDebug() << "\twhich is inside:" << cursor_widget;
	}
}


void
GPlatesGui::GuiDebug::debug_set_all_files_clean()
{
	qDebug() << "GPlatesGui::GuiDebug::debug_set_all_files_clean()";

	// Grab the FeatureCollectionFileState and just go through all loaded files' feature collections.
	GPlatesAppLogic::FeatureCollectionFileState &fcfs = 
			d_app_state_ptr->get_feature_collection_file_state();

	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			fcfs.get_loaded_files();
	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &loaded_file,
			loaded_files)
	{
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
				loaded_file.get_file().get_feature_collection();

		if (feature_collection_ref.is_valid()) {
			feature_collection_ref->clear_unsaved_changes();
		}
	}
}


void
GPlatesGui::GuiDebug::debug_menu_structure()
{
	print_menu_structure(d_viewport_window_ptr->menuBar());
}


void
GPlatesGui::GuiDebug::debug_font_metrics()
{
	QFontMetrics fm = QApplication::fontMetrics();

	qDebug() << "\nFONT METRICS DEBUGGING:";
	qDebug() << "QApplication::style() == " << QApplication::style()->metaObject()->className();
	qDebug() << "QApplication::font().toString() == " << QApplication::font().toString();
	qDebug() << "QLocale().name() == " << QLocale().name();
	qDebug() << "fm.ascent() == " << fm.ascent();
	qDebug() << "fm.descent() == " << fm.descent();
	qDebug() << "fm.boundingRect(Q) == " << fm.boundingRect('Q');
	qDebug() << "fm.boundingRect(y) == " << fm.boundingRect('y');
	qDebug() << "fm.boundingRect(QylLj!|[]`~_) == " << fm.boundingRect("QylLj!|[]`~_");
	qDebug() << "fm.height() == " << fm.height();
	qDebug() << "fm.lineSpacing() == " << fm.lineSpacing();
	qDebug() << "fm.leading() == " << fm.leading();
}


void
GPlatesGui::GuiDebug::debug_system_paths()
{
	qDebug() << "\nSYSTEM PATHS:";
	qDebug() << "QDesktopServices::DesktopLocation ==" << QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
	qDebug() << "QDesktopServices::DocumentsLocation ==" << QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
	qDebug() << "QDesktopServices::FontsLocation ==" << QDesktopServices::storageLocation(QDesktopServices::FontsLocation);
	qDebug() << "QDesktopServices::ApplicationsLocation ==" << QDesktopServices::storageLocation(QDesktopServices::ApplicationsLocation);
	qDebug() << "QDesktopServices::MusicLocation ==" << QDesktopServices::storageLocation(QDesktopServices::MusicLocation);
	qDebug() << "QDesktopServices::MoviesLocation ==" << QDesktopServices::storageLocation(QDesktopServices::MoviesLocation);
	qDebug() << "QDesktopServices::PicturesLocation ==" << QDesktopServices::storageLocation(QDesktopServices::PicturesLocation);
	qDebug() << "QDesktopServices::TempLocation ==" << QDesktopServices::storageLocation(QDesktopServices::TempLocation);
	qDebug() << "QDesktopServices::HomeLocation ==" << QDesktopServices::storageLocation(QDesktopServices::HomeLocation);
	qDebug() << "QDesktopServices::DataLocation ==" << QDesktopServices::storageLocation(QDesktopServices::DataLocation);
#if QT_VERSION >= 0x040500
	qDebug() << "QDesktopServices::CacheLocation (4.5) ==" << QDesktopServices::storageLocation(QDesktopServices::CacheLocation);
#endif
}

