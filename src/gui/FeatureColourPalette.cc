/* $Id$ */

/**
 * @file 
 * Contains the implementation of the FeatureColourPalette class.
 *
 * Most recent change:
 *   $Date$
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

#include "FeatureColourPalette.h"

GPlatesGui::FeatureColourPalette::FeatureColourPalette()
{
	// FIXME: Pick nicer colours
	
	d_colours[ GPlatesModel::FeatureType::create_gpml("TopologicalClosedPlateBoundary") ] =
			GPlatesGui::Colour::get_black();
	d_colours[ GPlatesModel::FeatureType::create_gpml("TopologicalNetwork") ] =
			GPlatesGui::Colour::get_grey();

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
			GPlatesGui::Colour::get_fuchsia();
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
			GPlatesGui::Colour::get_fuchsia();
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
			GPlatesGui::Colour::get_fuchsia();
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
	d_colours[ GPlatesModel::FeatureType::create_gpml("Slab") ] =
			GPlatesGui::Colour::get_red();
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
			GPlatesGui::Colour::get_fuchsia();
	d_colours[ GPlatesModel::FeatureType::create_gpml("PseudoFault") ] =
			GPlatesGui::Colour::get_lime();
	d_colours[ GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature") ] =
			GPlatesGui::Colour::get_grey();

	//see file-io/FeaturePropertiesMap.h
}

boost::optional<GPlatesGui::Colour>
GPlatesGui::FeatureColourPalette::get_colour(
		const GPlatesModel::FeatureType &feature_type) const
{
	std::map<GPlatesModel::FeatureType, Colour>::const_iterator colour =
		d_colours.find(feature_type);
	if (colour == d_colours.end())
	{
		return boost::none;
	}
	else
	{
		return boost::optional<Colour>(colour->second);
	}
}

