/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_FEATUREVISITORS_PLATEIDFINDER_H
#define GPLATES_FEATUREVISITORS_PLATEIDFINDER_H

#include <vector>
#include "model/ConstFeatureVisitor.h"
#include "model/PropertyName.h"
#include "model/types.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * This const feature visitor finds all plate IDs contained within the feature.
	 */
	class PlateIdFinder:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		typedef std::vector<GPlatesModel::integer_plate_id_type> plate_id_container_type;
		typedef plate_id_container_type::const_iterator plate_id_container_const_iterator;


		// FIXME:  We should also pass the current reconstruction time, so we can correctly
		// handle time-dependent property values.
		explicit
		PlateIdFinder(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}

		virtual
		~PlateIdFinder() {  }

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
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		plate_id_container_const_iterator
		found_plate_ids_begin() const
		{
			return d_found_plate_ids.begin();
		}

		plate_id_container_const_iterator
		found_plate_ids_end() const
		{
			return d_found_plate_ids.end();
		}

		void
		clear_found_plate_ids()
		{
			d_found_plate_ids.clear();
		}

	private:
		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
		plate_id_container_type d_found_plate_ids;
	};
}

#endif  // GPLATES_FEATUREVISITORS_PLATEIDFINDER_H
