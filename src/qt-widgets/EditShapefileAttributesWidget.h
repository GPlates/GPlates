/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_EDITSHAPEFILEATTRIBUTESWIDGET_H
#define GPLATES_QTWIDGETS_EDITSHAPEFILEATTRIBUTESWIDGET_H

#include "property-values/GpmlKeyValueDictionary.h"

#include "AbstractEditWidget.h"
#include "EditShapefileAttributesWidgetUi.h"

namespace GPlatesQtWidgets
{
	class EditShapefileAttributesWidget:
		public AbstractEditWidget,
		protected Ui_EditShapefileAttributesWidget
	{
		Q_OBJECT

	public:
		explicit
		EditShapefileAttributesWidget(
			QWidget *parent_=NULL);

		virtual
		void
		reset_widget_to_default_values();
	
		virtual
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget() const;

		virtual
		bool
		update_property_value_from_widget();

		void
		update_widget_from_key_value_dictionary(
			GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary);

	private slots:

		/**
		 * Handle the content of a cell changing. 
		 */
		void
		handle_cell_changed(
			int row,
			int column);

	private:

		/**
		 * This boost::intrusive_ptr is used to remember the property value which
		 * was last loaded into this editing widget. This is done so that the
		 * edit widget can directly update the property value later.
		 *
		 * We need to use a reference-counting pointer to make sure the property
		 * value doesn't disappear while this edit widget is active; however, since
		 * the property value is not known at the time the widget is created,
		 * the pointer may be NULL and this must be checked for.
		 *
		 * The pointer will also be NULL when the edit widget is being used for
		 * adding brand new properties to the model.
		 */
		boost::intrusive_ptr<GPlatesPropertyValues::GpmlKeyValueDictionary> d_key_value_dictionary_ptr;
	};
}

#endif // GPLATES_QTWIDGETS_EDITSHAPEFILEATTRIBUTESWIDGET_H
