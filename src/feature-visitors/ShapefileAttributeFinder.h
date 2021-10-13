/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
 * Copyright (C) 2010 The University of Sydney, Australia
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
#define GPLATES_FEATUREVISITORS_SHAPEFILEATTRIBUTEFINDER_H

#include <vector>
#include <QVariant>
#include <Qt>
#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"

namespace GPlatesPropertyValues
{
	class GpmlKeyValueDictionaryElement;
}

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

	protected:

		virtual
		bool
		initialise_pre_property_values(
				const GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

#if 0
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
#endif
		virtual
		void
		visit_gpml_key_value_dictionary(
				const GPlatesPropertyValues::GpmlKeyValueDictionary &dictionary);
#if 0
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
#endif
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

	private:

		void
		find_shapefile_attribute_in_element(
				const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element);

		QString d_attribute_name;
		qvariant_container_type d_found_qvariants;
		

	};

}

#endif  // GPLATES_FEATUREVISITORS_SHAPEFILEATTRIBUTEFINDER_H

