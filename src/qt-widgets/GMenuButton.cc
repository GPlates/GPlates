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

#include <QMenuBar>
#include <QList>
#include <QAction>

#include "GMenuButton.h"


GPlatesQtWidgets::GMenuButton::GMenuButton(
		GPlatesQtWidgets::ViewportWindow &main_window,
		QWidget *parent_ = NULL):
	QWidget(parent_),
	d_menu_ptr(new QMenu(this))
{
	setupUi(this);
	
	// We will be hidden by default, until full-screen mode is activated.
	hide();

	// Set up the GMenu.
	d_menu_ptr->setObjectName("GMenu");
	button_gmenu->setMenu(d_menu_ptr);
	
	// The rest of setting up the GMenu currently has to be done in ViewportWindow
	// for fun reasons related to setupUi.
}


