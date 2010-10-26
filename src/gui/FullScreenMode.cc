/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <QStringList>
#include <QtGlobal>

#include "FullScreenMode.h"
#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "qt-widgets/ViewportWindow.h"


namespace
{
	/**
	 * This anonymous namespace function is called once to init a static list
	 * of widgets in @a FullScreenMode::toggle_full_screen().
	 *
	 * Requiring accessors exposing parts of ReconstructionViewWidget etc.,
	 * only to call ->hide() on them is wasteful. This list refers to the widgets by
	 * their @a objectName property, which lets us grab them as long as they
	 * are children of the main window. It also opens the door for making this
	 * list user-configurable in the future, perhaps - or at the very least,
	 * easier to maintain when the inevitable UI redesigns occur.
	 */
	QStringList
	get_full_screen_widgets_to_hide()
	{
		QStringList names;
		// Possible options are: menubar, statusbar, dock_search_results, toolbar_canvas_tools,
		// TaskPanel, ZoomSlider, AwesomeBar_1, ViewBar.
		// Bear in mind that if you hide AwesomeBar_1, you're also hiding the GMenu, which
		// would cause some problems.
		names << "menubar";
		names << "dock_search_results";
		names << "toolbar_canvas_tools";
		names << "statusbar";
		names << "TaskPanel";
		names << "ZoomSlider";	// Had some trouble hiding this one; seems ok now.
		return names;		// QStringList is pimpl-idiom so this isn't a bad thing.
	}

	/**
	 * Same as above, but for actions attached to menus.
	 */
	QStringList
	get_actions_to_disable()
	{
		QStringList names;
		names << "action_Show_Bottom_Panel";
		return names;
	}
}


GPlatesGui::FullScreenMode::FullScreenMode(
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		QObject *parent_):
	QObject(parent_),
	d_viewport_window_ptr(&viewport_window_)
{
}


void
GPlatesGui::FullScreenMode::init()
{
	QObject::connect(&leave_full_screen_button(), SIGNAL(clicked()),
			this, SLOT(leave_full_screen()));
}


void
GPlatesGui::FullScreenMode::leave_full_screen()
{
	// A special case; we just want to get out of it by pressing Esc.
	full_screen_action().setChecked(false);
	toggle_full_screen(false);
}


void
GPlatesGui::FullScreenMode::toggle_full_screen(
		bool wants_full_screen)
{
	static const QStringList things_to_hide = get_full_screen_widgets_to_hide();
	static const QStringList actions_to_disable = get_actions_to_disable();

	// Tell Qt to do this step as one big change - looks nicer, probably means
	// less race-condition-like buggy behaviour due to hiding a bunch of widgets.
	viewport_window().setUpdatesEnabled(false);
	
	if (wants_full_screen) {
		// Store original state of toolbars, docks, etc.
		d_viewport_state = viewport_window().saveState();
		
		// Set the 'Full Screen' state bit.
		viewport_window().setWindowState(viewport_window().windowState() | Qt::WindowFullScreen);
		
		// Hide non-essential widgets.
		Q_FOREACH(QString widget_name, things_to_hide) {
			QWidget *widget = viewport_window().findChild<QWidget *>(widget_name);
			if (widget) {
				widget->clearFocus();
				widget->hide();
			}
		}

		// Disable certain actions on menus.
		Q_FOREACH(QString action_name, actions_to_disable) {
			QAction *action = viewport_window().findChild<QAction *>(action_name);
			if (action) {
				action->setEnabled(false);
			}
		}
		
		// Reduce the border around the RecontructionViewWidget so that
		// access to the GMenu passes Fitt's law.
		reconstruction_view_widget().layout()->setContentsMargins(0,0,0,0);
		reconstruction_view_widget().layout()->setSpacing(0);
	} else {
		// Clear the 'Full Screen' state bit.
		viewport_window().setWindowState(viewport_window().windowState() & ~Qt::WindowFullScreen);

		// Unhide hidden widgets.
		Q_FOREACH(QString widget_name, things_to_hide) {
			QWidget *widget = viewport_window().findChild<QWidget *>(widget_name);
			if (widget) {
				widget->show();
			}
		}

		// Re-enable disabled actions.
		Q_FOREACH(QString action_name, actions_to_disable) {
			QAction *action = viewport_window().findChild<QAction *>(action_name);
			if (action) {
				action->setEnabled(true);
			}
		}

		// Restore the border around the RecontructionViewWidget.
		reconstruction_view_widget().layout()->setContentsMargins(1,1,1,1);
		reconstruction_view_widget().layout()->setSpacing(0);

		// Restore original state of toolbars, docks, etc.
		viewport_window().restoreState(d_viewport_state);
	}

	// Hide the GMenu button if windowed; show it if full-screen.
	gmenu_button().setVisible(wants_full_screen);

	// The 'Leave Full Screen' button should be made available in full screen mode.
	leave_full_screen_button().setVisible(wants_full_screen);

	// After changing all those widgets, allow Qt to do the painting.
	viewport_window().setUpdatesEnabled(true);

	// In each case, we want to ensure the window is raised (visually on top)
	// and activated (taking keyboard focus).
	viewport_window().raise();
	viewport_window().activateWindow();
	// And try to move keyboard focus to the reconstruction view widget; widgets
	// that get hidden by the full-screening process might be ones that had focus.
	reconstruction_view_widget().setFocus();
}



QWidget &
GPlatesGui::FullScreenMode::gmenu_button()
{
	// Obtain a pointer to the widget once, via the ViewportWindow and Qt magic.
	static QWidget *gmenu_button_ptr = viewport_window().findChild<QWidget *>("GMenuButton");
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			gmenu_button_ptr != NULL,
			GPLATES_ASSERTION_SOURCE);
	return *gmenu_button_ptr;
}


QWidget &
GPlatesGui::FullScreenMode::leave_full_screen_button()
{
	// Obtain a pointer to the widget once, via the ViewportWindow and Qt magic.
	static QWidget *leave_button_ptr = viewport_window().findChild<QWidget *>("LeaveFullScreenButton");
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			leave_button_ptr != NULL,
			GPLATES_ASSERTION_SOURCE);
	return *leave_button_ptr;
}


QWidget &
GPlatesGui::FullScreenMode::reconstruction_view_widget()
{
	// Obtain a pointer to the widget once, via the ViewportWindow and Qt magic.
	static QWidget *reconstruction_view_widget_ptr = viewport_window().findChild<QWidget *>("ReconstructionViewWidget");
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			reconstruction_view_widget_ptr != NULL,
			GPLATES_ASSERTION_SOURCE);
	return *reconstruction_view_widget_ptr;
}


QAction &
GPlatesGui::FullScreenMode::full_screen_action()
{
	// Obtain a pointer to the action once, via the ViewportWindow and Qt magic.
	static QAction *action_Full_Screen_ptr = viewport_window().findChild<QAction *>("action_Full_Screen");
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			action_Full_Screen_ptr != NULL,
			GPLATES_ASSERTION_SOURCE);
	return *action_Full_Screen_ptr;
}

