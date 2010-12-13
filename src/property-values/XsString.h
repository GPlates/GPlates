/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_XSSTRING_H
#define GPLATES_PROPERTYVALUES_XSSTRING_H

#include "TextContent.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"

#include "utils/UnicodeStringUtils.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::XsString, visit_xs_string)

namespace GPlatesPropertyValues
{

	class XsString :
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<XsString>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<XsString> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const XsString>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const XsString> non_null_ptr_to_const_type;

		virtual
		~XsString()
		{  }

		static
		const non_null_ptr_type
		create(
				const GPlatesUtils::UnicodeString &s)
		{
			XsString::non_null_ptr_type ptr(new XsString(s));
			return ptr;
		}

		const XsString::non_null_ptr_type
		clone() const
		{
			XsString::non_null_ptr_type dup(new XsString(*this));
			return dup;
		}

		const XsString::non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		/**
		 * Accesses the TextContent contained within this XsString.
		 *
		 * Note that this does not allow you to modify the TextContent contained
		 * within this XsString directly; for that, you should set a new
		 * TextContent using the @a set_value function below.
		 */
		const TextContent &
		value() const
		{
			return d_value;
		}

		/**
		 * Set the TextContent contained within this XsString to @a tc.
		 * TextContent can be created by passing a UnicodeString in to
		 * TextContent's constructor.
		 * 
		 * FIXME: when we have undo/redo, this act should cause
		 * a new revision to be propagated up to the Feature which
		 * contains this PropertyValue.
		 */
		void
		set_value(
				const TextContent &tc)
		{
			d_value = tc;
			update_instance_id();
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
			visitor.visit_xs_string(*this);
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
			visitor.visit_xs_string(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		XsString(
				const GPlatesUtils::UnicodeString &s) :
			PropertyValue(),
			d_value(s)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		XsString(
				const XsString &other) :
			PropertyValue(other), /* share instance id */
			d_value(other.d_value)
		{  }

	private:

		TextContent d_value;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		XsString &
		operator=(const XsString &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_XSSTRING_H
