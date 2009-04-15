/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GMLDOMAINSET_H
#define GPLATES_PROPERTYVALUES_GMLDOMAINSET_H

#include <utility>  /* std::pair */
#include "model/PropertyValue.h"
#include "maths/GeometryForwardDeclarations.h"
#include "GmlMultiPoint.h"


namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:domainSet".
	 */
	class GmlDomainSet:
			public GPlatesModel::PropertyValue
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlDomainSet,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlDomainSet,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GmlDomainSet,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlDomainSet,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		virtual
		~GmlDomainSet()
		{  }

		/**
		 * Create a GmlDomainSet instance from a GmlMultiPoint instance.
		 */
		static
		const non_null_ptr_type
		create( 
			GmlMultiPoint::non_null_ptr_type &mp) {
			non_null_ptr_type domain_set_ptr(
					new GmlDomainSet( mp ),
					GPlatesUtils::NullIntrusivePointerHandler());
			return domain_set_ptr;
		}

		/**
		 * Create a duplicate of this PropertyValue instance.
		 */
		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const
		{
			GPlatesModel::PropertyValue::non_null_ptr_type dup(
					new GmlDomainSet(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		/**
		 * Access the GmlMultiPoint which encodes the geometry of this instance.
		 */
		const GPlatesUtils::non_null_intrusive_ptr<GmlMultiPoint,
				GPlatesUtils::NullIntrusivePointerHandler>
		get_gml_multi_point() const
		{
			return d_multi_point;
		}

		/**
		 * Set the point within this instance to @a p.
		 *
		 * FIXME: when we have undo/redo, this act should cause
		 * a new revision to be propagated up to the Feature which
		 * contains this PropertyValue.
		 */
		void
		set_gml_multi_point(
				GPlatesUtils::non_null_intrusive_ptr<GmlMultiPoint,
						GPlatesUtils::NullIntrusivePointerHandler> mp)
		{
			d_multi_point = mp;
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
			visitor.visit_gml_domain_set(*this);
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
			visitor.visit_gml_domain_set(*this);
		}

	protected:

#if 0
#endif
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GmlDomainSet(
				GPlatesUtils::non_null_intrusive_ptr<GmlMultiPoint,
						GPlatesUtils::NullIntrusivePointerHandler> multi_point_):
			PropertyValue(),
			d_multi_point( multi_point_ )
		{  }


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlDomainSet(
				const GmlDomainSet &other):
			PropertyValue(),
			d_multi_point(other.d_multi_point)
		{  }

	private:

		// the multi point 
		GmlMultiPoint::non_null_ptr_type d_multi_point;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlDomainSet &
		operator=(
				const GmlDomainSet &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLDOMAINSET_H
