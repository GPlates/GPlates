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

#ifndef GPLATES_PROPERTYVALUES_GMLTIMEPERIOD_H
#define GPLATES_PROPERTYVALUES_GMLTIMEPERIOD_H

#include "GmlTimeInstant.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/RevisionContext.h"
#include "model/RevisionedReference.h"


// Enable GPlatesFeatureVisitors::get_revisionable() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlTimePeriod, visit_gml_time_period)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:TimePeriod".
	 *
	 * A "gml:TimePeriod" possesses two attributes: a "begin" time instant and an "end" time
	 * instant; Note that it is an invariant of this class that the "begin" attribute must not
	 * be later than the "end" attribute.
	 */
	class GmlTimePeriod:
			public GPlatesModel::PropertyValue,
			public GPlatesModel::RevisionContext
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlTimePeriod>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlTimePeriod> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlTimePeriod>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlTimePeriod> non_null_ptr_to_const_type;


		virtual
		~GmlTimePeriod()
		{  }

		/**
		 * Create a GmlTimePeriod instance which begins at @a begin_ and ends at @a end_.
		 *
		 * Note that the time instant represented by @a begin_ must not be later than (ie,
		 * more recent than) the time instant represented by @a end_.
		 *
		 * FIXME:  Check this as a precondition, and throw an exception if it's violated.
		 */
		static
		const non_null_ptr_type
		create(
				GmlTimeInstant::non_null_ptr_type begin_,
				GmlTimeInstant::non_null_ptr_type end_);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlTimePeriod>(clone_impl());
		}

		/**
		 * Return the 'const' "begin" attribute of this GmlTimePeriod instance.
		 */
		const GmlTimeInstant::non_null_ptr_to_const_type
		begin() const
		{
			return get_current_revision<Revision>().begin.get_revisionable();
		}

		/**
		 * Return the 'non-const' "begin" attribute of this GmlTimePeriod instance.
		 *
		 * Note that it is an invariant of this class that the "begin" attribute must not
		 * be later than the "end" attribute.
		 */
		const GmlTimeInstant::non_null_ptr_type
		begin()
		{
			return get_current_revision<Revision>().begin.get_revisionable();
		}

		/**
		 * Set the "begin" attribute of this GmlTimePeriod instance.
		 *
		 * Note that it is an invariant of this class that the "begin" attribute must not
		 * be later than the "end" attribute.
		 */
		void
		set_begin(
				GmlTimeInstant::non_null_ptr_type begin_);

		/**
		 * Return the "end" attribute of this GmlTimePeriod instance.
		 */
		const GmlTimeInstant::non_null_ptr_to_const_type
		end() const
		{
			return get_current_revision<Revision>().end.get_revisionable();
		}

		/**
		 * Return the "end" attribute of this GmlTimePeriod instance.
		 *
		 * Note that it is an invariant of this class that the "end" attribute must not
		 * be earlier than the "begin" attribute.
		 */
		const GmlTimeInstant::non_null_ptr_type
		end()
		{
			return get_current_revision<Revision>().end.get_revisionable();
		}

		/**
		 * Set the "end" attribute of this GmlTimePeriod instance.
		 *
		 * Note that it is an invariant of this class that the "end" attribute must not
		 * be earlier than the "begin" attribute.
		 */
		void
		set_end(
				GmlTimeInstant::non_null_ptr_type end_);

		/**
		 * Determine whether @a geo_time lies within the temporal span of this
		 * GmlTimePeriod instance.
		 *
		 * Note that this function @em will consider @a geo_time to lie "within" the
		 * temporal span, in the event that @a geo_time coincides with either (or both) of
		 * the bounding times.
		 */
		bool
		contains(
				const GeoTimeInstant &geo_time) const
		{
			return begin()->get_time_position().is_earlier_than_or_coincident_with(geo_time) &&
					geo_time.is_earlier_than_or_coincident_with(end()->get_time_position());
		}

		/**
		 * Determine whether @a geo_time lies within the temporal span of this GmlTimePeriod instance.
		 *
		 * This is an overloaded version of the above method.
		 */
		bool
		contains(
				const double &geo_time) const
		{
			return contains(GeoTimeInstant(geo_time));
		}


		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("TimePeriod");
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
			visitor.visit_gml_time_period(*this);
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
			visitor.visit_gml_time_period(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlTimePeriod(
				GPlatesModel::ModelTransaction &transaction_,
				GmlTimeInstant::non_null_ptr_type begin_,
				GmlTimeInstant::non_null_ptr_type end_):
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, begin_, end_)))
		{  }

		//! Constructor used when cloning.
		GmlTimePeriod(
				const GmlTimePeriod &other_,
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
			return non_null_ptr_type(new GmlTimePeriod(*this, context));
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
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					RevisionContext &child_context_,
					const GmlTimeInstant::non_null_ptr_type &begin_,
					const GmlTimeInstant::non_null_ptr_type &end_) :
				begin(
						GPlatesModel::RevisionedReference<GmlTimeInstant>::attach(
								transaction_, child_context_, begin_)),
				end(
						GPlatesModel::RevisionedReference<GmlTimeInstant>::attach(
								transaction_, child_context_, end_))
			{  }

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				PropertyValue::Revision(context_),
				begin(other_.begin),
				end(other_.end)
			{
				// Clone data members that were not deep copied.
				begin.clone(child_context_);
				end.clone(child_context_);
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				begin(other_.begin),
				end(other_.end)
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

				return *begin.get_revisionable() == *other_revision.begin.get_revisionable() &&
						*end.get_revisionable() == *other_revision.end.get_revisionable() &&
						GPlatesModel::Revision::equality(other);
			}

			GPlatesModel::RevisionedReference<GmlTimeInstant> begin;
			GPlatesModel::RevisionedReference<GmlTimeInstant> end;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLTIMEPERIOD_H
