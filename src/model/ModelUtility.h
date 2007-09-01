/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_MODELUTILITY_H
#define GPLATES_MODEL_MODELUTILITY_H

#include "Model.h"
#include "InlinePropertyContainer.h"
#include "PropertyValue.h"
#include "GmlTimeInstant.h"
#include "GpmlIrregularSampling.h"
#include "XsString.h"
#include "XsBoolean.h"
#include "GeoTimeInstant.h"
#include "StrikeSlipEnumeration.h"

namespace GPlatesModel
{
	namespace ModelUtility
	{
		struct TotalReconstructionPoleData
		{
			double time;
			double lat_of_euler_pole;
			double lon_of_euler_pole;
			double rotation_angle;
			const char *comment;
		};


		const InlinePropertyContainer::non_null_ptr_type
		append_property_value_to_feature(
				PropertyValue::non_null_ptr_type property_value,
				const char *property_name_string,
				FeatureHandle::weak_ref &feature);


		const GmlTimeInstant::non_null_ptr_type
		create_gml_time_instant(
				const GeoTimeInstant &geo_time_instant);


		const GpmlIrregularSampling::non_null_ptr_type
		create_gpml_irregular_sampling(
				const GpmlTimeSample &first_time_sample);


		const XsString::non_null_ptr_type
		create_xs_string(
				const std::string &str);


		const XsBoolean::non_null_ptr_type
		create_xs_boolean(
				bool value);

		const StrikeSlipEnumeration::non_null_ptr_type
		create_strike_slip_enumeration(
				const std::string &value);

		// Before this line are the new, hopefully-better-designed functions; after this
		// line are the old, arbitrary functions which should probably be reviewed (and
		// should quite possibly be refactored).
		// FIXME:  Review the following functions and refactor if necessary.

		const PropertyContainer::non_null_ptr_type
		create_reconstruction_plate_id(
				unsigned long plate_id);
	
		const PropertyContainer::non_null_ptr_type
		create_reference_frame_plate_id(
				unsigned long plate_id,
				const char *which_reference_frame);
	
		const PropertyContainer::non_null_ptr_type
		create_centre_line_of(
				const std::vector<double> &points);	

		const PropertyContainer::non_null_ptr_type
		create_valid_time(
				const GeoTimeInstant &geo_time_instant_begin,
				const GeoTimeInstant &geo_time_instant_end);
		
		const PropertyContainer::non_null_ptr_type
		create_name(
				const UnicodeString &name,
				const UnicodeString &codespace);
	
		const PropertyContainer::non_null_ptr_type
		create_total_reconstruction_pole(
				const std::vector<TotalReconstructionPoleData> &five_tuples);
	
		const FeatureHandle::weak_ref
		create_total_recon_seq(
				ModelInterface &model,
				FeatureCollectionHandle::weak_ref &target_collection,
				unsigned long fixed_plate_id,
				unsigned long moving_plate_id,
				const std::vector<TotalReconstructionPoleData> &five_tuples);
	}
}

#endif  // GPLATES_MODEL_MODELUTILITY_H
