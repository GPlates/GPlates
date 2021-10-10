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
 
#ifndef GPLATES_QTWIDGETS_EDITENUMERATIONWIDGET_H
#define GPLATES_QTWIDGETS_EDITENUMERATIONWIDGET_H

#include "AbstractEditWidget.h"
#include "property-values/Enumeration.h"

#include "EditEnumerationWidgetUi.h"

namespace GPlatesQtWidgets
{
	class EditEnumerationWidget:
			public AbstractEditWidget, 
			protected Ui_EditEnumerationWidget
	{
		Q_OBJECT
		
	public:
		explicit
		EditEnumerationWidget(
				QWidget *parent_ = NULL);
		
		virtual
		void
		configure_for_property_value_type(
				const QString &property_value_name);

		virtual
		void
		reset_widget_to_default_values();


		void
		update_widget_from_enumeration(
				const GPlatesPropertyValues::Enumeration &enumeration);

		virtual
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget() const;

	private slots:
	
		void
		handle_combobox_change()
		{
			set_dirty();
			emit commit_me();
		}
	
	private:
		
		/**
		 * The name of the PropertyValue which this widget is currently configured
		 * to produce.
		 */
		QString d_property_value_name;
	};
}

#endif  // GPLATES_QTWIDGETS_EDITENUMERATIONWIDGET_H

