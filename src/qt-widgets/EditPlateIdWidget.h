/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_EDITPLATEIDWIDGET_H
#define GPLATES_QTWIDGETS_EDITPLATEIDWIDGET_H

#include "AbstractEditWidget.h"
#include "property-values/GpmlPlateId.h"

#include "EditPlateIdWidgetUi.h"

namespace GPlatesQtWidgets
{
	class EditPlateIdWidget:
			public AbstractEditWidget, 
			protected Ui_EditPlateIdWidget
	{
		Q_OBJECT
		
	public:
		explicit
		EditPlateIdWidget(
				QWidget *parent_ = NULL);
		
		virtual
		void
		reset_widget_to_default_values();

		void
		update_widget_from_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		virtual
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget() const;

	};
}

#endif  // GPLATES_QTWIDGETS_EDITPLATEIDWIDGET_H

