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
#include "model/PropertyValueRevisionContext.h"
#include "model/PropertyValueRevisionedReference.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlHotSpotTrailMark, visit_gpml_hot_spot_trail_mark)

namespace GPlatesPropertyValues
{

	class GpmlHotSpotTrailMark:
			public GPlatesModel::PropertyValue,
			public GPlatesModel::PropertyValueRevisionContext
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
				const GmlPoint::non_null_ptr_type &position_,
				const boost::optional<GpmlMeasure::non_null_ptr_type> &trail_width_,
				const boost::optional<GmlTimeInstant::non_null_ptr_type> &measured_age_,
				const boost::optional<GmlTimePeriod::non_null_ptr_type> &measured_age_range_);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlHotSpotTrailMark>(clone_impl());
		}

		/**
		 * Returns the 'const' position.
		 */
		const GmlPoint::non_null_ptr_to_const_type
		position() const
		{
			return get_current_revision<Revision>().position.get_property_value();
		}

		/**
		 * Returns the 'non-const' position.
		 */
		const GmlPoint::non_null_ptr_type
		position()
		{
			return get_current_revision<Revision>().position.get_property_value();
		}

		/**
		 * Sets the internal position.
		 */
		void
		set_position(
				GmlPoint::non_null_ptr_type pos);

		/**
		 * Returns the 'const' trail width.
		 */
		const boost::optional<GpmlMeasure::non_null_ptr_to_const_type>
		trail_width() const;

		/**
		 * Returns the 'non-const' trail width.
		 */
		const boost::optional<GpmlMeasure::non_null_ptr_type>
		trail_width();

		/**
		 * Sets the internal trail width.
		 */
		void
		set_trail_width(
				GpmlMeasure::non_null_ptr_type tw);

		/**
		 * Returns the 'const' measured age.
		 */
		const boost::optional<GmlTimeInstant::non_null_ptr_to_const_type>
		measured_age() const;

		/**
		 * Returns the 'non-const' measured age.
		 */
		const boost::optional<GmlTimeInstant::non_null_ptr_type>
		measured_age();

		/**
		 * Sets the internal measured age.
		 */
		void
		set_measured_age(
				GmlTimeInstant::non_null_ptr_type ti);

		/**
		 * Returns the 'const' measured age range.
		 */
		const boost::optional<GmlTimePeriod::non_null_ptr_to_const_type>
		measured_age_range() const;

		/**
		 * Returns the 'non-const' measured age range.
		 */
		const boost::optional<GmlTimePeriod::non_null_ptr_type>
		measured_age_range();

		/**
		 * Sets the internal measured age range.
		 */
		void
		set_measured_age_range(
				GmlTimePeriod::non_null_ptr_type tp);

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
				GPlatesModel::ModelTransaction &transaction_,
				const GmlPoint::non_null_ptr_type &position_,
				const boost::optional<GpmlMeasure::non_null_ptr_type> &trail_width_,
				const boost::optional<GmlTimeInstant::non_null_ptr_type> &measured_age_,
				const boost::optional<GmlTimePeriod::non_null_ptr_type> &measured_age_range_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, position_, trail_width_, measured_age_, measured_age_range_)))
		{  }

		//! Constructor used when cloning.
		GpmlHotSpotTrailMark(
				const GpmlHotSpotTrailMark &other_,
				boost::optional<PropertyValueRevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const PropertyValue::non_null_ptr_type
		clone_impl(
				boost::optional<PropertyValueRevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlHotSpotTrailMark(*this, context));
		}

	private:

		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a PropertyValueRevisionContext.
		 */
		virtual
		GPlatesModel::PropertyValueRevision::non_null_ptr_type
		bubble_up(
				GPlatesModel::ModelTransaction &transaction,
				const PropertyValue::non_null_ptr_to_const_type &child_property_value);

		/**
		 * Inherited from @a PropertyValueRevisionContext.
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
				public GPlatesModel::PropertyValueRevision
		{
			Revision(
					GPlatesModel::ModelTransaction &transaction_,
					PropertyValueRevisionContext &child_context_,
					const GmlPoint::non_null_ptr_type &position_,
					const boost::optional<GpmlMeasure::non_null_ptr_type> &trail_width_,
					const boost::optional<GmlTimeInstant::non_null_ptr_type> &measured_age_,
					const boost::optional<GmlTimePeriod::non_null_ptr_type> &measured_age_range_);

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<PropertyValueRevisionContext &> context_,
					PropertyValueRevisionContext &child_context_);

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<PropertyValueRevisionContext &> context_);

			virtual
			PropertyValueRevision::non_null_ptr_type
			clone_revision(
					boost::optional<PropertyValueRevisionContext &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const PropertyValueRevision &other) const;

			GPlatesModel::PropertyValueRevisionedReference<GmlPoint> position;
			boost::optional<GPlatesModel::PropertyValueRevisionedReference<GpmlMeasure> > trail_width;
			boost::optional<GPlatesModel::PropertyValueRevisionedReference<GmlTimeInstant> > measured_age;
			boost::optional<GPlatesModel::PropertyValueRevisionedReference<GmlTimePeriod> > measured_age_range;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLHOTSPOTTRAILMARK_H
