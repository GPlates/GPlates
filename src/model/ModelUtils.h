/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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

#ifndef GPLATES_MODEL_MODELUTILS_H
#define GPLATES_MODEL_MODELUTILS_H

#include "FeatureHandle.h"
#include "FeatureCollectionHandle.h"
#include "ModelInterface.h"
#include "PropertyName.h"
#include "PropertyValue.h"
#include "TopLevelPropertyInline.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/TemplateTypeParameterType.h"

#include "app-logic/FeatureCollectionFileState.h"

#include "global/InvalidFeatureCollectionException.h"
#include "global/InvalidParametersException.h"


namespace GPlatesModel
{
	namespace ModelUtils
	{
		// Note: Consider adding functions as member functions in one of the Handle classes instead.

		struct TotalReconstructionPoleData
		{
			double time;
			double lat_of_euler_pole;
			double lon_of_euler_pole;
			double rotation_angle;
			const char *comment;
		};


		const GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type
		create_gml_orientable_curve(
				const GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string,
				bool reverse_orientation = false);


		const GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type
		create_gml_time_period(
				const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_begin,
				const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant_end);


		const GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type
		create_gml_time_instant(
				const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant);


		const GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type
		create_gpml_constant_value(
				const PropertyValue::non_null_ptr_type property_value,
				const GPlatesPropertyValues::TemplateTypeParameterType &template_type_parameter_type);


		// Before this line are the new, hopefully-better-designed functions; after this
		// line are the old, arbitrary functions which should probably be reviewed (and
		// should quite possibly be refactored).
		// FIXME:  Review the following functions and refactor if necessary.

		const TopLevelProperty::non_null_ptr_type
		create_total_reconstruction_pole(
				const std::vector<TotalReconstructionPoleData> &five_tuples);
	
		const FeatureHandle::weak_ref
		create_total_recon_seq(
				ModelInterface &model,
				const FeatureCollectionHandle::weak_ref &target_collection,
				unsigned long fixed_plate_id,
				unsigned long moving_plate_id,
				const std::vector<TotalReconstructionPoleData> &five_tuples);

		
		bool
		remove_feature(
				GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref,
				GPlatesModel::FeatureHandle::weak_ref feature_ref);
	}

}

#endif  // GPLATES_MODEL_MODELUTILS_H
