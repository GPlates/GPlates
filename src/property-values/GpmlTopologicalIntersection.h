/* $Id: GpmlTopologicalIntersection.h 1564 2007-10-02 06:40:28Z jboyden $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2007-10-01 23:40:28 -0700 (Mon, 01 Oct 2007) $
 * 
 * Copyright (C) 2008, 2009, 2010 California Institute of Technology
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

#ifndef GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALINTERSECTION_H
#define GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALINTERSECTION_H

#include "GpmlPropertyDelegate.h"
#include "GmlPoint.h"

#include "model/PropertyValue.h"


namespace GPlatesPropertyValues {

	// Since all the members of this class are of type boost::intrusive_ptr or
	// TemplateTypeParameterType (which wraps an StringSet::SharedIterator instance which
	// points to a pre-allocated node in a StringSet), none of the construction,
	// copy-construction or copy-assignment operations for this class should throw.
	class GpmlTopologicalIntersection {

	public:

		GpmlTopologicalIntersection(
				GpmlPropertyDelegate::non_null_ptr_type intersection_geometry_,
				GmlPoint::non_null_ptr_type reference_point_,
				GpmlPropertyDelegate::non_null_ptr_type reference_point_plate_id_):
			d_intersection_geometry(intersection_geometry_),
			d_reference_point(reference_point_),
			d_reference_point_plate_id(reference_point_plate_id_)
		{  }

		GpmlTopologicalIntersection(
				const GpmlTopologicalIntersection &other) :
			d_intersection_geometry(other.d_intersection_geometry),
			d_reference_point(other.d_reference_point),
			d_reference_point_plate_id(other.d_reference_point_plate_id)
		{  }

		virtual
		~GpmlTopologicalIntersection() { }

		const GpmlTopologicalIntersection
		deep_clone() const;

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const {
			visitor.visit_gpml_topological_intersection(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) {
			visitor.visit_gpml_topological_intersection(*this);
		}


		// access to d_intersection_geometry
		GpmlPropertyDelegate::non_null_ptr_type
		intersection_geometry() const {
			return d_intersection_geometry;
		}

		void
		set_d_intersection_geometry(
				GpmlPropertyDelegate::non_null_ptr_type intersection_geom) {
			d_intersection_geometry = intersection_geom;
		} 


		// access to reference point
		GmlPoint::non_null_ptr_type
		reference_point() const {
			return d_reference_point;
		}

		void
		set_reference_point(
				GmlPoint::non_null_ptr_type point) {
			d_reference_point = point;
		}


		// access to d_reference_point_plate_id
		GpmlPropertyDelegate::non_null_ptr_type
		reference_point_plate_id() const {
			return d_reference_point_plate_id;
		}

		void
		set_reference_point_plate_id(
				GpmlPropertyDelegate::non_null_ptr_type ref_point_plate_id) {
			d_reference_point_plate_id = ref_point_plate_id;
		} 

	private:

		GpmlPropertyDelegate::non_null_ptr_type d_intersection_geometry;
		GmlPoint::non_null_ptr_type d_reference_point;
		GpmlPropertyDelegate::non_null_ptr_type d_reference_point_plate_id;
	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTOPOLOGICALINTERSECTION_H
