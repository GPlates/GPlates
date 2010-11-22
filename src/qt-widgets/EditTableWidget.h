/* $Id: EditTableWidget.h 6415 2009-08-06 13:25:03Z rwatson $ */

/**
* \file 
* $Revision: 6415 $
* $Date: 2009-08-06 15:25:03 +0200 (to, 06 aug 2009) $ 
* 
* Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_EDITTABLEWIDGET_H
#define GPLATES_QTWIDGETS_EDITTABLEWIDGET_H

/**
 *  An abstract base class for classes which will make use of the EditTableActionWidget.                                                                     
 */
namespace GPlatesQtWidgets
{
	class EditTableActionWidget;
	
	class EditTableWidget{
		
	public:

		virtual
		~EditTableWidget()
		{  }
	
		virtual
		void
		handle_insert_row_above(
			const EditTableActionWidget *) = 0;
	
		virtual
		void
		handle_insert_row_below(
			const EditTableActionWidget *) = 0;
		
		virtual
		void
		handle_delete_row(
			const EditTableActionWidget *) = 0;
			
	};
}

#endif // GPLATES_QTWIDGETS_EDITTABLEWIDGET_H

