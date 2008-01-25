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

#ifndef GPLATES_FEATUREVISITORS_GEOMETRYFINDER_H
#define GPLATES_FEATUREVISITORS_GEOMETRYFINDER_H

#include <vector>
#include "model/ConstFeatureVisitor.h"
#include "model/PropertyName.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlConstantValue.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * This const feature visitor finds all geometry contained within the feature.
	 * It currently handles the following property-values:
	 * GmlLineString, GmlOrientableCurve (Assuming a GmlLineString is used as the base),
	 * and GmlPoint.
	 */
	class GeometryFinder:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		typedef std::vector<GPlatesPropertyValues::GmlLineString::non_null_ptr_to_const_type>
				line_string_container_type;
		typedef line_string_container_type::const_iterator line_string_container_const_iterator;

		typedef std::vector<GPlatesPropertyValues::GmlPoint::non_null_ptr_to_const_type>
				point_container_type;
		typedef point_container_type::const_iterator point_container_const_iterator;

		// FIXME: Supply the current reconstruction time to allow for time-dependent properties.
		GeometryFinder()
		{  }

		explicit
		GeometryFinder(
				const GPlatesModel::PropertyName &property_name_to_allow)
		{
			d_property_names_to_allow.push_back(property_name_to_allow);
		}

		virtual
		~GeometryFinder() {  }
		
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
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);
                                                                                

		line_string_container_const_iterator
		found_line_strings_begin() const
		{
			return d_found_line_strings.begin();
		}

		line_string_container_const_iterator
		found_line_strings_end() const
		{
			return d_found_line_strings.end();
		}

		point_container_const_iterator
		found_points_begin() const
		{
			return d_found_points.begin();
		}

		point_container_const_iterator
		found_points_end() const
		{
			return d_found_points.end();
		}
		
		bool
		has_found_geometry() const
		{
			if (found_line_strings_begin() != found_line_strings_end() ||
					found_points_begin() != found_points_end())
			{
				return true;
			}
			return false;
		}

		void
		clear_found_geometry()
		{
			d_found_line_strings.clear();
			d_found_points.clear();
		}
		
		/**
		 * If all you are interested in is finding the first point of some geometry
		 * of the feature (for example, to display to a user to differentiate two different
		 * geometries), you can call this function.
		 * 
		 * It returns the first point found in one of the geometric structural types of the
		 * feature. If multiple geometries are found, the first gml:Point gets priority,
		 * followed by the first point of the first gml:LineString.
		 */
		const GPlatesMaths::PointOnSphere *
		get_first_point();
		

	private:
		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
		line_string_container_type d_found_line_strings;
		point_container_type d_found_points;
	};
}

#endif  // GPLATES_FEATUREVISITORS_GEOMETRYFINDER_H
