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

#ifndef GPLATES_PROPERTYVALUES_GPMLIRREGULARSAMPLING_H
#define GPLATES_PROPERTYVALUES_GPMLIRREGULARSAMPLING_H

#include <vector>
#include <boost/foreach.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility/compare_pointees.hpp>

#include "GpmlInterpolationFunction.h"
#include "GpmlTimeSample.h"
#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlIrregularSampling, visit_gpml_irregular_sampling)

namespace GPlatesPropertyValues
{

	class GpmlIrregularSampling:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlIrregularSampling>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlIrregularSampling> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlIrregularSampling>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlIrregularSampling> non_null_ptr_to_const_type;


		virtual
		~GpmlIrregularSampling()
		{  }

		static
		const non_null_ptr_type
		create(
				const GpmlTimeSample &first_time_sample,
				GpmlInterpolationFunction::maybe_null_ptr_type interp_func,
				const StructuralType &value_type_)
		{
			return non_null_ptr_type(new GpmlIrregularSampling(first_time_sample, interp_func, value_type_));
		}

		static
		const non_null_ptr_type
		create(
				const std::vector<GpmlTimeSample> &time_samples_,
				GpmlInterpolationFunction::maybe_null_ptr_type interp_func,
				const StructuralType &value_type_)
		{
			return non_null_ptr_type(new GpmlIrregularSampling(time_samples_, interp_func, value_type_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlIrregularSampling>(clone_impl());
		}

		/**
		 * Returns the 'const' time samples.
		 *
		 * To modify any time samples:
		 * (1) make a copy of the returned vector,
		 * (2) make additions/removals/modifications, and
		 * (3) use @a set_time_samples to set them.
		 */
		const std::vector<GpmlTimeSample> &
		get_time_samples() const
		{
			return get_current_revision<Revision>().time_samples;
		}

		void
		set_time_samples(
				const std::vector<GpmlTimeSample> &time_samples);

		const GpmlInterpolationFunction::maybe_null_ptr_to_const_type
		get_interpolation_function() const
		{
			return get_current_revision<Revision>().interpolation_function;
		}

		// Note that, because the copy-assignment operator of GpmlInterpolationFunction is
		// private, the GpmlInterpolationFunction referenced by the return-value of this
		// function cannot be assigned-to, which means that this function does not provide
		// a means to directly switch the GpmlInterpolationFunction within this
		// GpmlIrregularSampling instance.  (This restriction is intentional.)
		// However the property value contained within can be modified, and this causes no
		// revisioning problems because all property values take care of their own revisioning.
		//
		// To switch the GpmlInterpolationFunction within this GpmlIrregularSampling
		// instance, use the function @a set_interpolation_function below.
		//
		// (This overload is provided to allow the referenced GpmlInterpolationFunction
		// instance to accept a FeatureVisitor instance.)
		const GpmlInterpolationFunction::maybe_null_ptr_type
		get_interpolation_function()
		{
			return get_current_revision<Revision>().interpolation_function;
		}

		void
		set_interpolation_function(
				GpmlInterpolationFunction::maybe_null_ptr_type interpolation_function);

		// Note that no "setter" is provided:  The value type of a GpmlIrregularSampling
		// instance should never be changed.
		const StructuralType &
		get_value_type() const
		{
			return d_value_type;
		}

		bool
		is_disabled() const;

		void
		set_disabled(
				bool flag);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("IrregularSampling");
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
			visitor.visit_gpml_irregular_sampling(*this);
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
			visitor.visit_gpml_irregular_sampling(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlIrregularSampling(
				const GpmlTimeSample &first_time_sample,
				GpmlInterpolationFunction::maybe_null_ptr_type interp_func,
				const StructuralType &value_type) :
			PropertyValue(
					new Revision(
							std::vector<GpmlTimeSample>(1, first_time_sample),
							interp_func)),
			d_value_type(value_type)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlIrregularSampling(
				const std::vector<GpmlTimeSample> &time_samples,
				GpmlInterpolationFunction::maybe_null_ptr_type interp_func,
				const StructuralType &value_type) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(time_samples, interp_func))),
			d_value_type(value_type)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlIrregularSampling(
				const GpmlIrregularSampling &other) :
			PropertyValue(other),
			d_value_type(other.d_value_type)
		{  }

		bool
		contains_disabled_sequence_flag() const;

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlIrregularSampling(*this));
		}

		virtual
		bool
		equality(
				const PropertyValue &other) const
		{
			const GpmlIrregularSampling &other_pv = static_cast<const GpmlIrregularSampling &>(other);

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
			Revision(
					const std::vector<GpmlTimeSample> &time_samples_,
					const GpmlInterpolationFunction::maybe_null_ptr_type &interpolation_function_) :
				// Shallow copy of time samples...
				time_samples(time_samples_),
				// Shallow copy...
				interpolation_function(interpolation_function_)
			{  }

			Revision(
					const Revision &other);

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone_with_shared_nested_property_values() const
			{
				// Don't clone the property values in the time samples and
				// don't clone the interpolation function property value.
				// This can be achieved using our regular constructor which does shallow copying.
				return non_null_ptr_type(new Revision(time_samples, interpolation_function));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const
			{
				const Revision &other_revision = static_cast<const Revision &>(other);

				return time_samples == other_revision.time_samples &&
						boost::equal_pointees(interpolation_function, other_revision.interpolation_function) &&
						GPlatesModel::PropertyValue::Revision::equality(other);
			}

			std::vector<GpmlTimeSample> time_samples;
			GpmlInterpolationFunction::maybe_null_ptr_type interpolation_function;
		};


		// Immutable, so doesn't need revisioning.
		StructuralType d_value_type;


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlIrregularSampling &
		operator=(const GpmlIrregularSampling &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLIRREGULARSAMPLING_H
