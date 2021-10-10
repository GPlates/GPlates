/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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
 
#ifndef GPLATES_GUI_GEOMETRICPROPERTYVALUECONSTRUCTOR_H
#define GPLATES_GUI_GEOMETRICPROPERTYVALUECONSTRUCTOR_H

#include <boost/optional.hpp>

#include "model/types.h"
#include "model/PropertyValue.h"
#include "model/ReconstructionTree.h"
#include "maths/GeometryOnSphere.h"
#include "maths/ConstGeometryOnSphereVisitor.h"


namespace GPlatesGui
{
	/**
	 * ConstGeometryVisitor to wrap a GeometryOnSphere with the appropriate
	 * PropertyValue (e.g. GmlLineString for PolylineOnSphere)
	 *
	 * FIXME: This should ideally be in the geometry-visitors/ directory.
	 *
	 * FIXME 2: We should pass the flag indicating whether the resulting
	 * geometry PropertyValue should be GpmlConstantValue-wrapped.
	 *
	 * FIXME 3: The ReconstructionTree argument should be optional, for
	 * other classes wishing to use this visitor which pass only present-day
	 * coordinates.
	 *
	 * FIXME 4: And maybe, just maybe, specify if we should 'construct' a
	 * PropertyValue or merely 'update' an existing PropertyValue using a
	 * setter. Possibly, that could be a different visitor, but many of the
	 * other steps would be shared...
	 * 
	 * Alternative FIXME: Or perhaps a series of geometry-visitors and
	 * property-visitors would be pleasing? One to reverse-reconstruct a
	 * GeometryOnSphere, one to create a PropertyValue, an alternative one
	 * to use setters on an existing PropertyValue. One more to wrap a
	 * PropertyValue in a GpmlConstantValue wrapper.
	 * But that can wait until we have a geometry-visitors dir that works
	 * in the build system.
	 */
	class GeometricPropertyValueConstructor : 
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		GeometricPropertyValueConstructor():
			GPlatesMaths::ConstGeometryOnSphereVisitor(),
			d_property_value_opt(boost::none),
			d_plate_id_opt(boost::none),
			d_wrap_with_gpml_constant_value(true),
			d_recon_tree_ptr(NULL)
		{  }
		
		virtual
		~GeometricPropertyValueConstructor()
		{  }
		
		/**
		 * Call this to visit a GeometryOnSphere and (attempt to)
		 *  a) reverse-reconstruct the geometry to appropriate present-day coordinates,
		 *  b) create a suitable geometric PropertyValue out of it.
		 *  c) wrap that property value in a GpmlConstantValue.
		 *
		 * May return boost::none.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		convert(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr,
				boost::optional<GPlatesModel::ReconstructionTree &> reconstruction_tree_opt_ref,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_opt,
				bool wrap_with_gpml_constant_value);

		
		// Please keep these geometries ordered alphabetically.
		
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere);
		
		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere);

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere);
		
		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere);
	
	private:
		// This operator should never be defined, because we don't want to allow copy-assignment.
		GeometricPropertyValueConstructor &
		operator=(
				const GeometricPropertyValueConstructor &);
		
		/**
		 * The return value of convert(), assigned during the visit.
		 */
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> d_property_value_opt;
		
		/**
		 * The plate id parameter for reverse reconstructing the geometry.
		 * Set from convert().
		 * If it is set to boost::none we will assume that, for whatever reason, the caller
		 * does not want us to do reverse reconstructions today.
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id_opt;
		
		/**
		 * The parameter indicating the caller wants the final PropertyValue to be
		 * wrapped up in a suitable GpmlConstantValue.
		 * Set from convert().
		 */
		bool d_wrap_with_gpml_constant_value;
		
		/**
		 * This is the reconstruction tree, used to perform the reverse reconstruction
		 * and obtain present-day geometry appropriate for the given plate id.
		 *
		 * May be NULL but only because I'm in a hurry to port this code over to the
		 * EditGeometryWidget which will only deal in present day geometry for now!!
		 * See class FIXMEs!
		 */
		GPlatesModel::ReconstructionTree *d_recon_tree_ptr;
	};
}


#endif // GPLATES_GUI_GEOMETRICPROPERTYVALUECONSTRUCTOR_H
