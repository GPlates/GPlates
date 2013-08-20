/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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
#include "maths/PolygonOnSphere.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
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
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlPolygon>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlPolygon> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlPolygon>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlPolygon> non_null_ptr_to_const_type;


		/**
		 * A convenience typedef for the ring representation used internally.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolygonOnSphere> ring_type;
		
		/**
		 * Typedef for a sequence of rings -- used to hold the interior ring definition.
		 */
		typedef std::vector<ring_type> ring_sequence_type;


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
			return create(exterior_ring, ring_sequence_type());
		}

		/**
		 * Create a GmlPolygon instance which contains an exterior ring and some number of
		 * interior rings.
		 *
		 * This function will store a copy of @a exterior_ring (which is a pointer) and
		 * each ring in @a interior_ring_collection (which are pointers).
		 */
		static
		const non_null_ptr_type
		create(
				const ring_type &exterior_ring,
				const ring_sequence_type &interior_rings)
		{
			// Because PolygonOnSphere can only ever be handled via a
			// non_null_ptr_to_const_type, there is no way a PolylineOnSphere instance
			// can be changed.  Hence, it is safe to store a pointer to the instance
			// which was passed into this 'create' function.
			return non_null_ptr_type(new GmlPolygon(exterior_ring, interior_rings));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlPolygon>(clone_impl());
		}

		/**
		 * Access the GPlatesMaths::PolygonOnSphere which encodes the exterior geometry of this instance.
		 */
		const ring_type
		get_exterior() const
		{
			return get_current_revision<Revision>().exterior;
		}

		/**
		 * Set the exterior polygon within this instance to @a exterior.
		 */
		void
		set_exterior(
				const ring_type &exterior);

		/**
		 * Return the sequence of interior rings of this GmlPolygon.
		 *
		 * To modify any interior rings:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a set_interiors to set them.
		 */
		const ring_sequence_type &
		get_interiors() const
		{
			return get_current_revision<Revision>().interiors;
		}

		/**
		 * Set the sequence of interior rings within this instance to @a r.
		 */
		void
		set_interiors(
				const ring_sequence_type &interiors);


		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("Polygon");
			return STRUCTURAL_TYPE;
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

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		GmlPolygon(
				const ring_type &exterior_ring,
				const ring_sequence_type &interior_rings) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(exterior_ring, interior_rings)))
		{  }

		//! Constructor used when cloning.
		GmlPolygon(
				const GmlPolygon &other_,
				boost::optional<GPlatesModel::PropertyValueRevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(other_.get_current_revision<Revision>(), context_)))
		{  }

		virtual
		const PropertyValue::non_null_ptr_type
		clone_impl(
				boost::optional<GPlatesModel::PropertyValueRevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GmlPolygon(*this, context));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValueRevision
		{
			Revision(
					const ring_type &exterior_ring_,
					const ring_sequence_type &interior_rings_) :
				exterior(exterior_ring_),
				interiors(interior_rings_)
			{  }

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::PropertyValueRevisionContext &> context_) :
				PropertyValueRevision(context_),
				// Note there is no need to distinguish between shallow and deep copying because
				// PolygonOnSphere is immutable and hence there is never a need to deep copy it...
				exterior(other_.exterior),
				interiors(other_.interiors)
			{  }

			virtual
			PropertyValueRevision::non_null_ptr_type
			clone_revision(
					boost::optional<GPlatesModel::PropertyValueRevisionContext &> context) const
			{
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const PropertyValueRevision &other) const;

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
			ring_type exterior;
			
			/**
			 * This is the sequence of GPlatesMaths::PolygonOnSphere which defines the interior ring(s)
			 * of this GmlPolygon.  A GmlPolygon contains exactly one exterior ring, and zero or more
			 * interior rings.
			 */
			ring_sequence_type interiors;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLPOLYGON_H
