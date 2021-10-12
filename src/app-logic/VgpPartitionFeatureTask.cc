/* $Id$ */
 
/**
 * \file 
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

#include <QDebug>

#include "VgpPartitionFeatureTask.h"

#include "GeometryCookieCutter.h"
#include "PartitionFeatureUtils.h"
#include "ReconstructionGeometryUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"

#include "property-values/GmlPoint.h"

#include "utils/UnicodeStringUtils.h"


bool
GPlatesAppLogic::VgpPartitionFeatureTask::can_partition_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref) const
{
	// See if the feature is a VirtualGeomagneticPole.
	static const GPlatesModel::FeatureType vgp_feature_type = 
			GPlatesModel::FeatureType::create_gpml("VirtualGeomagneticPole");

	return feature_ref->feature_type() == vgp_feature_type;
}


void
GPlatesAppLogic::VgpPartitionFeatureTask::partition_feature(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
		const GPlatesAppLogic::GeometryCookieCutter &geometry_cookie_cutter)
{
	// Look for the 'gpml:averageSampleSitePosition' property.
	static const GPlatesModel::PropertyName sample_site_property_name =
			GPlatesModel::PropertyName::create_gpml("averageSampleSitePosition");
	const GPlatesPropertyValues::GmlPoint *sample_site_gml_point = NULL;
	if (!GPlatesFeatureVisitors::get_property_value(
			feature_ref, sample_site_property_name, sample_site_gml_point))
	{
		qDebug() << "WARNING: Unable to find 'gpml:averageSampleSitePosition' property "
				"in 'VirtualGeomagneticPole' with feature id = ";
		qDebug() << GPlatesUtils::make_qstring_from_icu_string(
				feature_ref->feature_id().get());
		return;
	}
	const GPlatesMaths::PointOnSphere &sample_site_point = *sample_site_gml_point->point();

	// Find a partitioning polygon boundary that contains the sample site.
	const boost::optional<const ReconstructionGeometry *> partitioning_polygon =
						geometry_cookie_cutter.partition_point(sample_site_point);

	if (!partitioning_polygon)
	{
		qDebug() << "WARNING: Unable to assign 'reconstructionPlateId' to "
				"'VirtualGeomagneticPole' with feature id = ";
		qDebug() << GPlatesUtils::make_qstring_from_icu_string(
				feature_ref->feature_id().get());
		qDebug() << "because it's sample site is not inside any partitioning polygon boundaries.";
		return;
	}

	// Get the reconstruction plate id from the partitioning polygon.
	const boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_opt =
			ReconstructionGeometryUtils::get_plate_id(partitioning_polygon.get());
	if (!reconstruction_plate_id_opt)
	{
		qDebug() << "WARNING: Unable to assign 'reconstructionPlateId' to "
				"'VirtualGeomagneticPole' with feature id = ";
		qDebug() << GPlatesUtils::make_qstring_from_icu_string(
				feature_ref->feature_id().get());
		qDebug() << "because the partitioning polygon containing the sample site "
				"does not have a plate id.";
		// This shouldn't really happen.
		return;
	}
	const GPlatesModel::integer_plate_id_type reconstruction_plate_id =
			*reconstruction_plate_id_opt;

	// Now assign the reconstruction plate id to the feature.
	PartitionFeatureUtils::assign_reconstruction_plate_id_to_feature(
			reconstruction_plate_id, feature_ref);

	// NOTE: This paleomag data is present day data - even though the VGP
	// has an age (corresponding to the rock sample age) the location of the sample
	// site and the VGP are actually present day locations.
	// So we don't assume that the reconstruction time (of the partitioning polygons)
	// corresponds to VGP locations at that time. All VGP locations are present day
	// and so it only makes sense for the user to have partitioning polygons at
	// present day.
}
