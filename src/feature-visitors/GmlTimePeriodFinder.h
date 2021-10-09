/* $Id $ */

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

#ifndef GPLATES_FEATUREVISITORS_GMLTIMEPERIODFINDER_H
#define GPLATES_FEATUREVISITORS_GMLTIMEPERIODFINDER_H

#include <vector>
#include "model/ConstFeatureVisitor.h"
#include "model/PropertyName.h"
#include "property-values/GmlTimePeriod.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * This const feature visitor finds all gml:TimePeriods contained within the feature.
	 * The most interesting gml:TimePeriod will probably be contained in the "gpml:validTime" property.
	 */
	class GmlTimePeriodFinder:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		typedef std::vector<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type>
				time_period_container_type;
		typedef time_period_container_type::const_iterator time_period_container_const_iterator;

		GmlTimePeriodFinder()
		{  }

		explicit
		GmlTimePeriodFinder(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}

		virtual
		~GmlTimePeriodFinder() {  }

		void
		add_property_name_to_allow(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
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
		visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		time_period_container_const_iterator
		found_time_periods_begin() const
		{
			return d_found_time_periods.begin();
		}

		time_period_container_const_iterator
		found_time_periods_end() const
		{
			return d_found_time_periods.end();
		}

		void
		clear_found_time_periods()
		{
			d_found_time_periods.clear();
		}

	private:
		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
		time_period_container_type d_found_time_periods;
	};
}

#endif  // GPLATES_FEATUREVISITORS_GMLTIMEPERIODFINDER_H
