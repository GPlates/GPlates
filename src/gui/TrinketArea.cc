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

#include <boost/bind.hpp>
#include <QLabel>
#include <QIcon>

#include "TrinketArea.h"

#include "Dialogs.h"

#include "qt-widgets/TrinketIcon.h"
#include "qt-widgets/ViewportWindow.h"


namespace
{
	GPlatesQtWidgets::TrinketIcon *
	create_unsaved_changes_trinket(
			GPlatesGui::Dialogs &dialogs)
	{
		GPlatesQtWidgets::TrinketIcon *unsaved = new GPlatesQtWidgets::TrinketIcon(
				QIcon(":/unsaved_changes_red_disk_bang_22.png"),
				QObject::tr("Save vs Fortitude."));
		unsaved->setVisible(false);
		unsaved->set_clickable(true);
		
		// Set a callback function object using boost::bind to wrap up the member function
		// call with our instance of Dialogs.
		unsaved->set_clicked_callback_function(boost::bind(
				&GPlatesGui::Dialogs::pop_up_manage_feature_collections_dialog,
				&dialogs
				));
		return unsaved;
	}

	
	GPlatesQtWidgets::TrinketIcon *
	create_read_errors_trinket(
			GPlatesGui::Dialogs &dialogs)
	{
		GPlatesQtWidgets::TrinketIcon *errors = new GPlatesQtWidgets::TrinketIcon(
				QIcon(":/gnome_dialog_warning_22.png"),
				QObject::tr("Some files had problems when they were loaded. Click for more information."));
		errors->setVisible(false);
		errors->set_clickable(true);

		// Set a callback function object using boost::bind to wrap up the member function
		// call with our instance of Dialogs.
		errors->set_clicked_callback_function(boost::bind(
				&GPlatesGui::Dialogs::pop_up_read_error_accumulation_dialog,
				&dialogs
				));
		return errors;
	}
}



GPlatesGui::TrinketArea::TrinketArea(
		Dialogs &dialogs,
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		QObject *parent_):
	QObject(parent_),
	d_viewport_window_ptr(&viewport_window_),
	d_trinket_unsaved(create_unsaved_changes_trinket(dialogs)),
	d_trinket_read_errors(create_read_errors_trinket(dialogs))
{  }


void
GPlatesGui::TrinketArea::init()
{
	// Set up UI connections and things here which don't exist until after
	// ViewportWindow's setupUi() has been called. Which includes the status bar.
	
	// The status bar itself normally draws a border around every item added to it,
	// which looks ugly. Apply a stylesheet that removes it.
	status_bar().setStyleSheet("QStatusBar::item {border: none;}");
	
	// Widgets are added to the bar in a left-to-right order.
	status_bar().addPermanentWidget(d_trinket_read_errors);
	connect(d_trinket_read_errors, SIGNAL(clicked(GPlatesQtWidgets::TrinketIcon *, QMouseEvent *)),
			this, SLOT(react_icon_clicked(GPlatesQtWidgets::TrinketIcon *, QMouseEvent *)));

	status_bar().addPermanentWidget(d_trinket_unsaved);
	connect(d_trinket_unsaved, SIGNAL(clicked(GPlatesQtWidgets::TrinketIcon *, QMouseEvent *)),
			this, SLOT(react_icon_clicked(GPlatesQtWidgets::TrinketIcon *, QMouseEvent *)));
}


QStatusBar &
GPlatesGui::TrinketArea::status_bar()
{
	return *d_viewport_window_ptr->statusBar();
}


void
GPlatesGui::TrinketArea::react_icon_clicked(
		GPlatesQtWidgets::TrinketIcon *icon,
		QMouseEvent *ev)
{
	// We don't actually need this now we have callback fn objects, which are a tiny bit
	// cleaner than an if-elseif chain here. Nevertheless, may prove useful later.
}


