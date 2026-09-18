/* $Id$ */

/**
 * \file 
 * Contains a collection of functors that extract properties from ReconstructionGeometry.
 *
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "PropertyExtractors.h"

#include "ApplicationState.h"

#include "model/PropertyName.h"
#include "model/PropertyValueFinder.h"

#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"


const boost::optional<GPlatesAppLogic::PlateIdPropertyExtractor::return_type>
GPlatesAppLogic::PlateIdPropertyExtractor::operator()(
		const GPlatesModel::FeatureHandle& feature) const
{
	static const GPlatesModel::PropertyName GPML_RECONSTRUCTION_PLATE_ID =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> gpml_plate_id =
			GPlatesModel::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
					feature.reference(),
					GPML_RECONSTRUCTION_PLATE_ID);
	if (!gpml_plate_id)
	{
		return boost::none;
	}

	return gpml_plate_id.get()->get_value();
}


const boost::optional<GPlatesAppLogic::AgePropertyExtractor::return_type>
GPlatesAppLogic::AgePropertyExtractor::operator()(
		const GPlatesModel::FeatureHandle& feature) const
{
	static const GPlatesModel::PropertyName GML_VALID_TIME =
			GPlatesModel::PropertyName::create_gml("validTime");
	boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> gml_valid_time =
			GPlatesModel::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
					feature.reference(),
					GML_VALID_TIME);
	if (!gml_valid_time)
	{
		return boost::none;
	}

	// The age is measured from the feature's time of formation (the begin of its valid time).
	const GPlatesPropertyValues::GeoTimeInstant time_of_formation =
			gml_valid_time.get()->begin()->get_time_position();
	if (time_of_formation.is_distant_past())
	{
		return GPlatesMaths::Real::positive_infinity();
	}
	else if (time_of_formation.is_distant_future())
	{
		return GPlatesMaths::Real::negative_infinity();
	}
	else
	{
		return GPlatesMaths::Real(
				time_of_formation.value() - d_application_state.get_current_reconstruction_time());
	}
}


const boost::optional<GPlatesAppLogic::FeatureTypePropertyExtractor::return_type>
GPlatesAppLogic::FeatureTypePropertyExtractor::operator()(
		const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const
{
	const boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
			ReconstructionGeometryUtils::get_feature_ref(&reconstruction_geometry);
	if (!feature_ref)
	{
		return boost::none;
	}
	else
	{
		return feature_ref.get()->feature_type();
	}
}

