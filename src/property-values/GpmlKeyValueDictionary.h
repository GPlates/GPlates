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

#include <cstddef>
#include <vector>

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"

#include "property-values/GpmlKeyValueDictionaryElement.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlKeyValueDictionary, visit_gpml_key_value_dictionary)

namespace GPlatesPropertyValues
{

	class GpmlKeyValueDictionaryElement;

	class GpmlKeyValueDictionary :
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
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
		const non_null_ptr_type
		create()
		{
			return create(std::vector<GpmlKeyValueDictionaryElement>());
		}

		static
		const non_null_ptr_type
		create(
			const std::vector<GpmlKeyValueDictionaryElement> &elements);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlKeyValueDictionary>(clone_impl());
		}

		/**
		 * Returns the dictionary elements.
		 *
		 * To modify any dictionary elements:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a set_elements to set them.
		 *
		 * The returned dictionary elements implement copy-on-write to promote resource sharing (until write)
		 * and to ensure our internal state cannot be modified and bypass the revisioning system.
		 */
		const std::vector<GpmlKeyValueDictionaryElement> &
		get_elements() const
		{
			return get_current_revision<Revision>().elements;
		}

		/**
		 * Sets the internal dictionary elements.
		 */
		void
		set_elements(
				const std::vector<GpmlKeyValueDictionaryElement> &elements);

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

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlKeyValueDictionary(
				GPlatesModel::ModelTransaction &transaction_,
				const std::vector<GpmlKeyValueDictionaryElement> &elements_):
			PropertyValue(Revision::non_null_ptr_type(new Revision(transaction_, *this, elements_)))
		{  }

		//! Constructor used when cloning.
		GpmlKeyValueDictionary(
				const GpmlKeyValueDictionary &other_,
				boost::optional<RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlKeyValueDictionary(*this, context));
		}

	private:

		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a RevisionContext.
		 */
		virtual
		GPlatesModel::Revision::non_null_ptr_type
		bubble_up(
				GPlatesModel::ModelTransaction &transaction,
				const Revisionable::non_null_ptr_to_const_type &child_revisionable);

		/**
		 * Inherited from @a RevisionContext.
		 */
		virtual
		boost::optional<GPlatesModel::Model &>
		get_model()
		{
			return PropertyValue::get_model();
		}

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			explicit
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					const std::vector<GpmlKeyValueDictionaryElement> &elements_) :
				elements(elements_)
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				elements(other_.elements)
			{
				// Clone data members that were not deep copied.
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				elements(other_.elements)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<RevisionContext &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return elements == other_revision.elements &&
						GPlatesModel::Revision::equality(other);
			}

			std::vector<GpmlKeyValueDictionaryElement> elements;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARY_H
