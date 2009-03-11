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
#include "model/ReconstructedFeatureGeometry.h"


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
			GPlatesGui::Colour::BLACK;

	// Reconstruction features.
	d_colours[ GPlatesModel::FeatureType::create_gpml("TotalReconstructionSequence") ] = 
			GPlatesGui::Colour::AQUA;
	d_colours[ GPlatesModel::FeatureType::create_gpml("AbsoluteReferenceFrame") ] = 
			GPlatesGui::Colour::RED;
	
	// Artificial features.
	d_colours[ GPlatesModel::FeatureType::create_gpml("ClosedPlateBoundary") ] =
			GPlatesGui::Colour::GREEN;
	d_colours[ GPlatesModel::FeatureType::create_gpml("ClosedContinentalBoundary") ] =
			GPlatesGui::Colour::BLUE;
	d_colours[ GPlatesModel::FeatureType::create_gpml("InferredPaleoBoundary") ] =
			GPlatesGui::Colour::SILVER;
	d_colours[ GPlatesModel::FeatureType::create_gpml("OldPlatesGridMark") ] =
			GPlatesGui::Colour::MAROON;

	// Rock units.
	d_colours[ GPlatesModel::FeatureType::create_gpml("BasicRockUnit") ] =
			GPlatesGui::Colour::PURPLE;
		
	// Abstract Geological Plane & Contact features.
	d_colours[ GPlatesModel::FeatureType::create_gpml("GeologicalPlane") ] =
			GPlatesGui::Colour::FUSCHIA;
	d_colours[ GPlatesModel::FeatureType::create_gpml("FoldPlane") ] =
			GPlatesGui::Colour::LIME;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Fault") ] =
			GPlatesGui::Colour::GREY;
	d_colours[ GPlatesModel::FeatureType::create_gpml("TerraneBoundary") ] =
			GPlatesGui::Colour::YELLOW;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Unconformity") ] =
			GPlatesGui::Colour::NAVY;
	d_colours[ GPlatesModel::FeatureType::create_gpml("UnknownContact") ] =
			GPlatesGui::Colour::TEAL;

	// Tectonic sections.
	d_colours[ GPlatesModel::FeatureType::create_gpml("MidOceanRidge") ] =
			GPlatesGui::Colour::AQUA;
	d_colours[ GPlatesModel::FeatureType::create_gpml("ContinentalRift") ] =
			GPlatesGui::Colour::BLACK;
	d_colours[ GPlatesModel::FeatureType::create_gpml("SubductionZone") ] =
			GPlatesGui::Colour::AQUA;
	d_colours[ GPlatesModel::FeatureType::create_gpml("OrogenicBelt") ] =
			GPlatesGui::Colour::RED;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Transform") ] =
			GPlatesGui::Colour::GREEN;
	d_colours[ GPlatesModel::FeatureType::create_gpml("FractureZone") ] =
			GPlatesGui::Colour::BLUE;
	d_colours[ GPlatesModel::FeatureType::create_gpml("PassiveContinentalBoundary") ] =
			GPlatesGui::Colour::SILVER;

	// Abstract fields.
	d_colours[ GPlatesModel::FeatureType::create_gpml("Bathymetry") ] =
			GPlatesGui::Colour::MAROON;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Topography") ] =
			GPlatesGui::Colour::PURPLE;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Gravimetry") ] =
			GPlatesGui::Colour::FUSCHIA;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Magnetics") ] =
			GPlatesGui::Colour::LIME;
	d_colours[ GPlatesModel::FeatureType::create_gpml("GlobalElevation") ] =
			GPlatesGui::Colour::GREY;
	d_colours[ GPlatesModel::FeatureType::create_gpml("OceanicAge") ] =
			GPlatesGui::Colour::YELLOW;
	d_colours[ GPlatesModel::FeatureType::create_gpml("CrustalThickness") ] =
			GPlatesGui::Colour::NAVY;
	d_colours[ GPlatesModel::FeatureType::create_gpml("DynamicTopography") ] =
			GPlatesGui::Colour::TEAL;
	d_colours[ GPlatesModel::FeatureType::create_gpml("MantleDensity") ] =
			GPlatesGui::Colour::AQUA;
	d_colours[ GPlatesModel::FeatureType::create_gpml("HeatFlow") ] =
			GPlatesGui::Colour::BLACK;
	d_colours[ GPlatesModel::FeatureType::create_gpml("SedimentThickness") ] =
			GPlatesGui::Colour::AQUA;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Roughness") ] =
			GPlatesGui::Colour::RED;
	d_colours[ GPlatesModel::FeatureType::create_gpml("SpreadingRate") ] =
			GPlatesGui::Colour::GREEN;
	d_colours[ GPlatesModel::FeatureType::create_gpml("SpreadingAsymmetry") ] =
			GPlatesGui::Colour::BLUE;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Stress") ] =
			GPlatesGui::Colour::SILVER;

	// Tangible features.
	d_colours[ GPlatesModel::FeatureType::create_gpml("Isochron") ] = 
			GPlatesGui::Colour::MAROON;
	d_colours[ GPlatesModel::FeatureType::create_gpml("MagneticAnomalyIndentification") ] = 
			GPlatesGui::Colour::PURPLE;
	d_colours[ GPlatesModel::FeatureType::create_gpml("MagneticAnomalyShipTrack") ] = 
			GPlatesGui::Colour::FUSCHIA;
	d_colours[ GPlatesModel::FeatureType::create_gpml("FractureZoneIdentification") ] = 
			GPlatesGui::Colour::LIME;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Suture") ] = 
			GPlatesGui::Colour::GREY;
	d_colours[ GPlatesModel::FeatureType::create_gpml("IslandArc") ] = 
			GPlatesGui::Colour::YELLOW;
	d_colours[ GPlatesModel::FeatureType::create_gpml("HotSpotTrail") ] = 
			GPlatesGui::Colour::NAVY;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Seamount") ] =
			GPlatesGui::Colour::TEAL;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Volcano") ] =
			GPlatesGui::Colour::AQUA;
	d_colours[ GPlatesModel::FeatureType::create_gpml("AseismicRidge") ] =
			GPlatesGui::Colour::BLACK;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Coastline") ] =
			GPlatesGui::Colour::AQUA;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Craton") ] =
			GPlatesGui::Colour::RED;
	d_colours[ GPlatesModel::FeatureType::create_gpml("LargeIgneousProvince") ] =
			GPlatesGui::Colour::GREEN;
	d_colours[ GPlatesModel::FeatureType::create_gpml("Basin") ] =
			GPlatesGui::Colour::BLUE;
	d_colours[ GPlatesModel::FeatureType::create_gpml("ExtendedContinentalCrust") ] =
			GPlatesGui::Colour::SILVER;
	d_colours[ GPlatesModel::FeatureType::create_gpml("TransitionalCrust") ] =
			GPlatesGui::Colour::MAROON;
	d_colours[ GPlatesModel::FeatureType::create_gpml("ContinentalFragment") ] =
			GPlatesGui::Colour::PURPLE;
	d_colours[ GPlatesModel::FeatureType::create_gpml("GeologicalLineation") ] =
			GPlatesGui::Colour::FUSCHIA;
	d_colours[ GPlatesModel::FeatureType::create_gpml("PseudoFault") ] =
			GPlatesGui::Colour::LIME;
	d_colours[ GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature") ] =
			GPlatesGui::Colour::GREY;

	//see file-io/FeaturePropertiesMap.h
}


GPlatesGui::ColourTable::const_iterator
GPlatesGui::FeatureColourTable::lookup(
		const GPlatesModel::ReconstructedFeatureGeometry &feature_geometry) const
{
	GPlatesGui::ColourTable::const_iterator colour = NULL;

	if (feature_geometry.is_valid()) {
		GPlatesModel::FeatureType feature_type =
				feature_geometry.feature_handle_ptr()->feature_type();
		colour_map_type::const_iterator iter = d_colours.find(feature_type);
		if (iter == d_colours.end()) {
			colour = NULL;
		} else {
			colour = &(iter->second);
		}
	}
	return colour;
}


GPlatesGui::FeatureColourTable *
GPlatesGui::FeatureColourTable::d_instance;
