/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#include <map>
#include <algorithm>

#include "FeatureColourTable.h"
#include "app-logic/ReconstructionGeometryUtils.h"


GPlatesGui::FeatureColourTable *
GPlatesGui::FeatureColourTable::Instance()
{
	if (d_instance == NULL) {

		// create a new instance
		d_instance = new FeatureColourTable();
	}
	return d_instance;
}


GPlatesGui::FeatureColourTable::FeatureColourTable()
{
	d_colours[ GPlatesModel::FeatureType::create_gpml("TopologicalClosedPlateBoundary") ] =
			GPlatesGui::Colour::get_black();

	// Reconstruction features.
	d_colours[ GPlatesModel::FeatureType::create_gpml("TotalReconstructionSequence") ] = 
			GPlatesGui::Colour::get_aqua();
	d_colours[ GPlatesModel::FeatureType::create_gpml("AbsoluteReferenceFrame") ] = 
			GPlatesGui::Colour::get_red();
	
	// Artificial features.
	d_colours[ GPlatesModel::FeatureType::create_gpml("ClosedPlateBoundary") ] =
			GPlatesGui::Colour::get_green();
	d_colours[ GPlatesModel::FeatureType::create_gpml("ClosedContinentalBoundary") ] =
			GPlatesGui::Colour::get_blue();
	d_colours[ GPlatesModel::FeatureType::create_gpml("InferredPaleoBoundary") ] =
			GPlatesGui::Colour::get_silver();
	d_colours[ GPlatesModel::FeatureType::create_gpml("OldPlatesGridMark") ] =
			GPlatesGui::Colour::get_maroon();

	// Rock units.
	d_colours[ GPlatesModel::FeatureType::create_gpml("BasicRockUnit") ] =
			GPlatesGui::Colour::get_purple();
		
	// Abstract Geological Plane & Contact features.
	d_colours[ GPlatesModel::FeatureType::create_gpml("GeologicalPlane") ] =
			GPlatesGui::Colour::get_fuschia();
	d_colours[ GPlatesModel::FeatureType::create_gpml("FoldPlane") ] =
			GPlatesGui::Colour::get_lime();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Fault") ] =
			GPlatesGui::Colour::get_grey();
	d_colours[ GPlatesModel::FeatureType::create_gpml("TerraneBoundary") ] =
			GPlatesGui::Colour::get_yellow();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Unconformity") ] =
			GPlatesGui::Colour::get_navy();
	d_colours[ GPlatesModel::FeatureType::create_gpml("UnknownContact") ] =
			GPlatesGui::Colour::get_teal();

	// Tectonic sections.
	d_colours[ GPlatesModel::FeatureType::create_gpml("MidOceanRidge") ] =
			GPlatesGui::Colour::get_aqua();
	d_colours[ GPlatesModel::FeatureType::create_gpml("ContinentalRift") ] =
			GPlatesGui::Colour::get_black();
	d_colours[ GPlatesModel::FeatureType::create_gpml("SubductionZone") ] =
			GPlatesGui::Colour::get_aqua();
	d_colours[ GPlatesModel::FeatureType::create_gpml("OrogenicBelt") ] =
			GPlatesGui::Colour::get_red();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Transform") ] =
			GPlatesGui::Colour::get_green();
	d_colours[ GPlatesModel::FeatureType::create_gpml("FractureZone") ] =
			GPlatesGui::Colour::get_blue();
	d_colours[ GPlatesModel::FeatureType::create_gpml("PassiveContinentalBoundary") ] =
			GPlatesGui::Colour::get_silver();

	// Abstract fields.
	d_colours[ GPlatesModel::FeatureType::create_gpml("Bathymetry") ] =
			GPlatesGui::Colour::get_maroon();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Topography") ] =
			GPlatesGui::Colour::get_purple();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Gravimetry") ] =
			GPlatesGui::Colour::get_fuschia();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Magnetics") ] =
			GPlatesGui::Colour::get_lime();
	d_colours[ GPlatesModel::FeatureType::create_gpml("GlobalElevation") ] =
			GPlatesGui::Colour::get_grey();
	d_colours[ GPlatesModel::FeatureType::create_gpml("OceanicAge") ] =
			GPlatesGui::Colour::get_yellow();
	d_colours[ GPlatesModel::FeatureType::create_gpml("CrustalThickness") ] =
			GPlatesGui::Colour::get_navy();
	d_colours[ GPlatesModel::FeatureType::create_gpml("DynamicTopography") ] =
			GPlatesGui::Colour::get_teal();
	d_colours[ GPlatesModel::FeatureType::create_gpml("MantleDensity") ] =
			GPlatesGui::Colour::get_aqua();
	d_colours[ GPlatesModel::FeatureType::create_gpml("HeatFlow") ] =
			GPlatesGui::Colour::get_black();
	d_colours[ GPlatesModel::FeatureType::create_gpml("SedimentThickness") ] =
			GPlatesGui::Colour::get_aqua();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Roughness") ] =
			GPlatesGui::Colour::get_red();
	d_colours[ GPlatesModel::FeatureType::create_gpml("SpreadingRate") ] =
			GPlatesGui::Colour::get_green();
	d_colours[ GPlatesModel::FeatureType::create_gpml("SpreadingAsymmetry") ] =
			GPlatesGui::Colour::get_blue();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Stress") ] =
			GPlatesGui::Colour::get_silver();

	// Tangible features.
	d_colours[ GPlatesModel::FeatureType::create_gpml("Isochron") ] = 
			GPlatesGui::Colour::get_maroon();
	d_colours[ GPlatesModel::FeatureType::create_gpml("MagneticAnomalyIndentification") ] = 
			GPlatesGui::Colour::get_purple();
	d_colours[ GPlatesModel::FeatureType::create_gpml("MagneticAnomalyShipTrack") ] = 
			GPlatesGui::Colour::get_fuschia();
	d_colours[ GPlatesModel::FeatureType::create_gpml("FractureZoneIdentification") ] = 
			GPlatesGui::Colour::get_lime();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Suture") ] = 
			GPlatesGui::Colour::get_grey();
	d_colours[ GPlatesModel::FeatureType::create_gpml("IslandArc") ] = 
			GPlatesGui::Colour::get_yellow();
	d_colours[ GPlatesModel::FeatureType::create_gpml("HotSpotTrail") ] = 
			GPlatesGui::Colour::get_navy();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Seamount") ] =
			GPlatesGui::Colour::get_teal();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Volcano") ] =
			GPlatesGui::Colour::get_aqua();
	d_colours[ GPlatesModel::FeatureType::create_gpml("AseismicRidge") ] =
			GPlatesGui::Colour::get_black();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Coastline") ] =
			GPlatesGui::Colour::get_aqua();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Craton") ] =
			GPlatesGui::Colour::get_red();
	d_colours[ GPlatesModel::FeatureType::create_gpml("LargeIgneousProvince") ] =
			GPlatesGui::Colour::get_green();
	d_colours[ GPlatesModel::FeatureType::create_gpml("Basin") ] =
			GPlatesGui::Colour::get_blue();
	d_colours[ GPlatesModel::FeatureType::create_gpml("ExtendedContinentalCrust") ] =
			GPlatesGui::Colour::get_silver();
	d_colours[ GPlatesModel::FeatureType::create_gpml("TransitionalCrust") ] =
			GPlatesGui::Colour::get_maroon();
	d_colours[ GPlatesModel::FeatureType::create_gpml("ContinentalFragment") ] =
			GPlatesGui::Colour::get_purple();
	d_colours[ GPlatesModel::FeatureType::create_gpml("GeologicalLineation") ] =
			GPlatesGui::Colour::get_fuschia();
	d_colours[ GPlatesModel::FeatureType::create_gpml("PseudoFault") ] =
			GPlatesGui::Colour::get_lime();
	d_colours[ GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature") ] =
			GPlatesGui::Colour::get_grey();

	//see file-io/FeaturePropertiesMap.h
}


GPlatesGui::ColourTable::const_iterator
GPlatesGui::FeatureColourTable::lookup(
		const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
{
	GPlatesModel::FeatureHandle::weak_ref feature_ref;
	if (!GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(
			&reconstruction_geometry, feature_ref))
	{
		return end();
	}

	return lookup_by_feature_type(feature_ref->feature_type());
}


GPlatesGui::ColourTable::const_iterator
GPlatesGui::FeatureColourTable::lookup_by_feature_type(
		const GPlatesModel::FeatureType &feature_type) const
{
	GPlatesGui::ColourTable::const_iterator colour = end();

	colour_map_type::const_iterator iter = d_colours.find(feature_type);
	if (iter != d_colours.end())
	{
		colour = &(iter->second);
	}

	return colour;
}


GPlatesGui::FeatureColourTable *
GPlatesGui::FeatureColourTable::d_instance;
