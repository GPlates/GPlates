/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 Geological Survey of Norway.
 * Copyright (C) 2010 The University of Sydney, Australia.
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

#ifndef GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARY_H
#define GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARY_H

#include <vector>

#include "GpmlKeyValueDictionaryElement.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlKeyValueDictionary, visit_gpml_key_value_dictionary)

namespace GPlatesPropertyValues
{

	class GpmlKeyValueDictionaryElement;

	class GpmlKeyValueDictionary :
			public GPlatesModel::PropertyValue
	{

	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlKeyValueDictionary>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlKeyValueDictionary> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlKeyValueDictionary>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlKeyValueDictionary> non_null_ptr_to_const_type;


		virtual
		~GpmlKeyValueDictionary()
		{  }

		static
		non_null_ptr_type
		create()
		{
			return create(std::vector<GpmlKeyValueDictionaryElement>());
		}

		static
		non_null_ptr_type
		create(
			const std::vector<GpmlKeyValueDictionaryElement> &elements)
		{
			return non_null_ptr_type(new GpmlKeyValueDictionary(elements));
		}

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GpmlKeyValueDictionary(*this));
		}

		const GpmlKeyValueDictionary::non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		const std::vector<GpmlKeyValueDictionaryElement> &
		elements() const
		{
				return d_elements;
		}

		std::vector<GpmlKeyValueDictionaryElement> &
		elements()
		{
				return d_elements;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("KeyValueDictionary");
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
			visitor.visit_gpml_key_value_dictionary(*this);
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
			visitor.visit_gpml_key_value_dictionary(*this);
		}

		bool
		is_empty() const
		{
			return d_elements.empty();
		}

		// FIXME: Why does this return an 'int', rather than a 'std::vector<T>::size_type'?
		int
		num_elements() const
		{
			return d_elements.size();
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlKeyValueDictionary():
			PropertyValue()
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlKeyValueDictionary(
			const std::vector<GpmlKeyValueDictionaryElement> &elements_):
			PropertyValue(),
			d_elements(elements_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlKeyValueDictionary(
				const GpmlKeyValueDictionary &other) :
			PropertyValue(other) /* share instance id */
		{  }

		virtual
		bool
		directly_modifiable_fields_equal(
				const PropertyValue &other) const;

	private:

		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> d_elements;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlKeyValueDictionary &
		operator=(const GpmlKeyValueDictionary &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARY_H
