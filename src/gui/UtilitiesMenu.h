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
 
#ifndef GPLATES_GUI_UTILITIESMENU_H
#define GPLATES_GUI_UTILITIESMENU_H

#include <map>
#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>
#include <QObject>
#include <QMenu>
#include <QString>

#include "api/Sleeper.h"
#include "gui/PythonManager.h"


namespace GPlatesApi
{
	class PythonExecutionThread;
}

namespace GPlatesGui
{
	/**
	 * This class allows Python scripts to register themselves onto the Utilities
	 * menu and handles their execution when a menu item is selected.
	 */
	class UtilitiesMenu :
			public QObject
	{
		Q_OBJECT
	
	public:

		UtilitiesMenu(
				QMenu *utilities_menu,
				QAction *before_action,
				GPlatesGui::PythonManager& python_manager,
				QObject *parent_ = NULL);

		~UtilitiesMenu();

		void
		add_utility(
				const QString &category,
				const QString &name,
				const boost::function< void () > &callback);

	private Q_SLOTS:

		void
		handle_action_triggered();

	private:

		QMenu *
		get_category_menu(
				const QString &category);

		QMenu *d_utilities_menu;
		QAction *d_before_action;
		GPlatesGui::PythonManager& d_python_manager;

		typedef std::map<QString, QMenu *> submenus_map_type;
		submenus_map_type d_submenus;
	};
}

#endif	// GPLATES_GUI_UTILITIESMENU_H
