/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 Geological Survey of Norway.
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

#ifndef GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARYELEMENT_H
#define GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARYELEMENT_H

#include <iosfwd>

#include "model/Revisionable.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"

#include "property-values/StructuralType.h"
#include "property-values/XsString.h"

#include "utils/QtStreamable.h"


namespace GPlatesPropertyValues
{

	class GpmlKeyValueDictionaryElement :
			public GPlatesModel::Revisionable,
			public GPlatesModel::RevisionContext,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<GpmlKeyValueDictionaryElement>
	{

	public:

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlKeyValueDictionaryElement>.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlKeyValueDictionaryElement> non_null_ptr_type;

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlKeyValueDictionaryElement>.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlKeyValueDictionaryElement> non_null_ptr_to_const_type;


		static
		non_null_ptr_type
		create(
				XsString::non_null_ptr_type key_,
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				const StructuralType &value_type_);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlKeyValueDictionaryElement>(clone_impl());
		}

		/**
		 * Returns the 'const' key.
		 */
		const XsString::non_null_ptr_to_const_type
		key() const
		{
			return get_current_revision<Revision>().key.get_revisionable();
		}

		/**
		 * Returns the 'non-const' key.
		 */
		const XsString::non_null_ptr_type
		key()
		{
			return get_current_revision<Revision>().key.get_revisionable();
		}		

		void
		set_key(
				XsString::non_null_ptr_type key_);

		/**
		 * Returns the 'const' value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		value() const
		{
			return get_current_revision<Revision>().value.get_revisionable();
		}

		/**
		 * Returns the 'non-const' value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_type
		value()
		{
			return get_current_revision<Revision>().value.get_revisionable();
		}

		void
		set_value(
				GPlatesModel::PropertyValue::non_null_ptr_type value_);

		// Note that no "setter" is provided:  The value type of a GpmlTimeSample instance
		// should never be changed.
		const StructuralType &
		get_value_type() const
		{
			return d_value_type;
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlKeyValueDictionaryElement(
				GPlatesModel::ModelTransaction &transaction_,
				XsString::non_null_ptr_type key_,
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				const StructuralType &value_type_) :
			Revisionable(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, key_, value_))),
			d_value_type(value_type_)
		{  }

		//! Constructor used when cloning.
		GpmlKeyValueDictionaryElement(
				const GpmlKeyValueDictionaryElement &other_,
				boost::optional<RevisionContext &> context_) :
			Revisionable(
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
			return non_null_ptr_type(new GpmlKeyValueDictionaryElement(*this, context));
		}

		virtual
		bool
		equality(
				const Revisionable &other) const
		{
			const GpmlKeyValueDictionaryElement &other_pv =
					dynamic_cast<const GpmlKeyValueDictionaryElement &>(other);

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
				const GPlatesModel::Revisionable::non_null_ptr_to_const_type &child_revisionable);


		/**
		 * Inherited from @a RevisionContext.
		 */
		virtual
		boost::optional<GPlatesModel::Model &>
		get_model()
		{
			return GPlatesModel::Revisionable::get_model();
		}


		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::Revision
		{
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					XsString::non_null_ptr_type key_,
					GPlatesModel::PropertyValue::non_null_ptr_type value_) :
				key(
						GPlatesModel::RevisionedReference<XsString>::attach(
								transaction_, child_context_, key_)),
				value(
						GPlatesModel::RevisionedReference<GPlatesModel::PropertyValue>::attach(
								transaction_, child_context_, value_))
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				GPlatesModel::Revision(context_),
				key(other_.key),
				value(other_.value)
			{
				// Clone data members that were not deep copied.
				key.clone(child_context_);
				value.clone(child_context_);
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				GPlatesModel::Revision(context_),
				key(other_.key),
				value(other_.value)
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

				// Note that we compare the property value contents (and not pointers).
				return *key.get_revisionable() == *other_revision.key.get_revisionable() &&
						*value.get_revisionable() == *other_revision.value.get_revisionable() &&
						GPlatesModel::Revision::equality(other);
			}


			GPlatesModel::RevisionedReference<XsString> key;
			GPlatesModel::RevisionedReference<GPlatesModel::PropertyValue> value;
		};


		StructuralType d_value_type;
	};


	std::ostream &
	operator<<(
			std::ostream &os,
			const GpmlKeyValueDictionaryElement &element);

}

#endif  // GPLATES_PROPERTYVALUES_GPMLKEYVALUEDICTIONARYELEMENT_H
