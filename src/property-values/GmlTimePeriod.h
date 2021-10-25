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

#include "global/PreconditionViolationError.h"

#include "model/PropertyValue.h"


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


		/**
		 * A time period's begin time should be earlier than its end time.
		 */
		class BeginTimeLaterThanEndTimeException :
				public GPlatesGlobal::PreconditionViolationError
		{
		public:
			explicit
			BeginTimeLaterThanEndTimeException(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				GPlatesGlobal::PreconditionViolationError(exception_source)
			{  }

			~BeginTimeLaterThanEndTimeException() throw()
			{  }

		protected:
			virtual
			const char *
			exception_name() const
			{
				return "BeginTimeLaterThanEndTimeException";
			}
		};


		virtual
		~GmlTimePeriod()
		{  }

		/**
		 * Create a GmlTimePeriod instance which begins at @a begin_ and ends at @a end_.
		 *
		 * Note that the time instant represented by @a begin_ must not be later than (ie,
		 * more recent than) the time instant represented by @a end_.
		 *
		 * @throws BeginTimeLaterThanEndTimeException if @a check_begin_end_times is true and
		 * begin time is later than end time. By default @a check_begin_end_times is false since
		 * there exists a lot of data from files that violates this condition.
		 */
		static
		const non_null_ptr_type
		create(
				GmlTimeInstant::non_null_ptr_type begin_,
				GmlTimeInstant::non_null_ptr_type end_,
				bool check_begin_end_times = false);

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GmlTimePeriod(*this));
		}

		const GmlTimePeriod::non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		/**
		 * Return the 'const' "begin" attribute of this GmlTimePeriod instance.
		 */
		const GmlTimeInstant::non_null_ptr_to_const_type
		begin() const
		{
			return d_begin;
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
			return d_begin;
		}

		/**
		 * Set the "begin" attribute of this GmlTimePeriod instance.
		 *
		 * Note that it is an invariant of this class that the "begin" attribute must not
		 * be later than the "end" attribute.
		 *
		 * @throws BeginTimeLaterThanEndTimeException if @a check_begin_end_times is true and
		 * begin time is later than end time. By default @a check_begin_end_times is false since
		 * there exists a lot of data from files that violates this condition.
		 */
		void
		set_begin(
				GmlTimeInstant::non_null_ptr_type begin_,
				bool check_begin_end_times = false);

		/**
		 * Return the "end" attribute of this GmlTimePeriod instance.
		 */
		const GmlTimeInstant::non_null_ptr_to_const_type
		end() const
		{
			return d_end;
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
			return d_end;
		}

		/**
		 * Set the "end" attribute of this GmlTimePeriod instance.
		 *
		 * Note that it is an invariant of this class that the "end" attribute must not
		 * be earlier than the "begin" attribute.
		 *
		 * @throws BeginTimeLaterThanEndTimeException if @a check_begin_end_times is true and
		 * begin time is later than end time. By default @a check_begin_end_times is false since
		 * there exists a lot of data from files that violates this condition.
		 */
		void
		set_end(
				GmlTimeInstant::non_null_ptr_type end_,
				bool check_begin_end_times = false);

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
			return begin()->time_position().is_earlier_than_or_coincident_with(geo_time) &&
					geo_time.is_earlier_than_or_coincident_with(end()->time_position());
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
				GmlTimeInstant::non_null_ptr_type begin_,
				GmlTimeInstant::non_null_ptr_type end_):
			PropertyValue(),
			d_begin(begin_),
			d_end(end_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlTimePeriod(
				const GmlTimePeriod &other) :
			PropertyValue(other), /* share instance id */
			d_begin(other.d_begin),
			d_end(other.d_end)
		{  }

		virtual
		bool
		directly_modifiable_fields_equal(
				const PropertyValue &other) const;

	private:

		GmlTimeInstant::non_null_ptr_type d_begin;
		GmlTimeInstant::non_null_ptr_type d_end;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlTimePeriod &
		operator=(const GmlTimePeriod &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLTIMEPERIOD_H
