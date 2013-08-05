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

#include "GmlPoint.h"
#include "GmlTimeInstant.h"
#include "GmlTimePeriod.h"
#include "GpmlMeasure.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"
#include "utils/CopyOnWrite.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
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
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlHotSpotTrailMark>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlHotSpotTrailMark> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlHotSpotTrailMark>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlHotSpotTrailMark> non_null_ptr_to_const_type;


		virtual
		~GpmlHotSpotTrailMark()
		{  }

		static
		const non_null_ptr_type
		create(
				const GmlPoint::non_null_ptr_to_const_type &position_,
				const boost::optional<GpmlMeasure::non_null_ptr_to_const_type> &trail_width_,
				const boost::optional<GmlTimeInstant::non_null_ptr_to_const_type> &measured_age_,
				const boost::optional<GmlTimePeriod::non_null_ptr_to_const_type> &measured_age_range_)
		{
			return non_null_ptr_type(
					new GpmlHotSpotTrailMark(position_, trail_width_, measured_age_, measured_age_range_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlHotSpotTrailMark>(clone_impl());
		}

		/**
		 * Returns the 'const' position - which is 'const' so that it cannot be
		 * modified and bypass the revisioning system.
		 */
		const GmlPoint::non_null_ptr_to_const_type
		get_position() const;

		/**
		 * Sets the internal position to a clone of @a pos.
		 */
		void
		set_position(
				GmlPoint::non_null_ptr_to_const_type pos);

		/**
		 * Returns the 'const' trail width - which is 'const' so that it cannot be
		 * modified and bypass the revisioning system.
		 */
		const boost::optional<GpmlMeasure::non_null_ptr_to_const_type>
		get_trail_width() const;

		/**
		 * Sets the internal trail width to a clone of @a tw.
		 */
		void
		set_trail_width(
				GpmlMeasure::non_null_ptr_to_const_type tw);

		/**
		 * Returns the 'const' measured age - which is 'const' so that it cannot be
		 * modified and bypass the revisioning system.
		 */
		const boost::optional<GmlTimeInstant::non_null_ptr_to_const_type>
		get_measured_age() const;

		/**
		 * Sets the internal measured age to a clone of @a ti.
		 */
		void
		set_measured_age(
				GmlTimeInstant::non_null_ptr_to_const_type ti);

		/**
		 * Returns the 'const' measured age range - which is 'const' so that it cannot be
		 * modified and bypass the revisioning system.
		 */
		const boost::optional<GmlTimePeriod::non_null_ptr_to_const_type>
		get_measured_age_range() const;

		/**
		 * Sets the internal measured age range to a clone of @a tp.
		 */
		void
		set_measured_age_range(
				GmlTimePeriod::non_null_ptr_to_const_type tp);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("HotSpotTrailMark");
			return STRUCTURAL_TYPE;
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

		GpmlHotSpotTrailMark(
				const GmlPoint::non_null_ptr_to_const_type &position_,
				const boost::optional<GpmlMeasure::non_null_ptr_to_const_type> &trail_width_,
				const boost::optional<GmlTimeInstant::non_null_ptr_to_const_type> &measured_age_,
				const boost::optional<GmlTimePeriod::non_null_ptr_to_const_type> &measured_age_range_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(position_, trail_width_, measured_age_, measured_age_range_)))
		{  }

		GpmlHotSpotTrailMark(
				const GpmlHotSpotTrailMark &other) :
			PropertyValue(other)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlHotSpotTrailMark(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			Revision(
					const GmlPoint::non_null_ptr_to_const_type &position_,
					const boost::optional<GpmlMeasure::non_null_ptr_to_const_type> &trail_width_,
					const boost::optional<GmlTimeInstant::non_null_ptr_to_const_type> &measured_age_,
					const boost::optional<GmlTimePeriod::non_null_ptr_to_const_type> &measured_age_range_) :
				position(position_),
				trail_width(trail_width_),
				measured_age(measured_age_),
				measured_age_range(measured_age_range_)
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
					const GPlatesModel::PropertyValue::Revision &other) const;

			GPlatesUtils::CopyOnWrite<GmlPoint::non_null_ptr_to_const_type> position;
			boost::optional<GPlatesUtils::CopyOnWrite<GpmlMeasure::non_null_ptr_to_const_type> > trail_width;
			boost::optional<GPlatesUtils::CopyOnWrite<GmlTimeInstant::non_null_ptr_to_const_type> > measured_age;
			boost::optional<GPlatesUtils::CopyOnWrite<GmlTimePeriod::non_null_ptr_to_const_type> > measured_age_range;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLHOTSPOTTRAILMARK_H
