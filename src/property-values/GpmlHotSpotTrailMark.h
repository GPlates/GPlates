/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLHOTSPOTTRAILMARK_H
#define GPLATES_PROPERTYVALUES_GPMLHOTSPOTTRAILMARK_H

#include <boost/optional.hpp>

#include "GmlTimeInstant.h"
#include "GmlTimePeriod.h"
#include "GmlPoint.h"
#include "GpmlMeasure.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"
#include "model/FeatureVisitor.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlHotSpotTrailMark, visit_gpml_hot_spot_trail_mark)

namespace GPlatesPropertyValues
{

	class GpmlHotSpotTrailMark:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlHotSpotTrailMark>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlHotSpotTrailMark> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlHotSpotTrailMark>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlHotSpotTrailMark> non_null_ptr_to_const_type;

		virtual
		~GpmlHotSpotTrailMark()
		{  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				const GmlPoint::non_null_ptr_type &position_,
				const boost::optional<GpmlMeasure::non_null_ptr_type> &trail_width_,
				const boost::optional<GmlTimeInstant::non_null_ptr_type> &measured_age_,
				const boost::optional<GmlTimePeriod::non_null_ptr_type> &measured_age_range_)
		{
			non_null_ptr_type ptr(
					new GpmlHotSpotTrailMark( position_, trail_width_,
							measured_age_, measured_age_range_));
			return ptr;
		}

		const GpmlHotSpotTrailMark::non_null_ptr_type
		clone() const
		{
			GpmlHotSpotTrailMark::non_null_ptr_type dup(new GpmlHotSpotTrailMark(*this));
			return dup;
		}

		const GpmlHotSpotTrailMark::non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		const GmlPoint::non_null_ptr_to_const_type
		position() const
		{
			return d_position;
		}

		// Note that, because the copy-assignment operator of GmlPoint is private,
		// the GmlPoint referenced by the return-value of this function cannot be
		// assigned-to, which means that this function does not provide a means to directly
		// switch the GmlPoint within this GpmlHotSpotTrailMark instance.  (This
		// restriction is intentional.)
		//
		// To switch the GmlPoint within this GpmlHotSpotTrailMark instance, use the
		// function @a set_position below.
		//
		// (This overload is provided to allow the referenced GmlPoint instance to
		// accept a FeatureVisitor instance.)
		const GmlPoint::non_null_ptr_type
		position()
		{
			return d_position;
		}

		void
		set_position(
				GmlPoint::non_null_ptr_type pos)
		{
			d_position = pos;
			update_instance_id();
		}

		const boost::optional<GpmlMeasure::non_null_ptr_type>
		trail_width() const
		{
			return d_trail_width;
		}

		// Note that, because the copy-assignment operator of GpmlMeasure is private,
		// the GpmlMeasure referenced by the return-value of this function cannot be
		// assigned-to, which means that this function does not provide a means to directly
		// switch the GpmlMeasure within this GpmlHotSpotTrailMark instance.  (This
		// restriction is intentional.)
		//
		// To switch the GpmlMeasure within this GpmlHotSpotTrailMark instance, use the
		// function @a set_trail_width below.
		//
		// (This overload is provided to allow the referenced GpmlMeasure instance to
		// accept a FeatureVisitor instance.)
		const boost::optional<GpmlMeasure::non_null_ptr_type>
		trail_width()
		{
			return d_trail_width;
		}

		void
		set_trail_width(
				GpmlMeasure::non_null_ptr_type tw)
		{
			d_trail_width = tw;
			update_instance_id();
		}

		const boost::optional<GmlTimeInstant::non_null_ptr_type>
		measured_age() const
		{
			return d_measured_age;
		}

		// Note that, because the copy-assignment operator of GmlTimeInstant is private, the
		// GmlTimeInstant referenced by the return-value of this function cannot be assigned-to,
		// which means that this function does not provide a means to directly switch the
		// GmlTimeInstant within this GpmlHotSpotTrailMark instance.  (This restriction is
		// intentional.)
		//
		// To switch the GmlTimeInstant within this GpmlHotSpotTrailMark instance, use the function
		// @a set_position below.
		//
		// (This overload is provided to allow the referenced GmlTimeInstant instance to accept a
		// FeatureVisitor instance.)
		const boost::optional<GmlTimeInstant::non_null_ptr_type>
		measured_age()
		{
			return d_measured_age;
		}

		void
		set_measured_age(
				GmlTimeInstant::non_null_ptr_type ti)
		{
			d_measured_age = ti;
			update_instance_id();
		}

		const boost::optional<GmlTimePeriod::non_null_ptr_type>
		measured_age_range() const
		{
			return d_measured_age_range;
		}

		const boost::optional<GmlTimePeriod::non_null_ptr_type>
		measured_age_range()
		{
			return d_measured_age_range;
		}

		void
		set_measured_age_range(
				GmlTimePeriod::non_null_ptr_type tp)
		{
			d_measured_age_range = tp;
			update_instance_id();
		}

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_gpml_hot_spot_trail_mark(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor)
		{
			visitor.visit_gpml_hot_spot_trail_mark(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		virtual
		bool
		directly_modifiable_fields_equal(
				const PropertyValue &other) const;

		GpmlHotSpotTrailMark(
				const GmlPoint::non_null_ptr_type &position_,
				const boost::optional<GpmlMeasure::non_null_ptr_type> &trail_width_,
				const boost::optional<GmlTimeInstant::non_null_ptr_type> &measured_age_,
				const boost::optional<GmlTimePeriod::non_null_ptr_type> &measured_age_range_) :
			PropertyValue(),
			d_position(position_),
			d_trail_width(trail_width_),
			d_measured_age(measured_age_),
			d_measured_age_range(measured_age_range_)
		{  }

		GpmlHotSpotTrailMark(
				const GpmlHotSpotTrailMark &other) :
			PropertyValue(other), /* share instance id */
			d_position(other.d_position),
			d_trail_width(other.d_trail_width),
			d_measured_age(other.d_measured_age),
			d_measured_age_range(other.d_measured_age_range)
		{  }

	private:

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlPlateId &
		operator=(const GpmlPlateId &);


		GmlPoint::non_null_ptr_type d_position;
		boost::optional<GpmlMeasure::non_null_ptr_type> d_trail_width;
		boost::optional<GmlTimeInstant::non_null_ptr_type> d_measured_age;
		boost::optional<GmlTimePeriod::non_null_ptr_type> d_measured_age_range;

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLHOTSPOTTRAILMARK_H
