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

#ifndef GPLATES_PROPERTYVALUES_GMLMULTIPOINT_H
#define GPLATES_PROPERTYVALUES_GMLMULTIPOINT_H

#include <vector>

#include "GmlPoint.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "model/PropertyValue.h"

#include "maths/PointOnSphere.h"
#include "maths/MultiPointOnSphere.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlMultiPoint, visit_gml_multi_point)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:MultiPoint".
	 */
	class GmlMultiPoint:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlMultiPoint>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlMultiPoint> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GmlMultiPoint>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlMultiPoint> non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for the internal multipoint representation.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::MultiPointOnSphere> internal_multipoint_type;

		virtual
		~GmlMultiPoint()
		{  }

		/**
		 * Create a GmlMultiPoint instance which contains a copy of @a multipoint_.
		 *
		 * All points are presumed to have property gml:pos (as opposed to gml:coordinates).
		 */
		static
		const non_null_ptr_type
		create(
				const internal_multipoint_type &multipoint_);

		/**
		 * Create a GmlMultiPoint instance which contains a copy of @a multipoint_.
		 *
		 * The property with which each point was specified (gml:pos or gml:coordinates)
		 * is specified in @a gml_properties_. The size of @a gml_properties_ must be
		 * the same as @a multipoint_.
		 */
		static
		const non_null_ptr_type
		create(
				const internal_multipoint_type &multipoint_,
				const std::vector<GmlPoint::GmlProperty> &gml_properties_);

		const GmlMultiPoint::non_null_ptr_type
		clone() const
		{
			GmlMultiPoint::non_null_ptr_type dup(new GmlMultiPoint(*this));
			return dup;
		}

		const GmlMultiPoint::non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		/**
		 * Access the GPlatesMaths::MultiPointOnSphere which encodes the geometry of this
		 * instance.
		 *
		 * Note that there is no accessor provided which returns a boost::intrusive_ptr to
		 * a non-const GPlatesMaths::MultiPointOnSphere.  The GPlatesMaths::MultiPointOnSphere
		 * within this instance should not be modified directly; to alter the
		 * GPlatesMaths::MultiPointOnSphere within this instance, set a new value using the
		 * function @a set_multipoint below.
		 */
		const internal_multipoint_type
		multipoint() const
		{
			return d_multipoint;
		}

		/**
		 * Set the GPlatesMaths::MultiPointOnSphere within this instance to @a p.
		 *
		 * Note: this sets all gml:Points to use gml:pos as their property.
		 */
		void
		set_multipoint(
				const internal_multipoint_type &p)
		{
			d_multipoint = p;
			fill_gml_properties();
			update_instance_id();
		}

		const std::vector<GmlPoint::GmlProperty> &
		gml_properties() const
		{
			return d_gml_properties;
		}

		void
		set_gml_properties(
				const std::vector<GmlPoint::GmlProperty> &gml_properties_)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_multipoint->number_of_points() == gml_properties_.size(),
					GPLATES_ASSERTION_SOURCE);

			d_gml_properties = gml_properties_;
			update_instance_id();
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("MultiPoint");
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
			visitor.visit_gml_multi_point(*this);
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
			visitor.visit_gml_multi_point(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlMultiPoint(
				const internal_multipoint_type &multipoint_);

		GmlMultiPoint(
				const internal_multipoint_type &multipoint_,
				const std::vector<GmlPoint::GmlProperty> &gml_properties_);

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlMultiPoint(
				const GmlMultiPoint &other) :
			PropertyValue(other), /* share instance id */
			d_multipoint(other.d_multipoint),
			d_gml_properties(other.d_gml_properties)
		{  }

	private:

		/**
		 * Fills d_gml_properties with d_multipoint.size() of GmlPoint::POS.
		 */
		void
		fill_gml_properties();

		internal_multipoint_type d_multipoint;

		// It's not the nicest OO, but this vector must be of the same size as d_multipoint.
		std::vector<GmlPoint::GmlProperty> d_gml_properties;

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLMULTIPOINT_H
