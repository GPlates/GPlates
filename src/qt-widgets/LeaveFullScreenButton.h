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
 
#ifndef GPLATES_QTWIDGETS_LEAVEFULLSCREENBUTTON_H
#define GPLATES_QTWIDGETS_LEAVEFULLSCREENBUTTON_H

#include <QWidget>
#include "LeaveFullScreenButtonUi.h"


namespace GPlatesQtWidgets
{
	/**
	 * This button appears in the main window during full-screen mode.
	 * It provides the user with a visible means of escaping full
	 * screen mode.
	 */
	class LeaveFullScreenButton: 
			public QWidget,
			protected Ui_LeaveFullScreenButton
	{
		Q_OBJECT
		
	public:
	
		explicit
		LeaveFullScreenButton(
				QWidget *parent_ = NULL):
			QWidget(parent_)
		{
			setupUi(this);
			// Re-emit the triggered() signal from the real button, makes connect
			// in GPlatesGui::FullScreenMode easier.
			QObject::connect(button_leave_full_screen, SIGNAL(clicked()),
					this, SIGNAL(clicked()));
			// We will be hidden by default, until full-screen mode is activated.
			hide();
		}


		virtual
		~LeaveFullScreenButton()
		{  }
	
	signals:
	
		void
		clicked();

	};
}


#endif	// GPLATES_QTWIDGETS_LEAVEFULLSCREENBUTTON_H
