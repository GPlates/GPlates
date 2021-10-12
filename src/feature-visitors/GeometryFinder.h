/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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
#include "maths/GeometryOnSphere.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * This const feature visitor finds all geometry contained within the feature.
	 *
	 * It currently handles the following property-values:
	 *  -# GmlLineString
	 *  -# GmlMultiPoint
	 *  -# GmlOrientableCurve (assuming a GmlLineString is used as the base)
	 *  -# GmlPoint
	 *  -# GmlPolygon (although the differentiation between the interior and exterior rings is
	 * lost)
	 */
	class GeometryFinder:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_elem_type;
		typedef std::vector<geometry_elem_type> geometry_container_type;
		typedef geometry_container_type::const_iterator geometry_container_const_iterator;

		// FIXME: Supply the current reconstruction time to allow for time-dependent
		// properties.
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

		geometry_container_const_iterator
		found_geometries_begin() const
		{
			return d_found_geometries.begin();
		}

		geometry_container_const_iterator
		found_geometries_end() const
		{
			return d_found_geometries.end();
		}

		bool
		has_found_geometries() const
		{
			return (found_geometries_begin() != found_geometries_end());
		}

		/**
		 * Access the first element in the container of found geometries.
		 *
		 * Note that this function assumes that the client code has already ensured that
		 * the container is not empty.  If the container @em is empty, a
		 * RetrievalFromEmptyContainerException will be thrown.
		 */
		geometry_elem_type
		first_geometry_found() const;

		void
		clear_found_geometries()
		{
			d_found_geometries.clear();
		}

	protected:

		virtual
		bool
		initialise_pre_property_values(
				const GPlatesModel::TopLevelPropertyInline &top_level_property_inline);

		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

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
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

	private:
		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
		geometry_container_type d_found_geometries;
	};
}

#endif  // GPLATES_FEATUREVISITORS_GEOMETRYFINDER_H
