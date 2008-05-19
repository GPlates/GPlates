/* $Id: ToQvariantConverter.h 2828 2008-04-24 07:49:42Z jclark $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-04-24 09:49:42 +0200 (to, 24 apr 2008) $
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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

#ifndef GPLATES_FEATUREVISITORS_SHAPEFILEATTRIBUTEFINDER_H
#define GPLATES_FEATUREVISITORS_SHAPEFILEATTRIBTUEFINDER_H

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
	class ShapefileAttributeFinder:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		typedef std::vector<QVariant> qvariant_container_type;
		typedef qvariant_container_type::const_iterator qvariant_container_const_iterator;


		explicit
		ShapefileAttributeFinder(
				const QString attribute_name):
			d_attribute_name(attribute_name)
		{
		}

		virtual
		~ShapefileAttributeFinder() {  }
		
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
		QString d_attribute_name;
		qvariant_container_type d_found_qvariants;
		

	};

}

#endif  // GPLATES_FEATUREVISITORS_SHAPEFILEATTRIBUTEFINDER_H

