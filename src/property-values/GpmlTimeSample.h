/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLTIMESAMPLE_H
#define GPLATES_PROPERTYVALUES_GPMLTIMESAMPLE_H

#include <iosfwd>
#include <boost/optional.hpp>

#include "GmlTimeInstant.h"

#include "StructuralType.h"
#include "XsString.h"

#include "model/PropertyValue.h"
#include "model/Revisionable.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"

#include "utils/QtStreamable.h"


namespace GPlatesPropertyValues
{

	// Since all the members of this class are of type non_null_intrusive_ptr or
	// StructuralType (which wraps an StringSet::SharedIterator instance which
	// points to a pre-allocated node in a StringSet), none of the construction,
	// copy-construction or copy-assignment operations for this class should throw.
	class GpmlTimeSample :
			public GPlatesModel::Revisionable,
			public GPlatesModel::RevisionContext,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<GpmlTimeSample>
	{
	public:

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlTimeSample>.
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlTimeSample> non_null_ptr_type;

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlTimeSample>.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlTimeSample> non_null_ptr_to_const_type;


		static
		non_null_ptr_type
		create(
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				GmlTimeInstant::non_null_ptr_type valid_time_,
				boost::optional<XsString::non_null_ptr_type> description_,
				const StructuralType &value_type_,
				bool is_disabled_ = false);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlTimeSample>(clone_impl());
		}

		/**
		 * Returns the 'const' time-dependent property value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		value() const
		{
			return get_current_revision<Revision>().value.get_revisionable();
		}

		/**
		 * Returns the 'non-const' time-dependent property value.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_type
		value()
		{
			return get_current_revision<Revision>().value.get_revisionable();
		}

		void
		set_value(
				GPlatesModel::PropertyValue::non_null_ptr_type v);

		/**
		 * Returns the 'const' time instant.
		 */
		const GmlTimeInstant::non_null_ptr_to_const_type
		valid_time() const
		{
			return get_current_revision<Revision>().valid_time.get_revisionable();
		}

		/**
		 * Returns the 'non-const' time instant.
		 */
		const GmlTimeInstant::non_null_ptr_type
		valid_time()
		{
			return get_current_revision<Revision>().valid_time.get_revisionable();
		}

		void
		set_valid_time(
				GmlTimeInstant::non_null_ptr_type vt);

		/**
		 * Returns the 'const' description.
		 */
		const boost::optional<XsString::non_null_ptr_to_const_type>
		description() const;

		/**
		 * Returns the 'non-const' description.
		 */
		const boost::optional<XsString::non_null_ptr_type>
		description();

		void
		set_description(
				boost::optional<XsString::non_null_ptr_type> d);

		bool
		is_disabled() const
		{
			return get_current_revision<Revision>().is_disabled;
		}

		void
		set_disabled(
				bool is_disabled_);

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
		GpmlTimeSample(
				GPlatesModel::ModelTransaction &transaction_,
				GPlatesModel::PropertyValue::non_null_ptr_type value_,
				GmlTimeInstant::non_null_ptr_type valid_time_,
				boost::optional<XsString::non_null_ptr_type> description_,
				const StructuralType &value_type_,
				bool is_disabled_) :
			Revisionable(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, value_, valid_time_, description_, is_disabled_))),
			d_value_type(value_type_)
		{  }

		//! Constructor used when cloning.
		GpmlTimeSample(
				const GpmlTimeSample &other_,
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
			return non_null_ptr_type(new GpmlTimeSample(*this, context));
		}

		virtual
		bool
		equality(
				const Revisionable &other) const
		{
			const GpmlTimeSample &other_pv = dynamic_cast<const GpmlTimeSample &>(other);

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
					GPlatesModel::PropertyValue::non_null_ptr_type value_,
					GmlTimeInstant::non_null_ptr_type valid_time_,
					boost::optional<XsString::non_null_ptr_type> description_,
					bool is_disabled_);

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_);

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_);

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


			GPlatesModel::RevisionedReference<GPlatesModel::PropertyValue> value;

			GPlatesModel::RevisionedReference<GmlTimeInstant> valid_time;

			/**
			 * The description is optional.
			 */
			boost::optional<GPlatesModel::RevisionedReference<XsString> > description;

			bool is_disabled;
		};


		StructuralType d_value_type;
	};

	// operator<< for GpmlTimeSample.
	std::ostream &
	operator<<(
			std::ostream &os,
			const GpmlTimeSample &time_sample);

}

#endif  // GPLATES_PROPERTYVALUES_GPMLTIMESAMPLE_H
