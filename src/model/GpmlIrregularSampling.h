/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_GPMLIRREGULARSAMPLING_H
#define GPLATES_MODEL_GPMLIRREGULARSAMPLING_H

#include <vector>
#include "PropertyValue.h"
#include "GpmlTimeSample.h"
#include "GpmlInterpolationFunction.h"
#include "TemplateTypeParameterType.h"


namespace GPlatesModel {

	class GpmlIrregularSampling :
			public PropertyValue {

	public:

		virtual
		~GpmlIrregularSampling() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		boost::intrusive_ptr<GpmlIrregularSampling>
		create(
				const std::vector<GpmlTimeSample> &time_samples_,
				boost::intrusive_ptr<GpmlInterpolationFunction> interp_func,
				const TemplateTypeParameterType &value_type_) {
			boost::intrusive_ptr<GpmlIrregularSampling> ptr(new GpmlIrregularSampling(time_samples_, interp_func, value_type_));
			return ptr;
		}

		virtual
		boost::intrusive_ptr<PropertyValue>
		clone() const {
			boost::intrusive_ptr<PropertyValue> dup(new GpmlIrregularSampling(*this));
			return dup;
		}

		const std::vector<GpmlTimeSample> &
		time_samples() const {
			return d_time_samples;
		}

		std::vector<GpmlTimeSample> &
		time_samples() {
			return d_time_samples;
		}

		boost::intrusive_ptr<const GpmlInterpolationFunction>
		interpolation_function() const {
			return d_interpolation_function;
		}

		boost::intrusive_ptr<GpmlInterpolationFunction>
		interpolation_function() {
			return d_interpolation_function;
		}

		const TemplateTypeParameterType &
		value_type() const {
			return d_value_type;
		}

		TemplateTypeParameterType &
		value_type() {
			return d_value_type;
		}

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_gpml_irregular_sampling(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlIrregularSampling(
				const std::vector<GpmlTimeSample> &time_samples_,
				boost::intrusive_ptr<GpmlInterpolationFunction> interp_func,
				const TemplateTypeParameterType &value_type_):
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
			PropertyValue(),
			d_time_samples(other.d_time_samples),
			d_interpolation_function(other.d_interpolation_function),
			d_value_type(other.d_value_type)
		{  }

	private:

		std::vector<GpmlTimeSample> d_time_samples;
		// FIXME:  Is it valid for this pointer to be NULL?  I don't think so...
		boost::intrusive_ptr<GpmlInterpolationFunction> d_interpolation_function;
		TemplateTypeParameterType d_value_type;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlIrregularSampling &
		operator=(const GpmlIrregularSampling &);

	};

}

#endif  // GPLATES_MODEL_GPMLIRREGULARSAMPLING_H
