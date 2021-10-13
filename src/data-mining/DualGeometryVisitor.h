/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
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
#ifndef GPLATES_DATAMINING_DUALGEOMETRYVISITOR_H
#define GPLATES_DATAMINING_DUALGEOMETRYVISITOR_H

#include <boost/type_traits/is_void.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <boost/utility/enable_if.hpp>

#include "maths/ConstGeometryOnSphereVisitor.h"

namespace GPlatesDataMining
{
	using namespace GPlatesMaths;

	template< class Handler, class T> 
	class DualGeometryVisitor;

	template< class FirstGeoType, class SecondGeoType, class HandlerType>
	inline
	typename boost::enable_if<typename boost::is_void<SecondGeoType>::type>::type
	dispatch(
			const GeometryOnSphere& geo_1,
			const GeometryOnSphere& geo_2,
			HandlerType* handler)
	{
		DualGeometryVisitor<HandlerType, typename boost::remove_pointer<FirstGeoType>::type> 
			v(geo_1, geo_2, handler);
		geo_2.accept_visitor(v);
	}


	template<class FirstGeoType, class SecondGeoType, class HandlerType>
	inline
	typename boost::disable_if<typename boost::is_void<SecondGeoType>::type>::type
	dispatch(
			const GeometryOnSphere& geo_1,
			const GeometryOnSphere& geo_2,
			HandlerType* handler)
	{
		SecondGeoType* geometry_1 = dynamic_cast<SecondGeoType*>(&geo_1);
		FirstGeoType* geometry_2 = dynamic_cast<FirstGeoType*>(&geo_2);
		if(geometry_1 && geometry_2)
		{
			if(handler)
			{
				handler->execute(geometry_1, geometry_2);
			}
			else
			{
				qWarning() << "No valid handler in DualGeometryVisitor.";
			}
		}
		else
		{
			qWarning() << "Unexpected error happened in DualGeometryVisitor.";
		}
	}


	template< class Handler, class T = const void > 
	class DualGeometryVisitor :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		DualGeometryVisitor(
				const GPlatesMaths::GeometryOnSphere& geomtry_1,
				const GPlatesMaths::GeometryOnSphere& geomtry_2,
				Handler* handler) :
			d_geomtry_1(geomtry_1),
			d_geomtry_2(geomtry_2),
			d_handler(handler)
		{ }

		inline
		void
		apply()
		{
			d_geomtry_1.accept_visitor(*this);
		}
		 
		inline
		void
		visit_multi_point_on_sphere(
				MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{ 
			dispatch<const MultiPointOnSphere,T,Handler>(d_geomtry_1,d_geomtry_2,d_handler);
		}

		inline
		void
		visit_point_on_sphere(
				PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{ 
			dispatch<const PointOnSphere,T,Handler>(d_geomtry_1,d_geomtry_2,d_handler);
		}

		inline
		void
		visit_polygon_on_sphere(
				PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{ 
			dispatch<const PolygonOnSphere,T,Handler>(d_geomtry_1,d_geomtry_2,d_handler);
		}

		inline
		void
		visit_polyline_on_sphere(
				PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			dispatch<const PolylineOnSphere,T,Handler>(d_geomtry_1,d_geomtry_2,d_handler);
		}

	protected:

		const GPlatesMaths::GeometryOnSphere& d_geomtry_1;
		const GPlatesMaths::GeometryOnSphere& d_geomtry_2;
		Handler* d_handler;
	
	private:
		DualGeometryVisitor();
	};
}

#endif  // GPLATES_DATAMINING_DUALGEOMETRYVISITOR_H
