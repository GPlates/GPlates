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

#include <algorithm>
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
			this, SLOT(react_dockwidget_location_change(
			GPlatesQtWidgets::DockWidget &, Qt::DockWidgetArea, bool)));
}


bool
GPlatesGui::DockState::can_dock(
		Qt::DockWidgetArea area,
		const GPlatesQtWidgets::DockWidget &dock) const
{
	switch(area)
	{
	case Qt::TopDockWidgetArea:
		return can_dock(dock, d_docked_top, d_tabified_top);

	case Qt::BottomDockWidgetArea:
		return can_dock(dock, d_docked_bottom, d_tabified_bottom);

	case Qt::LeftDockWidgetArea:
		return can_dock(dock, d_docked_left, d_tabified_left);

	case Qt::RightDockWidgetArea:
		return can_dock(dock, d_docked_right, d_tabified_right);

	default:
		return false;
	}
}


bool
GPlatesGui::DockState::can_dock(
		const GPlatesQtWidgets::DockWidget &dock,
		const QList< QPointer<GPlatesQtWidgets::DockWidget> > &docked_area_list,
		const QList< QPointer<GPlatesQtWidgets::DockWidget> > &tabified_area_list) const
{
	//
	// 'dock' can dock in 'area' if:
	//  * 'dock' is already tabified in 'area', or
	//  * 'dock' is *not* already docked (but untabified) in 'area'.
	//

	// Is 'dock' already in 'area'...
	// Can't use 'QList::contains()'...
	if (std::find(docked_area_list.begin(), docked_area_list.end(), &dock) != docked_area_list.end())
	{
		// Is 'dock' already tabified (in 'area')...
		// If 'dock' is tabified (in 'area') then it can be docked, otherwise it cannot be docked
		// (in 'area') because it is already docked there.
		// Can't use 'QList::contains()'...
		return std::find(tabified_area_list.begin(), tabified_area_list.end(), &dock) != tabified_area_list.end();
	}

	// 'dock' is *not* docked in 'area' which means it is available for docking there.
	return true;
}


bool
GPlatesGui::DockState::can_tabify(
		Qt::DockWidgetArea area,
		const GPlatesQtWidgets::DockWidget &dock) const
{
	switch(area)
	{
	case Qt::TopDockWidgetArea:
		return can_tabify(dock, d_docked_top, d_tabified_top);

	case Qt::BottomDockWidgetArea:
		return can_tabify(dock, d_docked_bottom, d_tabified_bottom);

	case Qt::LeftDockWidgetArea:
		return can_tabify(dock, d_docked_left, d_tabified_left);

	case Qt::RightDockWidgetArea:
		return can_tabify(dock, d_docked_right, d_tabified_right);

	default:
		return false;
	}
}


bool
GPlatesGui::DockState::can_tabify(
		const GPlatesQtWidgets::DockWidget &dock,
		const QList< QPointer<GPlatesQtWidgets::DockWidget> > &docked_area_list,
		const QList< QPointer<GPlatesQtWidgets::DockWidget> > &tabified_area_list) const
{
	//
	// 'dock' can tabify in 'area' if:
	//  * 'dock' is not already tabified in 'area', and
	//  * there is another widget in 'area' (ie, not 'dock').
	//

	// Is 'dock' already in 'area'...
	// Can't use 'QList::contains()'...
	if (std::find(docked_area_list.begin(), docked_area_list.end(), &dock) != docked_area_list.end())
	{
		// Is 'dock' already tabified (in 'area')...
		// Can't use 'QList::contains()'...
		if (std::find(tabified_area_list.begin(), tabified_area_list.end(), &dock) != tabified_area_list.end())
		{
			return false;
		}

		// Is there another widget docked in the area besides 'dock'...
		// 'dock' is docked in 'area' (but not tabified) so there needs to be another dock in same area.
		return docked_area_list.size() > 1;
	}

	// 'dock' is *not* docked in 'area' so there needs to be at least one dock in 'area'.
	return !docked_area_list.isEmpty();
}


void
GPlatesGui::DockState::move_dock(
		GPlatesQtWidgets::DockWidget &dock,
		Qt::DockWidgetArea area,
		bool tabify_as_appropriate)
{
	if (tabify_as_appropriate)
	{
		switch(area)
		{
		case Qt::TopDockWidgetArea:
			if (tabify(dock, d_docked_top, d_tabified_top))
			{
				return;
			}
			break;

		case Qt::BottomDockWidgetArea:
			if (tabify(dock, d_docked_bottom, d_tabified_bottom))
			{
				return;
			}
			break;

		case Qt::LeftDockWidgetArea:
			if (tabify(dock, d_docked_left, d_tabified_left))
			{
				return;
			}
			break;

		case Qt::RightDockWidgetArea:
			if (tabify(dock, d_docked_right, d_tabified_right))
			{
				return;
			}
			break;

		default:
			break;
		}
	}

	// Remove 'dock' from the tabify lists (if it's in any).
	// It's now docked but not tabified.
	remove_from_tabified_lists(&dock);

	// Default case for non-tabify and for tabify attempts that can't work:
	// Just move to that edge, and if there's something there already let Qt handle it.
	d_viewport_window_ptr->addDockWidget(area, &dock);
}


bool
GPlatesGui::DockState::tabify(
		GPlatesQtWidgets::DockWidget &dock,
		QList< QPointer<GPlatesQtWidgets::DockWidget> > &docked_area_list,
		QList< QPointer<GPlatesQtWidgets::DockWidget> > &tabified_area_list)
{
	//
	// 'dock' can tabify in 'area' if:
	//  * 'dock' is not already tabified in 'area', and
	//  * there is another widget in 'area' (ie, not 'dock').
	//

	// Is 'dock' already in 'area'...
	// Can't use 'QList::contains()'...
	if (std::find(docked_area_list.begin(), docked_area_list.end(), &dock) != docked_area_list.end())
	{
		// Is 'dock' already tabified (in 'area')...
		// Can't use 'QList::contains()'...
		if (std::find(tabified_area_list.begin(), tabified_area_list.end(), &dock) != tabified_area_list.end())
		{
			return false;
		}

		// Is there another widget docked in the area besides 'dock'...
		// 'dock' is docked in 'area' (but not tabified) so there needs to be another dock in same area.
		if (docked_area_list.size() == 1)
		{
			return false;
		}

		// Pick any dock widget, that is not 'dock', to tabify with.
		GPlatesQtWidgets::DockWidget *tabify_with =
				(docked_area_list.first() == &dock)
				? docked_area_list.last()
				: docked_area_list.first();

		// Add 'tabify_with' to the tabify list if the list is empty.
		if (tabified_area_list.isEmpty())
		{
			tabified_area_list.append(tabify_with);
		}
		// Add 'dock' to the tabify list.
		tabified_area_list.append(&dock);

		// Do the actual tabify last since it will emit a signal and then clients will query our state.
		d_viewport_window_ptr->tabifyDockWidget(tabify_with, &dock);

		return true;
	}

	// 'dock' is *not* docked in 'area' so there needs to be at least one dock in 'area'.
	if (docked_area_list.isEmpty())
	{
		return false;
	}

	// Pick any dock widget, in the area, to tabify with.
	GPlatesQtWidgets::DockWidget *tabify_with = docked_area_list.first();

	// Add 'tabify_with' to the tabify list if the list is empty.
	if (tabified_area_list.isEmpty())
	{
		tabified_area_list.append(tabify_with);
	}
	// Add 'dock' to the tabify list.
	tabified_area_list.append(&dock);

	// Do the actual tabify last since it will emit a signal and then clients will query our state.
	d_viewport_window_ptr->tabifyDockWidget(tabify_with, &dock);

	return true;
}


void
GPlatesGui::DockState::react_dockwidget_location_change(
		GPlatesQtWidgets::DockWidget &dock,
		Qt::DockWidgetArea area,
		bool floating)
{
	if (floating) {
		remove_from_docked_lists(&dock);
		d_floating.append(&dock);
	} else {
		switch(area) {
		case Qt::TopDockWidgetArea:
				remove_from_docked_lists(&dock);
				d_docked_top.append(&dock);
				break;

		case Qt::BottomDockWidgetArea:
				remove_from_docked_lists(&dock);
				d_docked_bottom.append(&dock);
				break;

		case Qt::LeftDockWidgetArea:
				remove_from_docked_lists(&dock);
				d_docked_left.append(&dock);
				break;

		case Qt::RightDockWidgetArea:
				remove_from_docked_lists(&dock);
				d_docked_right.append(&dock);
				break;

		default:
				break;
		}
	}
	
	// Notify all docks, so that menus are updated to be accurate.
	Q_EMIT dock_configuration_changed();
}


void
GPlatesGui::DockState::remove_from_docked_lists(
		GPlatesQtWidgets::DockWidget *remove)
{
	d_floating.removeAll(remove);
	d_docked_top.removeAll(remove);
	d_docked_bottom.removeAll(remove);
	d_docked_left.removeAll(remove);
	d_docked_right.removeAll(remove);
}


void
GPlatesGui::DockState::remove_from_tabified_lists(
		GPlatesQtWidgets::DockWidget *remove)
{
	// If 'remove' is removed from a tabified list and there is only one remaining dock
	// in that area then it is also removed from the list (because it is also no longer tabified).

	if (d_tabified_top.removeAll(remove) &&
		d_tabified_top.size() == 1)
	{
		d_tabified_top.clear();
	}

	if (d_tabified_bottom.removeAll(remove) &&
		d_tabified_bottom.size() == 1)
	{
		d_tabified_bottom.clear();
	}

	if (d_tabified_left.removeAll(remove) &&
		d_tabified_left.size() == 1)
	{
		d_tabified_left.clear();
	}

	if (d_tabified_right.removeAll(remove) &&
		d_tabified_right.size() == 1)
	{
		d_tabified_right.clear();
	}
}
