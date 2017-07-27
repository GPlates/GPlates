/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "ReconstructionFeatureProperties.h"

#include "model/PropertyName.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/Enumeration.h"
#include "property-values/XsDouble.h"


bool
GPlatesAppLogic::ReconstructionFeatureProperties::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	d_recon_plate_id = boost::none;
	d_valid_time.time_of_appearance = boost::none;
	d_valid_time.time_of_disappearance = boost::none;

	d_recon_method = boost::none;
	d_right_plate_id = boost::none;
	d_left_plate_id = boost::none;
	d_spreading_asymmetry = boost::none;
	d_geometry_import_time = boost::none;

	return true;
}


void
GPlatesAppLogic::ReconstructionFeatureProperties::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	static const GPlatesModel::PropertyName geometry_import_time_property_name =
			GPlatesModel::PropertyName::create_gpml("geometryImportTime");

	// Note that we're going to assume that we're in a property...
	if (current_top_level_propname() == geometry_import_time_property_name)
	{
		// Cache the geometry import time.
		d_geometry_import_time = gml_time_instant.get_time_position();
	}
}


void
GPlatesAppLogic::ReconstructionFeatureProperties::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	// Note that we're going to assume that we're in a property...
	if (current_top_level_propname() == valid_time_property_name)
	{
		// Cache the time of appearance/disappearance.
		d_valid_time.time_of_appearance = gml_time_period.begin()->get_time_position();
		d_valid_time.time_of_disappearance = gml_time_period.end()->get_time_position();
	}
}


void
GPlatesAppLogic::ReconstructionFeatureProperties::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesAppLogic::ReconstructionFeatureProperties::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static GPlatesModel::PropertyName reconstruction_plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	static GPlatesModel::PropertyName right_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("rightPlate");

	static GPlatesModel::PropertyName left_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("leftPlate");

	// Note that we're going to assume that we're in a property...
	if (current_top_level_propname() == reconstruction_plate_id_property_name)
	{
		// This plate ID is the reconstruction plate ID.
		d_recon_plate_id = gpml_plate_id.get_value();
	}
	else if (current_top_level_propname() == right_plate_id_property_name)
	{
		d_right_plate_id = gpml_plate_id.get_value();
	}
	else if (current_top_level_propname() == left_plate_id_property_name)
	{
		d_left_plate_id = gpml_plate_id.get_value();
	}
}


void
GPlatesAppLogic::ReconstructionFeatureProperties::visit_xs_double(
		const GPlatesPropertyValues::XsDouble& xs_double)
{
	static GPlatesModel::PropertyName spreading_asymmetry_property_name =
			GPlatesModel::PropertyName::create_gpml("spreadingAsymmetry");

	// Note that we're going to assume that we're in a property...
	if (current_top_level_propname() == spreading_asymmetry_property_name)
	{
		d_spreading_asymmetry = xs_double.get_value();
	}
}


void
GPlatesAppLogic::ReconstructionFeatureProperties::visit_enumeration(
		const enumeration_type &enumeration)
{
	static GPlatesModel::PropertyName reconstruction_method_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionMethod");

	// Note that we're going to assume that we're in a property...
	if (current_top_level_propname() == reconstruction_method_name)
	{
		d_recon_method = enumeration.get_value();
	}
}


bool
GPlatesAppLogic::ReconstructionFeatureProperties::TimePeriod::is_valid_at_recon_time(
		const double &reconstruction_time) const
{
	if (time_of_appearance &&
		time_of_disappearance)
	{
		const GPlatesPropertyValues::GeoTimeInstant reconstruction_geo_time(reconstruction_time);

		return time_of_appearance->is_earlier_than_or_coincident_with(reconstruction_geo_time) &&
				reconstruction_geo_time.is_earlier_than_or_coincident_with(time_of_disappearance.get());
	}

	return true;
}
