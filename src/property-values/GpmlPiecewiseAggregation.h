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

#ifndef GPLATES_PROPERTYVALUES_GPMLPIECEWISEAGGREGATION_H
#define GPLATES_PROPERTYVALUES_GPMLPIECEWISEAGGREGATION_H

#include <vector>
#include <boost/intrusive_ptr.hpp>

#include "GpmlTimeWindow.h"
#include "StructuralType.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlPiecewiseAggregation, visit_gpml_piecewise_aggregation)

namespace GPlatesPropertyValues
{

	class GpmlPiecewiseAggregation:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlPiecewiseAggregation>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlPiecewiseAggregation> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlPiecewiseAggregation>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlPiecewiseAggregation> non_null_ptr_to_const_type;


		virtual
		~GpmlPiecewiseAggregation()
		{  }

		static
		const non_null_ptr_type
		create(
				const std::vector<GpmlTimeWindow> &time_windows_,
				const StructuralType &value_type_)
		{
			return non_null_ptr_type(new GpmlPiecewiseAggregation(time_windows_, value_type_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlPiecewiseAggregation>(clone_impl());
		}

		/**
		 * Returns the time windows.
		 *
		 * The returned time windows are clones of the internal revisioned state so that the
		 * internal state cannot be modified and bypass the revisioning system.
		 * Just returning 'const' GpmlTimeWindow references is not sufficient protection.
		 *
		 * To modify any time windows:
		 * (1) make additions/removals/modifications to the returned vector, and
		 * (2) use @a set_time_windows to set them.
		 */
		std::vector<GpmlTimeWindow>
		get_time_windows() const;

		/**
		 * Sets the internal time windows to clones of those in @a time_windows.
		 */
		void
		set_time_windows(
				const std::vector<GpmlTimeWindow> &time_windows);

		// Note that no "setter" is provided:  The value type of a GpmlPiecewiseAggregation
		// instance should never be changed.
		const StructuralType &
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
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("PiecewiseAggregation");
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
			visitor.visit_gpml_piecewise_aggregation(*this);
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
			visitor.visit_gpml_piecewise_aggregation(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlPiecewiseAggregation(
				const std::vector<GpmlTimeWindow> &time_windows_,
				const StructuralType &value_type_):
			PropertyValue(Revision::non_null_ptr_type(new Revision(time_windows_))),
			d_value_type(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlPiecewiseAggregation(
				const GpmlPiecewiseAggregation &other) :
			PropertyValue(other),
			d_value_type(other.d_value_type)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlPiecewiseAggregation(*this));
		}

		virtual
		bool
		equality(
				const PropertyValue &other) const
		{
			const GpmlPiecewiseAggregation &other_pv = dynamic_cast<const GpmlPiecewiseAggregation &>(other);

			return d_value_type == other_pv.d_value_type &&
					// The revisioned data comparisons are handled here...
					GPlatesModel::PropertyValue::equality(other);
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			explicit
			Revision(
					const std::vector<GpmlTimeWindow> &time_windows_,
					bool deep_copy = true)
			{
				if (deep_copy)
				{
					set_cloned_time_windows(time_windows_);
				}
				else
				{
					time_windows = time_windows_;
				}
			}

			Revision(
					const Revision &other);

			// To keep our revision state immutable we clone the time windows so that the client
			// can no longer modify them indirectly...
			void
			set_cloned_time_windows(
					const std::vector<GpmlTimeWindow> &time_windows_);

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone_for_bubble_up_modification() const
			{
				// Don't clone the property values in the time samples and
				// don't clone the interpolation function property value.
				return non_null_ptr_type(new Revision(time_windows, false/*deep_copy*/));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const;

			std::vector<GpmlTimeWindow> time_windows;
		};


		// Immutable, so doesn't need revisioning.
		StructuralType d_value_type;


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlPiecewiseAggregation &
		operator=(const GpmlPiecewiseAggregation &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLPIECEWISEAGGREGATION_H
