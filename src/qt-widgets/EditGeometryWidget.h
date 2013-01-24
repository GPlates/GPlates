/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_EDITGEOMETRYWIDGET_H
#define GPLATES_QTWIDGETS_EDITGEOMETRYWIDGET_H

#include <boost/intrusive_ptr.hpp>
#include <boost/optional.hpp>

#include "AbstractEditWidget.h"
#include "EditGeometryWidgetUi.h"
#include "EditTableWidget.h"

#include "model/types.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"

#include "view-operations/GeometryType.h"


namespace GPlatesQtWidgets
{
	class EditTableActionWidget;
	
	class EditGeometryWidget:
			public AbstractEditWidget, 
			public EditTableWidget,
			protected Ui_EditGeometryWidget
	{
		Q_OBJECT
		
	public:
		explicit
		EditGeometryWidget(
				QWidget *parent_ = NULL);
		
		virtual
		void
		reset_widget_to_default_values();
		
		virtual
		void
		configure_for_property_value_type(
				const GPlatesPropertyValues::StructuralType &property_value_type);

		void
		update_widget_from_line_string(
				GPlatesPropertyValues::GmlLineString &gml_line_string);

		void
		update_widget_from_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		void
		update_widget_from_point(
				GPlatesPropertyValues::GmlPoint &gml_point);
		
		void
		update_widget_from_polygon(
				GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget() const;

		virtual
		bool
		update_property_value_from_widget();
	

		virtual
		void
		handle_insert_row_above(
				const EditTableActionWidget *action_widget);

		virtual
		void
		handle_insert_row_below(
				const EditTableActionWidget *action_widget);
	
		virtual
		void
		handle_delete_row(
				const EditTableActionWidget *action_widget);
	
		/**
		 * Adds a new point to end of the current geometry in the table.
		 */
		void
		append_point_to_table(
				double lat,
				double lon);

	private Q_SLOTS:
	
		/**
		 * Fired when the data of a cell has been modified.
		 *
		 * FIXME: ... by the user?
		 */
		void
		handle_cell_changed(int row, int column);
		
		/**
		 * Manages data entry focus for the "Append Point" widgets.
		 */
		void
		append_point_clicked()
		{
			append_point_to_table(spinbox_lat->value(), spinbox_lon->value());
			spinbox_lat->setFocus();
			spinbox_lat->selectAll();
		}
		

		/**
		 * Creates an EditTableActionWidget item in the current row.
		 */
		void
		handle_current_cell_changed(
				int currentRow, int currentColumn, int previousRow, int previousColumn);
		
	private:
		
		/**
		 * Finds the current table row associated with the EditTableActionWidget.
		 * Will return -1 in the event that the action widget provided is not in
		 * the table.
		 */
		int
		get_row_for_action_widget(
				const EditTableActionWidget *action_widget);
		
		/**
		 * Adds a new blank point to the current geometry in the table.
		 * The new row will be inserted to take the place of row index @a row.
		 */
		void
		insert_blank_point_into_table(
				int row);
		
		/**
		 * Removes a single point from the current geometry in the table.
		 */
		void
		delete_point_from_table(
				int row);

		/**
		 * Does appropriate tests for the current geometry of the table,
		 * and updates the interface to provide feedback to the user.
		 *
		 * Returns true if there are absolutely no problems with the geometry.
		 */
		bool
		test_geometry_validity();

		/**
		 * Creates GeometryOnSphere and uses setters to place it inside
		 * the current PropertyValue being edited.
		 */
		bool
		set_geometry_for_property_value();

		/**
		 * The type of geometry being edited.
		 */
		GPlatesViewOperations::GeometryType::Value d_geometry_type;
		
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
		boost::intrusive_ptr<GPlatesModel::PropertyValue> d_property_value_ptr;

	};
}

#endif  // GPLATES_QTWIDGETS_EDITGEOMETRYWIDGET_H

