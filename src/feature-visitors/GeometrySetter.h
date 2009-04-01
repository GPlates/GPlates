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

#ifndef GPLATES_FEATUREVISITORS_GEOMETRYSETTER_H
#define GPLATES_FEATUREVISITORS_GEOMETRYSETTER_H

#include <vector>

#include "model/FeatureVisitor.h"
#include "model/PropertyName.h"
#include "maths/GeometryOnSphere.h"


namespace GPlatesModel
{
	class PropertyValue;
	class PropertyContainer;
}

namespace GPlatesFeatureVisitors
{
	/**
	 * This feature visitor takes a GPlatesMaths::GeometryOnSphere, and assigns it 
	 * to a GPlatesModel::PropertyValue.
	 *
	 * It currently handles the following property-values:
	 *  -# GmlLineString
	 *  -# GmlMultiPoint
	 *  -# GmlOrientableCurve (assuming a GmlLineString is used as the base)
	 *  -# GmlPoint
	 *  -# GmlPolygon (although the differentiation between the interior and exterior rings is
	 * lost)
	 *
	 * NOTE: The interface is deliberately limited to setting property values directly
	 * or in a property container. We disable setting geometry on a feature because
	 * it is not obvious which geometry property should be changed. It is up to the
	 * client to determine this before using this interface. Currently multiple geometries
	 * are supported as separate property values (and currently there is no support for
	 * multiple geometry properties in a single property container).
	 */
	class GeometrySetter:
			private GPlatesModel::FeatureVisitor
	{
	public:
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr_type;

		explicit
		GeometrySetter(
				geometry_ptr_type geometry_to_set):
			d_geometry_to_set(geometry_to_set)
		{  }

		virtual
		~GeometrySetter()
		{  }

		/**
		 * Sets the geometry contained in @a geometry_property to the geometry
		 * specified in the constructor.
		 */
		void
		set_geometry(
				GPlatesModel::PropertyValue *geometry_property);

		/**
		 * Sets the geometry contained in @a geometry_property_container to the geometry
		 * specified in the constructor.
		 */
		void
		set_geometry(
				GPlatesModel::PropertyContainer *geometry_property_container);
		
	private:
		virtual
		void
		visit_gml_line_string(
				GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		virtual
		void
		visit_gml_orientable_curve(
				GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_point(
				GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gml_polygon(
				GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

	private:
	   geometry_ptr_type d_geometry_to_set;
	};
}

#endif  // GPLATES_FEATUREVISITORS_GEOMETRYSETTER_H
