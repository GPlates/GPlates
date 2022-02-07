/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <QMetaType>
#include <QVariant>

#include "ImportMenu.h"


Q_DECLARE_METATYPE( boost::function< void () > )


GPlatesGui::ImportMenu::ImportMenu(
		QMenu *import_menu,
		QMenu *parent_menu,
		QObject *parent_) :
	QObject(parent_),
	d_import_menu(import_menu),
	d_parent_menu(parent_menu),
	d_next_action_in_parent_menu(NULL),
	d_action_group(new QActionGroup(this))
{
	// Clear the Import menu and remove it from the parent menu. Before removing it
	// from the parent menu, we remember the action following the Import menu, so
	// we know where to add ourselves back.
	//
	// Do not add items to the Import menu via the designer. Please add items
	// programmatically using methods of this class.
	d_import_menu->clear();
	QList<QAction *> parent_actions = d_parent_menu->actions();
	int import_menu_index = parent_actions.indexOf(d_import_menu->menuAction());
	if (import_menu_index != -1 && import_menu_index != parent_actions.size() - 1)
	{
		d_next_action_in_parent_menu = parent_actions.at(import_menu_index + 1);
	}
	d_parent_menu->removeAction(d_import_menu->menuAction());

	// Create separators to bookend the sections.
	for (std::size_t i = 0; i != static_cast<std::size_t>(NUM_SECTIONS); ++i)
	{
		if (i == static_cast<std::size_t>(NUM_SECTIONS) - 1)
		{
			// This relies on the fact that insertAction called with a before parameter
			// of NULL inserts at the end of the menu.
			d_section_end_actions[i] = NULL;
		}
		else
		{
			QAction *separator = new QAction(d_import_menu);
			separator->setSeparator(true);
			d_import_menu->addAction(separator);
			d_section_end_actions[i] = separator;
		}
	}

	// Listen to all the actions from one place.
	QObject::connect(
			d_action_group,
			SIGNAL(triggered(QAction *)),
			this,
			SLOT(handle_action_triggered(QAction *)));
}


void
GPlatesGui::ImportMenu::add_import(
		Section section,
		const QString &text,
		const boost::function< void () > &callback)
{
	// Add the Import menu back to its parent menu if necessary.
	if (d_next_action_in_parent_menu)
	{
		d_parent_menu->insertAction(d_next_action_in_parent_menu, d_import_menu->menuAction());
		d_next_action_in_parent_menu = NULL;
	}

	// Create a new action.
	QAction *action = new QAction(text, d_import_menu);
	QVariant qv;
	qv.setValue(callback);
	action->setData(qv);

	// Insert the action in the right place.
	d_import_menu->insertAction(
			d_section_end_actions[static_cast<std::size_t>(section)],
			action);
	d_action_group->addAction(action);
}


void
GPlatesGui::ImportMenu::handle_action_triggered(
		QAction *action)
{
	// Extract the callback from the action and call it.
	typedef boost::function< void () > callback_type;
	callback_type callback = action->data().value<callback_type>();
	callback();
}

