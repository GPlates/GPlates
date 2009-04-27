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

#ifndef GPLATES_PROPERTYVALUES_GMLPOLYGON_H
#define GPLATES_PROPERTYVALUES_GMLPOLYGON_H

#include <vector>

#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"
#include "maths/PolygonOnSphere.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlPolygon, visit_gml_polygon)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:Polygon".
	 */
	class GmlPolygon:
			public GPlatesModel::PropertyValue
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlPolygon,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlPolygon,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GmlPolygon,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlPolygon,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for the ring representation used internally.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolygonOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler>
				ring_type;
		
		/**
		 * Typedef for a sequence of rings -- used to hold the interior ring definition.
		 */
		typedef std::vector<ring_type> ring_sequence_type;
		typedef ring_sequence_type::const_iterator ring_const_iterator;

		virtual
		~GmlPolygon()
		{  }

		/**
		 * Create a GmlPolygon instance which contains an exterior ring only (no interior
		 * rings).
		 *
		 * This function will store a copy of @a exterior_ring (which is a pointer).
		 */
		static
		const non_null_ptr_type
		create(
				const ring_type &exterior_ring)
		{
			// Because PolygonOnSphere can only ever be handled via a
			// non_null_ptr_to_const_type, there is no way a PolylineOnSphere instance
			// can be changed.  Hence, it is safe to store a pointer to the instance
			// which was passed into this 'create' function.
			GmlPolygon::non_null_ptr_type polygon_ptr(
					new GmlPolygon(exterior_ring),
					GPlatesUtils::NullIntrusivePointerHandler());
			return polygon_ptr;
		}

		/**
		 * Create a GmlPolygon instance which contains an exterior ring and some number of
		 * interior rings.
		 *
		 * It is assumed that @a interior_ring_collection is an STL-like container of
		 * ring_type.  Specifically, the template parameter-type @a C should provide the
		 * following member functions:
		 *  -# begin, which returns a "begin" forward-iterator which dereferences to an
		 * instance of ring_type.
		 *  -# end, which returns an "end" forward-iterator which dereferences to an
		 * instance of ring_type.
		 *  -# size, which returns the number of elements in the container.
		 *
		 * This function will store a copy of @a exterior_ring (which is a pointer) and
		 * each ring in @a interior_ring_collection (which are pointers).
		 */
		template<typename C>
		static
		const non_null_ptr_type
		create(
				const ring_type &exterior_ring,
				const C &interior_ring_collection)
		{
			// Because PolygonOnSphere can only ever be handled via a
			// non_null_ptr_to_const_type, there is no way a PolylineOnSphere instance
			// can be changed.  Hence, it is safe to store a pointer to the instance
			// which was passed into this 'create' function.
			GmlPolygon::non_null_ptr_type polygon_ptr(
					new GmlPolygon(exterior_ring, interior_ring_collection),
					GPlatesUtils::NullIntrusivePointerHandler());
			return polygon_ptr;
		}

		/**
		 * Create a duplicate of this PropertyValue instance.
		 */
		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const
		{
			GPlatesModel::PropertyValue::non_null_ptr_type dup(
					new GmlPolygon(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		/**
		 * Access the GPlatesMaths::PolygonOnSphere which encodes the exterior geometry of
		 * this instance.
		 *
		 * Note that there is no accessor provided which returns a boost::intrusive_ptr to
		 * a non-const GPlatesMaths::PolygonOnSphere.  The GPlatesMaths::PolygonOnSphere
		 * within this instance should not be modified directly; to alter the
		 * GPlatesMaths::PolygonOnSphere within this instance, set a new value using the
		 * function @a set_exterior below.
		 */
		const ring_type
		exterior() const
		{
			return d_exterior;
		}

		/**
		 * Set the exterior polygon within this instance to @a r.
		 *
		 * FIXME: when we have undo/redo, this act should cause
		 * a new revision to be propagated up to the Feature which
		 * contains this PropertyValue.
		 */
		void
		set_exterior(
				const ring_type &r)
		{
			d_exterior = r;
		}
		
		/**
		 * Return the "begin" const iterator to iterate over the interior rings of this
		 * GmlPolygon.
		 */
		ring_const_iterator
		interiors_begin() const
		{
			return d_interiors.begin();
		}

		/**
		 * Return the "end" const iterator for iterating over the interior rings of this
		 * GmlPolygon.
		 */
		ring_const_iterator
		interiors_end() const
		{
			return d_interiors.end();
		}
		
		/**
		 * Add an interior polygon @a r to the list of interiors of this instance.
		 *
		 * FIXME: when we have undo/redo, this act should cause
		 * a new revision to be propagated up to the Feature which
		 * contains this PropertyValue.
		 */
		void
		add_interior(
				const ring_type &r)
		{
			d_interiors.push_back(r);
		}
		
		
		/**
		 * Removes all interior rings from this GmlPolygon.
		 *
		 * FIXME: when we have undo/redo, this act should cause
		 * a new revision to be propagated up to the Feature which
		 * contains this PropertyValue.
		 */
		void
		clear_interiors()
		{
			d_interiors.clear();
		}
		

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_gml_polygon(*this);
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
				GPlatesModel::FeatureVisitor &visitor)
		{
			visitor.visit_gml_polygon(*this);
		}

	protected:

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This constructor creates a GmlPolygon with a single exterior and no interior
		 * rings.
		 */
		GmlPolygon(
				const ring_type &exterior_ring):
			PropertyValue(),
			d_exterior(exterior_ring)
		{  }

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		GmlPolygon(
				const ring_type &exterior_ring,
				const ring_sequence_type &interior_rings):
			PropertyValue(),
			d_exterior(exterior_ring),
			d_interiors(interior_rings)
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlPolygon(
				const GmlPolygon &other):
			PropertyValue(),
			d_exterior(other.d_exterior),
			d_interiors(other.d_interiors)
		{  }

	private:

		/**
		 * This is the GPlatesMaths::PolygonOnSphere which defines the exterior ring of this
		 * GmlPolygon. A GmlPolygon contains exactly one exterior ring, and zero or more interior
		 * rings.
		 *
		 * Note that this conflicts with the ESRI Shapefile definition which allows for multiple
		 * exterior rings.
		 *
		 * Also note that the GPlates model creates polygons by implicitly joining the first and last
		 * vertex fed to it; supplying three points creates a triangle, four points creates a
		 * quadrilateral. In contrast, the ESRI Shapefile spec and GML Polygons are supposed to be
		 * read from disk and written to disk with the first and last vertices coincident - four
		 * points creates a triangle, and three points are invalid. This is especially important
		 * to keep in mind as GPlates cannot create a GreatCircleArc between coincident points.
		 */
		ring_type d_exterior;
		
		/**
		 * This is the sequence of GPlatesMaths::PolygonOnSphere which defines the interior ring(s)
		 * of this GmlPolygon.  A GmlPolygon contains exactly one exterior ring, and zero or more
		 * interior rings.
		 */
		ring_sequence_type d_interiors;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlPolygon &
		operator=(
				const GmlPolygon &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLPOLYGON_H
