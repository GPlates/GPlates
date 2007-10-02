/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_XSBOOLEAN_H
#define GPLATES_PROPERTYVALUES_XSBOOLEAN_H

#include "model/PropertyValue.h"
#include "TextContent.h"


namespace GPlatesPropertyValues {

	class XsBoolean :
			public GPlatesModel::PropertyValue {

	public:

		/**
		 * A convenience typedef for GPlatesContrib::non_null_intrusive_ptr<XsBoolean>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<XsBoolean> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<const XsBoolean>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<const XsBoolean>
				non_null_ptr_to_const_type;

		virtual
		~XsBoolean() {  }

		static
		const non_null_ptr_type
		create(
				bool value) {
			XsBoolean::non_null_ptr_type ptr(*(new XsBoolean(value)));
			return ptr;
		}

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const {
			GPlatesModel::PropertyValue::non_null_ptr_type dup(*(new XsBoolean(*this)));
			return dup;
		}

		bool
		value() const {
			return d_value;
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
				GPlatesModel::ConstFeatureVisitor &visitor) const {
			visitor.visit_xs_boolean(*this);
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
			visitor.visit_xs_boolean(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		XsBoolean(
				bool value_) :
			PropertyValue(),
			d_value(value_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		XsBoolean(
				const XsBoolean &other) :
			PropertyValue(other),
			d_value(other.d_value)
		{  }

	private:

		bool d_value;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		XsBoolean &
		operator=(const XsBoolean &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_XSSTRING_H
