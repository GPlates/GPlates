/* $Id: GpmlArray.h 10193 2010-11-11 16:12:10Z rwatson $ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date: 2010-11-12 03:12:10 +1100 (Fri, 12 Nov 2010) $
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_PROPERTYVALUES_GPMLARRAY_H
#define GPLATES_PROPERTYVALUES_GPMLARRAY_H

#include <vector>
#include <boost/operators.hpp>

#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureVisitor.h"
#include "model/ModelTransaction.h"
#include "model/PropertyValue.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlArray, visit_gpml_array)

namespace GPlatesPropertyValues
{

	class GpmlArray:
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
	{

	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlArray>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlArray> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlArray>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlArray> non_null_ptr_to_const_type;


		/**
		 * A member of the array.
		 */
		class Member :
				public boost::equality_comparable<Member>
		{
		public:

			/**
			 * Member has value semantics where each @a Member instance has its own state.
			 * So if you create a copy and modify the copy's state then it will not modify the state
			 * of the original object.
			 *
			 * The constructor first clones the property value and then copy-on-write is used to allow
			 * multiple @a Member objects to share the same state (until the state is modified).
			 */
			Member(
					GPlatesModel::PropertyValue::non_null_ptr_type value) :
				d_value(value)
			{  }

			/**
			 * Returns the 'const' property value.
			 */
			const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
			get_value() const
			{
				return d_value;
			}

			/**
			 * Returns the 'non-const' property value.
			 */
			const GPlatesModel::PropertyValue::non_null_ptr_type
			get_value()
			{
				return d_value;
			}

			void
			set_value(
					GPlatesModel::PropertyValue::non_null_ptr_type value)
			{
				d_value = value;
			}

			/**
			 * Value equality comparison operator.
			 *
			 * Inequality provided by boost equality_comparable.
			 */
			bool
			operator==(
					const Member &other) const
			{
				return *d_value == *other.d_value;
			}

		private:
			GPlatesModel::PropertyValue::non_null_ptr_type d_value;
		};

		//! Typedef for a sequence of array members.
		typedef std::vector<Member> member_array_type;


		virtual
		~GpmlArray()
		{  }


		static
		const non_null_ptr_type
		create(
			const StructuralType &value_type,		
			const member_array_type &members)
		{
			return create(value_type, members.begin(), members.end());
		}

		template<typename ForwardIterator>
		static
		const non_null_ptr_type
		create(
				const StructuralType &value_type,		
				ForwardIterator begin,
				ForwardIterator end)
		{
			GPlatesModel::ModelTransaction transaction;
			non_null_ptr_type ptr(new GpmlArray(transaction, value_type, begin, end));
			transaction.commit();
			return ptr;
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlArray>(clone_impl());
		}

		/**
		 * Returns the members.
		 *
		 * To modify any members:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a set_members to set them.
		 *
		 * The returned members implement copy-on-write to promote resource sharing (until write)
		 * and to ensure our internal state cannot be modified and bypass the revisioning system.
		 */
		const member_array_type &
		get_members() const
		{
			return get_current_revision<Revision>().members;
		}

		/**
		 * Sets the internal members.
		 */
		void
		set_members(
				const member_array_type &members);

		// Note that no "setter" is provided:  The value type of a GpmlArray
		// instance should never be changed.
		StructuralType
		get_value_type() const
		{
			return d_value_type;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("Array");
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
			visitor.visit_gpml_array(*this);
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
			visitor.visit_gpml_array(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:


		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		template<typename ForwardIterator>
		GpmlArray(
				GPlatesModel::ModelTransaction &transaction_,
				const StructuralType &value_type,
				ForwardIterator begin,
				ForwardIterator end) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(transaction_, *this, begin, end))),
			d_value_type(value_type)
		{  }

		//! Constructor used when cloning.
		GpmlArray(
				const GpmlArray &other_,
				boost::optional<RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this))),
			d_value_type(other_.d_value_type)
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlArray(*this, context));
		}

		virtual
		bool
		equality(
				const Revisionable &other) const
		{
			const GpmlArray &other_pv = dynamic_cast<const GpmlArray &>(other);

			return d_value_type == other_pv.d_value_type &&
					// The revisioned data comparisons are handled here...
					Revisionable::equality(other);
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
			template<typename ForwardIterator>
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					ForwardIterator begin_,
					ForwardIterator end_) :
				members(begin_, end_)
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				members(other_.members)
			{
				// Clone data members that were not deep copied.
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				members(other_.members)
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
					const GPlatesModel::Revision &other) const;

			member_array_type members;
		};

		StructuralType d_value_type;

	};
}

#endif // GPLATES_PROPERTYVALUES_GPMLARRAY_H
