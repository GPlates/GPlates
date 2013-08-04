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

#include "utils/CopyOnWrite.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
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
			public GPlatesModel::PropertyValue
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
				GmlTimeInstant::non_null_ptr_type end_)
		{
			return non_null_ptr_type(new GmlTimePeriod(begin_, end_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlTimePeriod>(clone_impl());
		}

		/**
		 * Return the "begin" attribute of this GmlTimePeriod instance.
		 *
		 * Note that it is an invariant of this class that the "begin" attribute must not
		 * be later than the "end" attribute.
		 *
		 * The returned property value is 'const' so that it cannot be modified and
		 * bypass the revisioning system.
		 */
		const GmlTimeInstant::non_null_ptr_to_const_type
		get_begin() const
		{
			return get_current_revision<Revision>().begin.get();
		}

		/**
		 * Set the "begin" attribute of this GmlTimePeriod instance.
		 *
		 * Note that it is an invariant of this class that the "begin" attribute must not
		 * be later than the "end" attribute.
		 */
		void
		set_begin(
				GmlTimeInstant::non_null_ptr_to_const_type begin);

		/**
		 * Return the "end" attribute of this GmlTimePeriod instance.
		 *
		 * Note that it is an invariant of this class that the "end" attribute must not
		 * be earlier than the "begin" attribute.
		 *
		 * The returned property value is 'const' so that it cannot be modified and
		 * bypass the revisioning system.
		 */
		const GmlTimeInstant::non_null_ptr_to_const_type
		get_end() const
		{
			return get_current_revision<Revision>().end.get();
		}

		/**
		 * Set the "end" attribute of this GmlTimePeriod instance.
		 *
		 * Note that it is an invariant of this class that the "end" attribute must not
		 * be earlier than the "begin" attribute.
		 */
		void
		set_end(
				GmlTimeInstant::non_null_ptr_to_const_type end);

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
			return get_begin()->get_time_position().is_earlier_than_or_coincident_with(geo_time) &&
					geo_time.is_earlier_than_or_coincident_with(get_end()->get_time_position());
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
				GmlTimeInstant::non_null_ptr_to_const_type begin_,
				GmlTimeInstant::non_null_ptr_to_const_type end_):
			PropertyValue(Revision::non_null_ptr_type(new Revision(begin_, end_)))
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlTimePeriod(
				const GmlTimePeriod &other) :
			PropertyValue(other)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GmlTimePeriod(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			Revision(
					const GmlTimeInstant::non_null_ptr_to_const_type &begin_,
					const GmlTimeInstant::non_null_ptr_to_const_type &end_) :
				begin(begin_),
				end(end_)
			{  }

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			// Don't need 'clone_for_bubble_up_modification()' since we're using CopyOnWrite.

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return *begin.get_const() == *other_revision.begin.get_const() &&
						*end.get_const() == *other_revision.end.get_const() &&
						GPlatesModel::PropertyValue::Revision::equality(other);
			}

			GPlatesUtils::CopyOnWrite<GmlTimeInstant::non_null_ptr_to_const_type> begin;
			GPlatesUtils::CopyOnWrite<GmlTimeInstant::non_null_ptr_to_const_type> end;
		};


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlTimePeriod &
		operator=(const GmlTimePeriod &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLTIMEPERIOD_H
