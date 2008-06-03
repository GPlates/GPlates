/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_QTWIDGETS_EDITWIDGETCHOOSER_H
#define GPLATES_QTWIDGETS_EDITWIDGETCHOOSER_H

#include <vector>
#include <QWidget>
#include "model/FeatureVisitor.h"
#include "qt-widgets/EditWidgetGroupBox.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"


namespace GPlatesQtWidgets
{
	/**
	 * The EditWidgetChooser feature-visitor is used to help identify a widget suitable
	 * for editing a given property value (or given a feature reference and a property name)
	 * It is used by an EditWidgetGroupBox.
	 */
	class EditWidgetChooser:
			public GPlatesModel::FeatureVisitor
	{
	public:

		typedef std::vector<QVariant> qvariant_container_type;

		// FIXME:  We should also pass the current reconstruction time, so we can correctly
		// handle time-dependent property values.
		explicit
		EditWidgetChooser(
				GPlatesQtWidgets::EditWidgetGroupBox &edit_widget_group_box):
			d_edit_widget_group_box_ptr(&edit_widget_group_box)
		{  }

		explicit
		EditWidgetChooser(
				GPlatesQtWidgets::EditWidgetGroupBox &edit_widget_group_box,
				const GPlatesModel::PropertyName &property_name_to_allow):
			d_edit_widget_group_box_ptr(&edit_widget_group_box)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}

		virtual
		~EditWidgetChooser() {  }

		void
		add_property_name_to_allow(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}
		
		virtual
		void
		visit_feature_handle(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_inline_property_container(
				GPlatesModel::InlinePropertyContainer &inline_property_container);


		virtual
		void
		visit_enumeration(
				GPlatesPropertyValues::Enumeration &enumeration);

		virtual
		void
		visit_gml_time_instant(
				GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		virtual
		void
		visit_gml_time_period(
				GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);
		
		virtual
		void
		visit_gpml_polarity_chron_id(
				GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id);

		virtual
		void
		visit_gpml_measure(
				GPlatesPropertyValues::GpmlMeasure &gpml_measure);

		virtual
		void
		visit_gpml_old_plates_header(
				GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

		virtual
		void
		visit_xs_boolean(
				GPlatesPropertyValues::XsBoolean &xs_boolean);

		virtual
		void
		visit_xs_double(
				GPlatesPropertyValues::XsDouble &xs_double);

		virtual
		void
		visit_xs_integer(
				GPlatesPropertyValues::XsInteger& xs_integer);

		virtual
		void
		visit_xs_string(
				GPlatesPropertyValues::XsString &xs_string);


	private:
		GPlatesQtWidgets::EditWidgetGroupBox *d_edit_widget_group_box_ptr;
		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
				
	};

}

#endif  // GPLATES_QTWIDGETS_EDITWIDGETCHOOSER_H
