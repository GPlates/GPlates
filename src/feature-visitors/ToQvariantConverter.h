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

#ifndef GPLATES_FEATUREVISITORS_TOQVARIANTCONVERTER_H
#define GPLATES_FEATUREVISITORS_TOQVARIANTCONVERTER_H

#include <vector>
#include <QVariant>
#include <Qt>
#include "model/ConstFeatureVisitor.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * The ToQvariantConverter feature-visitor is used to locate specific property values
	 * within a Feature and convert them to QVariants, if possible. It is used by the
	 * FeaturePropertyTableModel Qt model.
	 */
	class ToQvariantConverter:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		typedef std::vector<QVariant> qvariant_container_type;
		typedef qvariant_container_type::const_iterator qvariant_container_const_iterator;

		// FIXME:  We should also pass the current reconstruction time, so we can correctly
		// handle time-dependent property values.
		explicit
		ToQvariantConverter():
			d_role(Qt::DisplayRole)
		{  }

		explicit
		ToQvariantConverter(
				const GPlatesModel::PropertyName &property_name_to_allow):
			d_role(Qt::DisplayRole)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}

		virtual
		~ToQvariantConverter() {  }

		void
		add_property_name_to_allow(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}
		
		/**
		 * The ToQvariantConverter defaults to Qt::DisplayRole, for returning QVariants
		 * suitable for display purposes (e.g. formatted strings or simple numbers).
		 * This member allows you to change the role that is considered when constructing
		 * QVariants - specifically, to Qt::EditRole, the role that is used to transfer
		 * data to a Delegate in Qt's Model/View structure.
		 * 
		 * Note that this will only make a difference for a few property value types:
		 *
		 * GmlTimePeriod: Returns a formatted QString in Qt::DisplayRole, returns a 
		 * 	QList<QVariant> containing two QVariants in Qt::EditRole.
		 *
		 * The values used by Qt are of the enum Qt::ItemDataRole, but Qt uses
		 * int in all it's methods, presumably to allow the definition of custom
		 * values.
		 */
		void
		set_desired_role(
				int role)
		{
			d_role = role;
		}

		virtual
		void
		visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_inline_property_container(
				const GPlatesModel::InlinePropertyContainer &inline_property_container);



		virtual
		void
		visit_enumeration(
				const GPlatesPropertyValues::Enumeration &enumeration);

		virtual
		void
		visit_gml_time_instant(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		virtual
		void
		visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		virtual
		void
		visit_gpml_polarity_chron_id(
				const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id);

		virtual
		void
		visit_gpml_measure(
				const GPlatesPropertyValues::GpmlMeasure &gpml_measure);

		virtual
		void
		visit_gpml_old_plates_header(
				const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);

		virtual
		void
		visit_xs_boolean(
				const GPlatesPropertyValues::XsBoolean &xs_boolean);

		virtual
		void
		visit_xs_double(
				const GPlatesPropertyValues::XsDouble &xs_double);

		virtual
		void
		visit_xs_integer(
				const GPlatesPropertyValues::XsInteger& xs_integer);

		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string);


		qvariant_container_const_iterator
		found_qvariants_begin() const
		{
			return d_found_qvariants.begin();
		}

		qvariant_container_const_iterator
		found_qvariants_end() const
		{
			return d_found_qvariants.end();
		}

		void
		clear_found_qvariants()
		{
			d_found_qvariants.clear();
		}

	private:
		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
		qvariant_container_type d_found_qvariants;
		
		/**
		 * The role that is to be used for the returned QVariant.
		 * This will default to Qt::DisplayRole, and can be set to Qt::EditRole
		 * when the model must present data to an editing widget.
		 * Most property values visitor members will not care about this, but for
		 * a few complex property values we will need to vary what they return
		 * based on the role.
		 *
		 * The values used by Qt are of the enum Qt::ItemDataRole, but Qt uses
		 * int in all it's methods, presumably to allow the definition of custom
		 * values.
		 */
		int d_role;

	};

}

#endif  // GPLATES_FEATUREVISITORS_TOQVARIANTCONVERTER_H
