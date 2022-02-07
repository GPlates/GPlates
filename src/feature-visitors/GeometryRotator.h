/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_FEATURE_VISITORS_GEOMETRYROTATOR_H
#define GPLATES_FEATURE_VISITORS_GEOMETRYROTATOR_H

#include "maths/FiniteRotation.h"

#include "model/FeatureVisitor.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * Visits all geometry properties in a feature, rotates them and
	 * replaces the original geometry with the rotated versions.
	 */
	class GeometryRotator :
			public GPlatesModel::FeatureVisitor
	{
	public:
		explicit
		GeometryRotator(
				const GPlatesMaths::FiniteRotation &finite_rotation):
			d_finite_rotation(finite_rotation)
		{  }
		
	protected:
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
	   GPlatesMaths::FiniteRotation d_finite_rotation;
	};
}

#endif // GPLATES_FEATURE_VISITORS_GEOMETRYROTATOR_H
