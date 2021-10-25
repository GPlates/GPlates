/* $Id$ */

/**
 * \file 
 * This contains the definition of class GpmlStringList.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_PROPERTYVALUES_GPMLSTRINGLIST_H
#define GPLATES_PROPERTYVALUES_GPMLSTRINGLIST_H

#include <vector>
#include <iterator>
#include <boost/intrusive_ptr.hpp>

#include "TextContent.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlStringList, visit_gpml_string_list)

namespace GPlatesPropertyValues
{
	/**
	 * A list of XsString instances in a "gpml:StringList".
	 *
	 * There are three 'create' functions which may be used to instantiate a GpmlStringList:
	 *  -# @a create_empty
	 *  -# @a create_copy
	 *  -# @a create_swap
	 */
	class GpmlStringList:
			public GPlatesModel::PropertyValue
	{
	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlStringList>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlStringList> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlStringList>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlStringList> non_null_ptr_to_const_type;

		/**
		 * The type used to contain the list of strings.
		 *
		 * We're assuming that the TextContent type performs some sort of string-sharing,
		 * so it won't be too expensive when the vector is resized.
		 *
		 * (Currently TextContent is implemented as a StringContentTypeGenerator for a
		 * StringSetSingleton, so our assumption holds.)
		 */
		typedef std::vector<TextContent> string_list_type;

		/**
		 * The type used to iterate over the list of strings.
		 */
		typedef string_list_type::const_iterator const_iterator;
		typedef string_list_type::iterator iterator;

		virtual
		~GpmlStringList()
		{  }

		/**
		 * Create a new GpmlStringList instance, leaving its elements empty.
		 *
		 * You can append strings using the member functions @a push_back.
		 */
		static
		const non_null_ptr_type
		create_empty()
		{
			non_null_ptr_type ptr(new GpmlStringList);
			return ptr;
		}

		/**
		 * Create a new GpmlStringList instance, then copy the values from @a strings
		 * into the GpmlStringList.
		 *
		 * The template type StringContainer is expected to be a container of TextContent
		 * or a container of UnicodeString.
		 */
		template<typename StringContainer>
		static
		const non_null_ptr_type
		create_copy(
				const StringContainer &strings)
		{
			return create_copy(strings.begin(), strings.end());
		}

		/**
		 * Create a new GpmlStringList instance, then copy the values from the iterator
		 * range [ @a strings_begin_, @a strings_end_ ) into the GpmlStringList.
		 *
		 * The template type StringIter is intended to be a forward iterator that
		 * dereferences to either a TextContent instance or a UnicodeString instance.
		 */
		template<typename StringIter>
		static
		const non_null_ptr_type
		create_copy(
				StringIter strings_begin_,
				StringIter strings_end_)
		{
			non_null_ptr_type ptr(new GpmlStringList(strings_begin_, strings_end_));
			return ptr;
		}

		/**
		 * Create a new GpmlStringList instance, then swap the contents of the supplied
		 * container @a strings_to_swap into the GpmlStringList, leaving @a strings_to_swap
		 * empty.
		 */
		static
		const non_null_ptr_type
		create_swap(
				string_list_type &strings_to_swap)
		{
			non_null_ptr_type ptr(new GpmlStringList);
			ptr->d_strings.swap(strings_to_swap);
			return ptr;
		}

		const GpmlStringList::non_null_ptr_type
		clone() const
		{
			GpmlStringList::non_null_ptr_type dup(new GpmlStringList(*this));
			return dup;
		}

		const GpmlStringList::non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular 'clone' will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		bool
		is_empty() const
		{
			return d_strings.empty();
		}

		string_list_type::size_type
		size() const
		{
			return d_strings.size();
		}

		const_iterator
		begin() const
		{
			return d_strings.begin();
		}

		const_iterator
		end() const
		{
			return d_strings.end();
		}

		void
		push_back(
				const GPlatesUtils::UnicodeString &s)
		{
			push_back(TextContent(s));
		}

		void
		push_back(
				const TextContent &tc)
		{
			d_strings.push_back(tc);
			update_instance_id();
		}

		const_iterator
		insert(
				const_iterator pos,
				const GPlatesUtils::UnicodeString &s)
		{
			return insert(pos, TextContent(s));
		}

		const_iterator
		insert(
				const_iterator pos,
				const TextContent &tc)
		{
			string_list_type::iterator nc_pos = convert_to_non_const(pos);
			string_list_type::iterator new_pos = d_strings.insert(nc_pos, tc);
			update_instance_id();
			return new_pos;
		}

		const_iterator
		erase(
				string_list_type::const_iterator pos)
		{
			string_list_type::iterator nc_pos = convert_to_non_const(pos);
			string_list_type::iterator next_pos = d_strings.erase(nc_pos);
			update_instance_id();
			return next_pos;
		}

		void
		clear()
		{
			d_strings.clear();
			update_instance_id();
		}

		/**
		 * Swap the contents of @a strings with the contents of the GpmlStringList.
		 */
		void
		swap(
				string_list_type &strings_)
		{
			d_strings.swap(strings_);
			update_instance_id();
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("StringList");
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
			visitor.visit_gpml_string_list(*this);
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
			visitor.visit_gpml_string_list(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlStringList():
			PropertyValue()
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		template<typename StringIter>
		GpmlStringList(
				StringIter strings_begin_,
				StringIter strings_end_):
			PropertyValue(),
			d_strings(strings_begin_, strings_end_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlStringList(
				const GpmlStringList &other) :
			PropertyValue(other), /* share instance id */
			d_strings(other.d_strings)
		{  }

		string_list_type::iterator
		convert_to_non_const(
				const_iterator iter)
		{
			string_list_type::iterator nc_iter = d_strings.begin();

			// Since string_list_type is a std::vector, both 'std::distance' and
			// 'std::advance' should be O(1) operations.
			string_list_type::size_type n_steps = std::distance(begin(), iter);
			std::advance(nc_iter, n_steps);

			return nc_iter;
		}

	private:
		string_list_type d_strings;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlStringList &
		operator=(const GpmlStringList &);
	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLSTRINGLIST_H
