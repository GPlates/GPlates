/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include <boost/foreach.hpp>

#include "ReconstructionGraphPopulator.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"


namespace
{
	/**
	 * Used to determine if @a ReconstructionGraphPopulator can reconstruct a feature.
	 */
	class IsReconstructionFeature :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		IsReconstructionFeature() :
			d_is_reconstruction_feature(false),
			d_has_finite_rotation(false),
			d_has_fixed_reference_frame(false),
			d_has_moving_reference_frame(false)
		{  }

		//! Returns true any features visited by us are reconstruction features.
		bool
		is_reconstruction_feature()
		{
			return d_is_reconstruction_feature;
		}

	private:
		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			d_has_finite_rotation = false;
			d_has_fixed_reference_frame = false;
			d_has_moving_reference_frame = false;

			return true;
		}

		virtual
		void
		finalise_post_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			if (d_has_finite_rotation && d_has_moving_reference_frame && d_has_fixed_reference_frame)
			{
				d_is_reconstruction_feature = true;
			}
		}

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
		{
			BOOST_FOREACH(
					const GPlatesPropertyValues::GpmlTimeSample &time_sample,
					gpml_irregular_sampling.time_samples())
			{
				time_sample.value()->accept_visitor(*this);
			}
		}

		virtual
		void
		visit_gpml_finite_rotation(
				const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation)
		{
			d_has_finite_rotation = true;
		}

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
		{
			static const GPlatesModel::PropertyName fixed_ref_frame_property_name =
				GPlatesModel::PropertyName::create_gpml("fixedReferenceFrame");
			static const GPlatesModel::PropertyName moving_ref_frame_property_name =
				GPlatesModel::PropertyName::create_gpml("movingReferenceFrame");

			if (*current_top_level_propname() == fixed_ref_frame_property_name) {
				// We're dealing with the fixed ref-frame of the Total Reconstruction Sequence.
				d_has_fixed_reference_frame = true;
			} else if (*current_top_level_propname() == moving_ref_frame_property_name) {
				// We're dealing with the moving ref-frame of the Total Reconstruction Sequence.
				d_has_moving_reference_frame = true;
			}
		}


		bool d_is_reconstruction_feature;

		bool d_has_finite_rotation;
		bool d_has_fixed_reference_frame;
		bool d_has_moving_reference_frame;
	};
}


bool
GPlatesAppLogic::ReconstructionGraphPopulator::can_process(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	IsReconstructionFeature is_reconstruction_visitor;

	is_reconstruction_visitor.visit_feature(feature_ref);

	return is_reconstruction_visitor.is_reconstruction_feature();
}


GPlatesAppLogic::ReconstructionGraphPopulator::ReconstructionGraphPopulator(
		ReconstructionGraphBuilder &graph_builder) :
	d_graph_builder(graph_builder)
{  }


bool
GPlatesAppLogic::ReconstructionGraphPopulator::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	d_accumulator.reset();

	return true;
}


void
GPlatesAppLogic::ReconstructionGraphPopulator::finalise_post_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	// So now we've visited the contents of this Total Recon Seq feature.  Let's find out if we
	// were able to obtain all the information we need.
	if (!d_accumulator.d_fixed_ref_frame)
	{
		// We couldn't obtain the fixed ref-frame.
		d_accumulator.reset();
		return;
	}

	if (!d_accumulator.d_moving_ref_frame)
	{
		// We couldn't obtain the moving ref-frame.
		d_accumulator.reset();
		return;
	}

	// We need at least two enabled time samples in the total reconstruction sequence in order
	// to have a meaningful sequence (ie, something that's valid at times other than present day).
	if (d_accumulator.d_total_reconstruction_pole.size() < 2)
	{
		// We couldn't obtain the total reconstruction pole.
		d_accumulator.reset();
		return;
	}

	// If we got to here, we have all the information we need.
	d_graph_builder.insert_total_reconstruction_sequence(
			d_accumulator.d_fixed_ref_frame.get(),
			d_accumulator.d_moving_ref_frame.get(),
			d_accumulator.d_total_reconstruction_pole);

	d_accumulator.reset();
}


void
GPlatesAppLogic::ReconstructionGraphPopulator::visit_gpml_finite_rotation(
		GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation)
{
	if (d_accumulator.d_is_expecting_a_finite_rotation)
	{
		// The visitor was expecting a FiniteRotation, which means the structure of the
		// Total Reconstruction Sequence is (more or less) correct.
		const ReconstructionGraphBuilder::total_reconstruction_pole_time_sample_type pole_sample(
				d_accumulator.d_finite_rotation_time_instant.get(),
				gpml_finite_rotation.finite_rotation());

		// Add the current pole sample to the sequence.
		d_accumulator.d_total_reconstruction_pole.push_back(pole_sample);

		d_accumulator.d_finite_rotation_time_instant = boost::none;
		d_accumulator.d_is_expecting_a_finite_rotation = false;
	}
}


void
GPlatesAppLogic::ReconstructionGraphPopulator::visit_gpml_irregular_sampling(
		GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
	// It is assumed that an IrregularSampling instance which has been reached by the visit function
	// of a ReconstructionGraphPopulator instance will only ever contain FiniteRotation instances.
	// And it is assumed that an IrregularSampling contains at least two enabled time samples in order
	// to have a meaningful sequence (ie, something that's valid at times other than present day).
	// If it doesn't then we won't end up with a valid total reconstruction sequence and it won't
	// get inserted into the reconstruction graph builder.

	// Iterate over the time samples and collect the enabled ones.
	std::vector<GPlatesPropertyValues::GpmlTimeSample>::iterator time_sample_iter =
			gpml_irregular_sampling.time_samples().begin();
	std::vector<GPlatesPropertyValues::GpmlTimeSample>::iterator time_sample_end =
			gpml_irregular_sampling.time_samples().end();
	for ( ; time_sample_iter != time_sample_end; ++time_sample_iter)
	{
		if (time_sample_iter->is_disabled())
		{
			// This time-sample is disabled.  Let's move to the next one.
			continue;
		}

		// Let's visit the time sample, to collect (what we expect to be) the FiniteRotation inside it.
		d_accumulator.d_finite_rotation_time_instant = time_sample_iter->valid_time()->time_position();
		d_accumulator.d_is_expecting_a_finite_rotation = true;
		time_sample_iter->value()->accept_visitor(*this);
	}
}


void
GPlatesAppLogic::ReconstructionGraphPopulator::visit_gpml_plate_id(
		GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static const GPlatesModel::PropertyName fixed_ref_frame_property_name =
		GPlatesModel::PropertyName::create_gpml("fixedReferenceFrame");
	static const GPlatesModel::PropertyName moving_ref_frame_property_name =
		GPlatesModel::PropertyName::create_gpml("movingReferenceFrame");

	// Note that we're going to assume that we've read a property name...
	if (*current_top_level_propname() == fixed_ref_frame_property_name)
	{
		// We're dealing with the fixed ref-frame of the Total Reconstruction Sequence.
		d_accumulator.d_fixed_ref_frame = gpml_plate_id.value();
	}
	else if (*current_top_level_propname() == moving_ref_frame_property_name)
	{
		// We're dealing with the moving ref-frame of the Total Reconstruction Sequence.
		d_accumulator.d_moving_ref_frame = gpml_plate_id.value();
	}
}

