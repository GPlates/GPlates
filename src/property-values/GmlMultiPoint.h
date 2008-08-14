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

#ifndef GPLATES_PROPERTYVALUES_GMLMULTIPOINT_H
#define GPLATES_PROPERTYVALUES_GMLMULTIPOINT_H

#include <vector>
#include "model/PropertyValue.h"
#include "maths/GeometryForwardDeclarations.h"
#include "maths/PointOnSphere.h"


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
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlMultiPoint,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlMultiPoint,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GmlMultiPoint,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlMultiPoint,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for the internal multipoint representation.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::MultiPointOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler>
				internal_multipoint_type;

		virtual
		~GmlMultiPoint()
		{  }

		/**
		 * Create a GmlMultiPoint instance which contains a copy of @a multipoint_.
		 */
		static
		const non_null_ptr_type
		create(
				const internal_multipoint_type &multipoint_);

		/**
		 * Create a duplicate of this PropertyValue instance.
		 */
		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const
		{
			GPlatesModel::PropertyValue::non_null_ptr_type dup(new GmlMultiPoint(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

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
		 * FIXME: when we have undo/redo, this act should cause
		 * a new revision to be propagated up to the Feature which
		 * contains this PropertyValue.
		 */
		void
		set_multipoint(
				const internal_multipoint_type &p)
		{
			d_multipoint = p;
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

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlMultiPoint(
				const internal_multipoint_type &multipoint_):
			PropertyValue(),
			d_multipoint(multipoint_)
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlMultiPoint(
				const GmlMultiPoint &other):
			PropertyValue(),
			d_multipoint(other.d_multipoint)
		{  }

	private:

		internal_multipoint_type d_multipoint;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlMultiPoint &
		operator=(
				const GmlMultiPoint &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLMULTIPOINT_H
