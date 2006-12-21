/* $Id: gplates_main.cc 968 2006-11-20 03:28:31Z jboyden $ */

/**
 * \file 
 * $Revision: 968 $
 * $Date: 2006-11-20 14:28:31 +1100 (Mon, 20 Nov 2006) $ 
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#include "gui/Interface.h"

#include "gui-qt/DesignerMainWindow.h"

namespace GPlatesInterface
{
	class QtInterface :
			public Interface
	{
	public:
		// Routines to create Qt-specific widgets go here.
		
		void
		create_main_window(
				const std::string &title) 
		{		
			d_main_window = new QMainWindow(title);
			
			d_ui_main_window.setupUi(d_main_window);
		}
				
	private:
		QMainWindow d_main_window;
		
		Ui_MainWindow d_ui_main_window;
	};
}
