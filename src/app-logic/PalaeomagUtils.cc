/* $Id: PalaeomagUtils.cc 9024 2010-07-30 10:47:35Z elau $ */


/**
 * \file 
 * $Revision: 9024 $
 * $Date: 2010-07-30 12:47:35 +0200 (fr, 30 jul 2010) $
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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

#include "PalaeomagUtils.h"

#include "property-values/GmlPoint.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"

bool
GPlatesAppLogic::PalaeomagUtils::VirtualGeomagneticPolePropertyFinder::initialise_pre_feature_properties(
	const GPlatesModel::FeatureHandle &feature_handle)
{
	static const GPlatesModel::FeatureType vgp_feature_type =
		GPlatesModel::FeatureType::create_gpml("VirtualGeomagneticPole");

	if (feature_handle.feature_type() == vgp_feature_type)
	{
		d_is_vgp_feature = true;
		return true;
	}

	return false;
}

void
GPlatesAppLogic::PalaeomagUtils::VirtualGeomagneticPolePropertyFinder::visit_gpml_constant_value(
	const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}

void
GPlatesAppLogic::PalaeomagUtils::VirtualGeomagneticPolePropertyFinder::visit_gpml_plate_id(
	const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	d_plate_id.reset(gpml_plate_id.get_value()); 
}

void
GPlatesAppLogic::PalaeomagUtils::VirtualGeomagneticPolePropertyFinder::visit_gml_point(
	const GPlatesPropertyValues::GmlPoint &gml_point)
{
	static const GPlatesModel::PropertyName site_name = 
		GPlatesModel::PropertyName::create_gpml("averageSampleSitePosition");

	static const GPlatesModel::PropertyName vgp_name =
		GPlatesModel::PropertyName::create_gpml("polePosition");

	if (current_top_level_propname() == site_name)
	{
		d_site_point.reset(gml_point.get_point());
	}
	else if (current_top_level_propname() == vgp_name)
	{
		d_vgp_point.reset(gml_point.get_point());
	}
}

void
GPlatesAppLogic::PalaeomagUtils::VirtualGeomagneticPolePropertyFinder::visit_xs_double(
	const GPlatesPropertyValues:: XsDouble &xs_double)
{
	static const GPlatesModel::PropertyName age_name = 
		GPlatesModel::PropertyName::create_gpml("averageAge");

	if (current_top_level_propname() == age_name)
	{
		d_age.reset(xs_double.get_value());
	}
}

