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
#include <QDebug>

#include "UtilitiesMenu.h"

#include "api/PythonExecutionMonitor.h"
#include "api/PythonExecutionThread.h"
#include "api/PythonInterpreterLocker.h"
#include "api/Sleeper.h"

#include <boost/foreach.hpp>

#if !defined(GPLATES_NO_PYTHON)

Q_DECLARE_METATYPE( boost::function< void () > )

GPlatesGui::UtilitiesMenu::UtilitiesMenu(
		QMenu *utilities_menu,
		QAction *before_action,
		GPlatesGui::PythonManager& python_manager,
		QObject *parent_) :
	QObject(parent_),
	d_utilities_menu(utilities_menu),
	d_before_action(utilities_menu->insertSeparator(before_action)),
	d_python_manager(python_manager)
{ }



GPlatesGui::UtilitiesMenu::~UtilitiesMenu()
{ }


void
GPlatesGui::UtilitiesMenu::add_utility(
		const QString &category,
		const QString &name,
		const boost::function< void () > &callback)
{
	QMenu *cat_menu = get_category_menu(category);

	QAction *new_action = new QAction(name, this);
	QVariant qv;
	qv.setValue(callback);
	new_action->setData(qv);
	QObject::connect(
			new_action,
			SIGNAL(triggered()),
			this,
			SLOT(handle_action_triggered()));
	cat_menu->addAction(new_action);
}


void
GPlatesGui::UtilitiesMenu::handle_action_triggered()
{
	QAction *action = qobject_cast<QAction *>(sender());
	// Extract the callback from the action and call it.
	typedef boost::function< void () > callback_type;
	callback_type callback = action->data().value<callback_type>();

#if !defined(GPLATES_NO_PYTHON)
	d_python_manager.get_python_execution_thread()->exec_function(callback);
#else
	callback();
#endif
}


QMenu *
GPlatesGui::UtilitiesMenu::get_category_menu(
		const QString &category)
{
	submenus_map_type::const_iterator iter = d_submenus.find(category);
	if (iter == d_submenus.end())
	{
		// Create a new submenu for this category.
		QMenu *new_menu = new QMenu(category, d_utilities_menu);
		d_utilities_menu->insertMenu(d_before_action, new_menu);
		d_submenus.insert(std::make_pair(category, new_menu));
		return new_menu;
	}
	else
	{
		return iter->second;
	}
}
#endif

