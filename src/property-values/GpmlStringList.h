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
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlStringList>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlStringList> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlStringList>.
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
			return non_null_ptr_type(new GpmlStringList);
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
			return non_null_ptr_type(new GpmlStringList(strings_begin_, strings_end_));
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
			return non_null_ptr_type(new GpmlStringList(strings_to_swap));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlStringList>(clone_impl());
		}


		/**
		 * Returns the string list.
		 *
		 * To modify any strings:
		 * (1) make a copy of the returned vector,
		 * (2) make additions/removals/modifications to the returned vector, and
		 * (3) use @a set_string_list to set them.
		 */
		const string_list_type &
		get_string_list() const
		{
			return get_current_revision<Revision>().strings;
		}

		/**
		 * Sets the string list.
		 */
		void
		set_string_list(
				const string_list_type &strings_);


		/**
		 * Swap the contents of @a strings with the contents of the GpmlStringList.
		 */
		void
		swap(
				string_list_type &strings_);

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
		GpmlStringList() :
			PropertyValue(Revision::non_null_ptr_type(new Revision()))
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		template<typename StringIter>
		GpmlStringList(
				StringIter strings_begin_,
				StringIter strings_end_):
			PropertyValue(Revision::non_null_ptr_type(new Revision(strings_begin_, strings_end_)))
		{  }

		// This constructor is used by @a create_swap.
		explicit
		GpmlStringList(
				string_list_type &strings_to_swap_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(strings_to_swap_)))
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlStringList(
				const GpmlStringList &other) :
			PropertyValue(other)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlStringList(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			Revision()
			{  }

			template<typename StringIter>
			Revision(
					StringIter strings_begin_,
					StringIter strings_end_) :
				strings(strings_begin_, strings_end_)
			{  }

			explicit
			Revision(
					string_list_type &strings_to_swap_)
			{
				swap_strings(strings_to_swap_);
			}

			void
			swap_strings(
					string_list_type &strings_to_swap_)
			{
				strings.swap(strings_to_swap_);
			}

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return strings == other_revision.strings &&
					GPlatesModel::PropertyValue::Revision::equality(other);
			}

			string_list_type strings;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLSTRINGLIST_H
