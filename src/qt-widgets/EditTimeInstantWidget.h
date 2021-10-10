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
 
#ifndef GPLATES_QTWIDGETS_EDITTIMEINSTANTWIDGET_H
#define GPLATES_QTWIDGETS_EDITTIMEINSTANTWIDGET_H

#include "AbstractEditWidget.h"
#include "property-values/GmlTimeInstant.h"

#include "EditTimeInstantWidgetUi.h"

namespace GPlatesQtWidgets
{
	class EditTimeInstantWidget:
			public AbstractEditWidget, 
			protected Ui_EditTimeInstantWidget
	{
		Q_OBJECT
		
	public:
		explicit
		EditTimeInstantWidget(
				QWidget *parent_ = NULL);
		
		virtual
		void
		reset_widget_to_default_values();

		void
		update_widget_from_time_instant(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		virtual
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget() const;

	};
}

#endif  // GPLATES_QTWIDGETS_EDITTIMEINSTANTWIDGET_H

