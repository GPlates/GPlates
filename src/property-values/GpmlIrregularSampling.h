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

#include <iterator>
#include <vector>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "GpmlInterpolationFunction.h"
#include "GpmlTimeSample.h"
#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/PropertyValueRevisionContext.h"
#include "model/PropertyValueRevisionedReference.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlIrregularSampling, visit_gpml_irregular_sampling)

namespace GPlatesPropertyValues
{

	class GpmlIrregularSampling:
			public GPlatesModel::PropertyValue,
			public GPlatesModel::PropertyValueRevisionContext
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
				boost::optional<GpmlInterpolationFunction::non_null_ptr_type> interp_func,
				const StructuralType &value_type_)
		{
			return create(std::vector<GpmlTimeSample>(1, first_time_sample), interp_func, value_type_);
		}

		static
		const non_null_ptr_type
		create(
				const std::vector<GpmlTimeSample> &time_samples_,
				boost::optional<GpmlInterpolationFunction::non_null_ptr_type> interp_func,
				const StructuralType &value_type_);

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlIrregularSampling>(clone_impl());
		}

		/**
		 * Returns the time samples.
		 *
		 * To modify any time samples:
		 * (1) make additions/removals/modifications to a copy of the returned vector, and
		 * (2) use @a set_time_samples to set them.
		 *
		 * The returned time samples implement copy-on-write to promote resource sharing (until write)
		 * and to ensure our internal state cannot be modified and bypass the revisioning system.
		 */
		const std::vector<GpmlTimeSample> &
		get_time_samples() const
		{
			return get_current_revision<Revision>().time_samples;
		}

		/**
		 * Sets the internal time samples.
		 */
		void
		set_time_samples(
				const std::vector<GpmlTimeSample> &time_samples);


		/**
		 * Returns the 'const' interpolation function.
		 */
		const boost::optional<GpmlInterpolationFunction::non_null_ptr_to_const_type>
		interpolation_function() const;

		/**
		 * Returns the 'non-const' interpolation function.
		 */
		const boost::optional<GpmlInterpolationFunction::non_null_ptr_type>
		interpolation_function();

		/**
		 * Sets the internal interpolation function.
		 */
		void
		set_interpolation_function(
				boost::optional<GpmlInterpolationFunction::non_null_ptr_type> interpolation_function);


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
				GPlatesModel::ModelTransaction &transaction_,
				const std::vector<GpmlTimeSample> &time_samples,
				boost::optional<GpmlInterpolationFunction::non_null_ptr_type> interp_func,
				const StructuralType &value_type) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, time_samples, interp_func))),
			d_value_type(value_type)
		{  }

		//! Constructor used when cloning.
		GpmlIrregularSampling(
				const GpmlIrregularSampling &other_,
				boost::optional<PropertyValueRevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this))),
			d_value_type(other_.d_value_type)
		{  }

		bool
		contains_disabled_sequence_flag() const;

		virtual
		const PropertyValue::non_null_ptr_type
		clone_impl(
				boost::optional<PropertyValueRevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new GpmlIrregularSampling(*this, context));
		}

		virtual
		bool
		equality(
				const PropertyValue &other) const
		{
			const GpmlIrregularSampling &other_pv = dynamic_cast<const GpmlIrregularSampling &>(other);

			return d_value_type == other_pv.d_value_type &&
					// The revisioned data comparisons are handled here...
					PropertyValue::equality(other);
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
					const std::vector<GpmlTimeSample> &time_samples_,
					boost::optional<GpmlInterpolationFunction::non_null_ptr_type> interpolation_function_) :
				time_samples(time_samples_)
			{
				if (interpolation_function_)
				{
					interpolation_function = GPlatesModel::PropertyValueRevisionedReference<GpmlInterpolationFunction>::attach(
							transaction_, child_context_, interpolation_function_.get());
				}
			}

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<PropertyValueRevisionContext &> context_,
					PropertyValueRevisionContext &child_context_) :
				PropertyValueRevision(context_),
				time_samples(other_.time_samples),
				interpolation_function(other_.interpolation_function)
			{
				// Clone data members that were not deep copied.
				if (interpolation_function)
				{
					interpolation_function->clone(child_context_);
				}
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<PropertyValueRevisionContext &> context_) :
				PropertyValueRevision(context_),
				time_samples(other_.time_samples),
				interpolation_function(other_.interpolation_function)
			{  }

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

			std::vector<GpmlTimeSample> time_samples; // Internally GpmlTimeSample uses CopyOnWrite.
			boost::optional<GPlatesModel::PropertyValueRevisionedReference<GpmlInterpolationFunction> > interpolation_function;
		};

		// Immutable, so doesn't need revisioning.
		StructuralType d_value_type;

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLIRREGULARSAMPLING_H
