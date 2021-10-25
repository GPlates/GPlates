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
#include <boost/intrusive_ptr.hpp>

#include "GpmlFiniteRotation.h"
#include "GpmlInterpolationFunction.h"
#include "GpmlTimeSample.h"
#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/Metadata.h"


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
		non_null_ptr_type
		create(
				const GpmlTimeSample &first_time_sample,
				GpmlInterpolationFunction::maybe_null_ptr_type interp_func,
				const StructuralType &value_type_)
		{
			return non_null_ptr_type(
					new GpmlIrregularSampling(first_time_sample, interp_func,
							value_type_));
		}

		static
		non_null_ptr_type
		create(
				const std::vector<GpmlTimeSample> &time_samples_,
				GpmlInterpolationFunction::maybe_null_ptr_type interp_func,
				const StructuralType &value_type_)
		{
			return non_null_ptr_type(
					new GpmlIrregularSampling(time_samples_, interp_func, value_type_));
		}

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GpmlIrregularSampling(*this));
		}

		const non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the time sample vector?  (For consistency with the non-const
		// overload...)
		const std::vector<GpmlTimeSample> &
		time_samples() const
		{
			return d_time_samples;
		}

		/**
		 * Returns the 'non-const' vector of time samples.
		 */
		std::vector<GpmlTimeSample> &
		time_samples()
		{
			return d_time_samples;
		}


		/**
		 * Returns the 'const' interpolation function.
		 */
		const GpmlInterpolationFunction::maybe_null_ptr_to_const_type
		interpolation_function() const
		{
			return d_interpolation_function;
		}

		/**
		 * Returns the 'non-const' interpolation function.
		 */
		const GpmlInterpolationFunction::maybe_null_ptr_type

		interpolation_function()
		{
			return d_interpolation_function;
		}

		/**
		 * Sets the internal interpolation function.
		 */
		void
		set_interpolation_function(
				GpmlInterpolationFunction::maybe_null_ptr_type i)
		{
			d_interpolation_function = i;
			update_instance_id();
		}


		// Note that no "setter" is provided:  The value type of a GpmlIrregularSampling
		// instance should never be changed.
		const StructuralType &
		value_type() const
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

		bool
		is_disabled() const;

		void
		set_disabled(
				bool flag);

	protected:

		bool
		contain_disabled_sequence_flag() const ;

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlIrregularSampling(
				const GpmlTimeSample &first_time_sample,
				GpmlInterpolationFunction::maybe_null_ptr_type interp_func,
				const StructuralType &value_type_):
			PropertyValue(),
			d_time_samples(),
			d_interpolation_function(interp_func),
			d_value_type(value_type_)
		{
			d_time_samples.push_back(first_time_sample);
		}

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlIrregularSampling(
				const std::vector<GpmlTimeSample> &time_samples_,
				GpmlInterpolationFunction::maybe_null_ptr_type interp_func,
				const StructuralType &value_type_):
			PropertyValue(),
			d_time_samples(time_samples_),
			d_interpolation_function(interp_func),
			d_value_type(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlIrregularSampling(
				const GpmlIrregularSampling &other) :
			PropertyValue(other), /* share instance id */
			d_time_samples(other.d_time_samples),
			d_interpolation_function(other.d_interpolation_function),
			d_value_type(other.d_value_type)
		{  }

		virtual
		bool
		directly_modifiable_fields_equal(
				const PropertyValue &other) const;

	private:

		std::vector<GpmlTimeSample> d_time_samples;
		GpmlInterpolationFunction::maybe_null_ptr_type d_interpolation_function;
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
