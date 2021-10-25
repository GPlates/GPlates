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

#include <QAction>
#include <QIcon>
#include "DockWidget.h"

#include "gui/DockState.h"
#include "ViewportWindow.h"


GPlatesQtWidgets::DockWidget::DockWidget(
		const QString &title,
		GPlatesGui::DockState &dock_state,
		ViewportWindow &main_window,
		boost::optional<QString> object_name_suffix):
	QDockWidget(title, &main_window),
	d_dock_state_ptr(&dock_state)
{
	// All GUI stuff should have an object name so we can e.g. hide it in F11.
	setObjectName(QString("Dock_%1").arg(object_name_suffix ? object_name_suffix.get() : title));

	// Connect to some of our own slots, so we can re-emit to DockState.
	connect(this, SIGNAL(topLevelChanged(bool)),
			this, SLOT(handle_floating_change(bool)));
	connect(this, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
			this, SLOT(handle_location_change(Qt::DockWidgetArea)));
	// And ensure DockState gets the message.
	d_dock_state_ptr->register_dock(*this);
	
	// DockState can also tell us when this or other docks have changed,
	// so we know to update our menu items.
	connect(d_dock_state_ptr, SIGNAL(dock_configuration_changed()),
			this, SLOT(hide_menu_items_as_appropriate()));
	
	// React to changes to the allowed dock areas (when 'setAllowedAreas()' called).
	connect(this, SIGNAL(allowedAreasChanged(Qt::DockWidgetAreas)),
			this, SLOT(hide_menu_items_as_appropriate()));

	// No designer setupUi() here, we define our own UI elements manually.
	set_up_context_menu();

	// Handle case where some areas are not allowed.
	hide_menu_items_as_appropriate();
}


void
GPlatesQtWidgets::DockWidget::set_up_context_menu()
{
	d_action_Dock_At_Top = new QAction(QIcon(":/gnome_go_up_16.png"), tr("Dock at &Top"), this);
	d_action_Dock_At_Bottom = new QAction(QIcon(":/gnome_go_down_16.png"), tr("Dock at &Bottom"), this);
	d_action_Dock_At_Left = new QAction(QIcon(":/gnome_go_previous_16.png"), tr("Dock at &Left"), this);
	d_action_Dock_At_Right = new QAction(QIcon(":/gnome_go_next_16.png"), tr("Dock at &Right"), this);
	d_action_Tabify_At_Top = new QAction(QIcon(":/gnome_go_up_16.png"), tr("Tabify at &Top"), this);
	d_action_Tabify_At_Bottom = new QAction(QIcon(":/gnome_go_down_16.png"), tr("Tabify at &Bottom"), this);
	d_action_Tabify_At_Left = new QAction(QIcon(":/gnome_go_previous_16.png"), tr("Tabify at &Left"), this);
	d_action_Tabify_At_Right = new QAction(QIcon(":/gnome_go_next_16.png"), tr("Tabify at &Right"), this);
	
	addAction(d_action_Dock_At_Top);
	addAction(d_action_Dock_At_Bottom);
	addAction(d_action_Dock_At_Left);
	addAction(d_action_Dock_At_Right);
	addAction(d_action_Tabify_At_Top);
	addAction(d_action_Tabify_At_Bottom);
	addAction(d_action_Tabify_At_Left);
	addAction(d_action_Tabify_At_Right);
	setContextMenuPolicy(Qt::ActionsContextMenu);
	
	QObject::connect(d_action_Dock_At_Top, SIGNAL(triggered()),
			this, SLOT(dock_at_top()));
	QObject::connect(d_action_Dock_At_Bottom, SIGNAL(triggered()),
			this, SLOT(dock_at_bottom()));
	QObject::connect(d_action_Dock_At_Left, SIGNAL(triggered()),
			this, SLOT(dock_at_left()));
	QObject::connect(d_action_Dock_At_Right, SIGNAL(triggered()),
			this, SLOT(dock_at_right()));
	QObject::connect(d_action_Tabify_At_Top, SIGNAL(triggered()),
			this, SLOT(tabify_at_top()));
	QObject::connect(d_action_Tabify_At_Bottom, SIGNAL(triggered()),
			this, SLOT(tabify_at_bottom()));
	QObject::connect(d_action_Tabify_At_Left, SIGNAL(triggered()),
			this, SLOT(tabify_at_left()));
	QObject::connect(d_action_Tabify_At_Right, SIGNAL(triggered()),
			this, SLOT(tabify_at_right()));
}


void
GPlatesQtWidgets::DockWidget::dock_at_top()
{
	//qDebug() << this<<"::dock_at_top()";
	setFloating(false);
	d_dock_state_ptr->move_dock(*this, Qt::TopDockWidgetArea, false);
}

void
GPlatesQtWidgets::DockWidget::dock_at_bottom()
{
	//qDebug() << this<<"::dock_at_bottom()";
	setFloating(false);
	d_dock_state_ptr->move_dock(*this, Qt::BottomDockWidgetArea, false);
}

void
GPlatesQtWidgets::DockWidget::dock_at_left()
{
	//qDebug() << this<<"::dock_at_left()";
	setFloating(false);
	d_dock_state_ptr->move_dock(*this, Qt::LeftDockWidgetArea, false);
}

void
GPlatesQtWidgets::DockWidget::dock_at_right()
{
	//qDebug() << this<<"::dock_at_right()";
	setFloating(false);
	d_dock_state_ptr->move_dock(*this, Qt::RightDockWidgetArea, false);
}


void
GPlatesQtWidgets::DockWidget::tabify_at_top()
{
	//qDebug() << this<<"::tabify_at_top()";
	setFloating(false);
	d_dock_state_ptr->move_dock(*this, Qt::TopDockWidgetArea, true);
}

void
GPlatesQtWidgets::DockWidget::tabify_at_bottom()
{
	//qDebug() << this<<"::tabify_at_bottom()";
	setFloating(false);
	d_dock_state_ptr->move_dock(*this, Qt::BottomDockWidgetArea, true);
}

void
GPlatesQtWidgets::DockWidget::tabify_at_left()
{
	//qDebug() << this<<"::tabify_at_left()";
	setFloating(false);
	d_dock_state_ptr->move_dock(*this, Qt::LeftDockWidgetArea, true);
}

void
GPlatesQtWidgets::DockWidget::tabify_at_right()
{
	//qDebug() << this<<"::tabify_at_right()";
	setFloating(false);
	d_dock_state_ptr->move_dock(*this, Qt::RightDockWidgetArea, true);
}


void
GPlatesQtWidgets::DockWidget::handle_floating_change(
		bool floating)
{
	//qDebug() << this<<"::handle_floating_change("<<floating<<")";
	Q_EMIT location_changed(*this, Qt::NoDockWidgetArea, floating);
}

void
GPlatesQtWidgets::DockWidget::handle_location_change(
		Qt::DockWidgetArea area)
{
	//qDebug() << this<<"::handle_location_change("<<area<<")";
	Q_EMIT location_changed(*this, area, false);
}


void
GPlatesQtWidgets::DockWidget::hide_menu_items_as_appropriate()
{
	const Qt::DockWidgetAreas allowed_areas = allowedAreas();

	// Can dock at any allowed location except the current dock location (cause already there).
	d_action_Dock_At_Top->setVisible(
			allowed_areas.testFlag(Qt::TopDockWidgetArea) &&
			d_dock_state_ptr->can_dock(Qt::TopDockWidgetArea, *this));
	d_action_Dock_At_Bottom->setVisible(
			allowed_areas.testFlag(Qt::BottomDockWidgetArea) &&
			d_dock_state_ptr->can_dock(Qt::BottomDockWidgetArea, *this));
	d_action_Dock_At_Left->setVisible(
			allowed_areas.testFlag(Qt::LeftDockWidgetArea) &&
			d_dock_state_ptr->can_dock(Qt::LeftDockWidgetArea, *this));
	d_action_Dock_At_Right->setVisible(
			allowed_areas.testFlag(Qt::RightDockWidgetArea) &&
			d_dock_state_ptr->can_dock(Qt::RightDockWidgetArea, *this));

	// Can tabify at any allowed location except the current dock location (cause already there).
	d_action_Tabify_At_Top->setVisible(
			allowed_areas.testFlag(Qt::TopDockWidgetArea) &&
			d_dock_state_ptr->can_tabify(Qt::TopDockWidgetArea, *this));
	d_action_Tabify_At_Bottom->setVisible(
			allowed_areas.testFlag(Qt::BottomDockWidgetArea) &&
			d_dock_state_ptr->can_tabify(Qt::BottomDockWidgetArea, *this));
	d_action_Tabify_At_Left->setVisible(
			allowed_areas.testFlag(Qt::LeftDockWidgetArea) &&
			d_dock_state_ptr->can_tabify(Qt::LeftDockWidgetArea, *this));
	d_action_Tabify_At_Right->setVisible(
			allowed_areas.testFlag(Qt::RightDockWidgetArea) &&
			d_dock_state_ptr->can_tabify(Qt::RightDockWidgetArea, *this));
}
