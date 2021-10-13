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

#include <QDebug>
#include <QDockWidget>
#include "DockState.h"

#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/DockWidget.h"


GPlatesGui::DockState::DockState(
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		QObject *parent_):
	QObject(parent_),
	d_viewport_window_ptr(&viewport_window_)
{
	setObjectName("DockState");
}


void
GPlatesGui::DockState::register_dock(
		GPlatesQtWidgets::DockWidget &dock)
{
	connect(&dock, SIGNAL(location_changed(
			GPlatesQtWidgets::DockWidget &, Qt::DockWidgetArea, bool)),
			this, SLOT(react_dockwidget_change(
			GPlatesQtWidgets::DockWidget &, Qt::DockWidgetArea, bool)));
}


void
GPlatesGui::DockState::move_dock(
		GPlatesQtWidgets::DockWidget &dock,
		Qt::DockWidgetArea area,
		bool tabify_as_appropriate)
{
	if (tabify_as_appropriate) {
		GPlatesQtWidgets::DockWidget *tabify_with = NULL;
		switch(area) {
		case Qt::TopDockWidgetArea:
				if ( ! d_docked_top.isEmpty()) {
					tabify_with = d_docked_top.last();
				}
				break;

		case Qt::BottomDockWidgetArea:
				if ( ! d_docked_bottom.isEmpty()) {
					tabify_with = d_docked_bottom.last();
				}
				break;

		case Qt::LeftDockWidgetArea:
				if ( ! d_docked_left.isEmpty()) {
					tabify_with = d_docked_left.last();
				}
				break;

		case Qt::RightDockWidgetArea:
				if ( ! d_docked_right.isEmpty()) {
					tabify_with = d_docked_right.last();
				}
				break;

		default:
				break;
		}
		if (tabify_with && tabify_with != &dock) {
			//qDebug() << this<<": Tabifying "<<&dock<<" with "<<tabify_with;
			d_viewport_window_ptr->tabifyDockWidget(tabify_with, &dock);
			return;
		}
   }
	// Default case for non-tabify and for tabify attempts that can't work:
	// Just move to that edge, and if there's something there already let Qt handle it.
	d_viewport_window_ptr->addDockWidget(area, &dock);
}


bool
GPlatesGui::DockState::can_tabify(
		Qt::DockWidgetArea area,
		const GPlatesQtWidgets::DockWidget &dock)
{
	// FIXME: Okay, this code is too repetitive. I could have structured it better. Sorry.
	switch(area) {
	case Qt::TopDockWidgetArea:
			if (d_docked_top.isEmpty()) {
				return false;
			} else if (d_docked_top.size() == 1 && d_docked_top.front() == &dock) {
				return false;
			} else {
				return true;
			}

	case Qt::BottomDockWidgetArea:
			if (d_docked_bottom.isEmpty()) {
   			return false;
   		} else if (d_docked_bottom.size() == 1 && d_docked_bottom.front() == &dock) {
      		return false;
      	} else {
	         return true;
         }

	case Qt::LeftDockWidgetArea:
			if (d_docked_left.isEmpty()) {
   			return false;
   		} else if (d_docked_left.size() == 1 && d_docked_left.front() == &dock) {
      		return false;
      	} else {
	         return true;
         }

	case Qt::RightDockWidgetArea:
			if (d_docked_right.isEmpty()) {
   			return false;
   		} else if (d_docked_right.size() == 1 && d_docked_right.front() == &dock) {
      		return false;
      	} else {
	         return true;
         }

	default:
			return false;
	}
}


void
GPlatesGui::DockState::react_dockwidget_change(
		GPlatesQtWidgets::DockWidget &dock,
		Qt::DockWidgetArea area,
		bool floating)
{
	if (floating) {
		remove_from_location_lists(&dock);
		d_floating.append(&dock);
	} else {
		switch(area) {
		case Qt::TopDockWidgetArea:
				remove_from_location_lists(&dock);
				d_docked_top.append(&dock);
				break;

		case Qt::BottomDockWidgetArea:
				remove_from_location_lists(&dock);
				d_docked_bottom.append(&dock);
				break;

		case Qt::LeftDockWidgetArea:
				remove_from_location_lists(&dock);
				d_docked_left.append(&dock);
				break;

		case Qt::RightDockWidgetArea:
				remove_from_location_lists(&dock);
				d_docked_right.append(&dock);
				break;

		default:
				break;
		}
	}
	
	// Notify all docks, so that menus are updated to be accurate.
	emit dock_configuration_changed();
}

void
GPlatesGui::DockState::remove_from_location_lists(
		GPlatesQtWidgets::DockWidget *remove)
{
	d_floating.removeAll(remove);
	d_docked_top.removeAll(remove);
	d_docked_bottom.removeAll(remove);
	d_docked_left.removeAll(remove);
	d_docked_right.removeAll(remove);
}


