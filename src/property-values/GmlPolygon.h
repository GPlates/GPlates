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
	class GmlPolygon :
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
		 * A convenience typedef for the internal polygon representation.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolygonOnSphere> internal_polygon_type;


		virtual
		~GmlPolygon()
		{  }

		/**
		 * Create a GmlPolygon instance which contains a copy of @a polygon_.
		 */
		static
		const non_null_ptr_type
		create(
				const internal_polygon_type &polygon_);

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GmlPolygon(*this));
		}

		const non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()


		/**
		 * Access the GPlatesMaths::PolygonOnSphere which encodes the geometry of this instance.
		 *
		 * Note that there is no accessor provided which returns a boost::intrusive_ptr to
		 * a non-const GPlatesMaths::PolygonOnSphere.  The GPlatesMaths::PolygonOnSphere
		 * within this instance should not be modified directly; to alter the
		 * GPlatesMaths::PolygonOnSphere within this instance, set a new value using the
		 * function @a set_polygon below.
		 */
		const internal_polygon_type
		polygon() const
		{
			return d_polygon;
		}

		/**
		 * Set the polygon within this instance to @a p.
		 */
		void
		set_polygon(
				const internal_polygon_type &p)
		{
			d_polygon = p;
			update_instance_id();
		}
		

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
				const internal_polygon_type &polygon_):
			PropertyValue(),
			d_polygon(polygon_)
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlPolygon(
				const GmlPolygon &other):
			PropertyValue(other), /* share instance id */
			d_polygon(other.d_polygon)
		{  }

	private:

		internal_polygon_type d_polygon;

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
