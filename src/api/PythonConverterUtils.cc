/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include "PythonConverterUtils.h"

#include "global/python.h"

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/FeatureVisitor.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * Visits a @a GeometryOnSphere and converts from its derived type to a python object.
	 */
	class GetGeometryOnSphereAsDerivedTypeVisitor:
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:

		/**
		 * The derived geometry-on-sphere retrieved after visiting a @a GeometryOnSphere.
		 */
		bp::object
		get_geometry_on_sphere_as_derived_type()
		{
			return d_geometry_on_sphere;
		}

		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			// Use to-python converter registered for derived geometry-on-sphere's 'non_null_ptr_to_const_type'.
			d_geometry_on_sphere = bp::object(multi_point_on_sphere);
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			// Use to-python converter registered for derived geometry-on-sphere's 'non_null_ptr_to_const_type'.
			d_geometry_on_sphere = bp::object(point_on_sphere);
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			// Use to-python converter registered for derived geometry-on-sphere's 'non_null_ptr_to_const_type'.
			d_geometry_on_sphere = bp::object(polygon_on_sphere);
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			// Use to-python converter registered for derived geometry-on-sphere's 'non_null_ptr_to_const_type'.
			d_geometry_on_sphere = bp::object(polyline_on_sphere);
		}

	private:
		bp::object d_geometry_on_sphere;
	};


	namespace
	{
		/**
		 * This is the same as @a to_python_ConstToNonConst<T>
		 * except it also converts an instance of GeometryOnSphere::non_null_ptr_to_const_type to
		 * its derived GeometryOnSphere type first - this ensures that the python object contains
		 * a pointer to the 'derived' type.
		 */
		struct to_python_ConstToNonConstGeometryOnSphere :
				private boost::noncopyable
		{
			struct Conversion
			{
				static
				PyObject *
				convert(
						const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry_on_sphere)
				{
					namespace bp = boost::python;

					GetGeometryOnSphereAsDerivedTypeVisitor visitor;
					geometry_on_sphere->accept_visitor(visitor);

					// Note that this returns a 'DerivedGeometryOnSphere::non_null_ptr_to_const_type'
					// and not a 'GPlatesUtils::non_null_intrusive_ptr<DerivedGeometryOnSphere>'
					// (which is the HeldType of the 'bp::class_' wrapper of the DerivedGeometryOnSphere type)
					// so a conversion from the former to the latter via the to-python converter registered by
					// register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion<DerivedGeometryOnSphere>()
					// is still required to complete the conversion to python.
					return bp::incref(visitor.get_geometry_on_sphere_as_derived_type().ptr());
				};
			};
		};
	}
}


void
GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_geometry_on_sphere_conversion()
{
	// To python conversion.
	bp::to_python_converter<
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type,
			to_python_ConstToNonConstGeometryOnSphere::Conversion>();
}

#endif // GPLATES_NO_PYTHON
