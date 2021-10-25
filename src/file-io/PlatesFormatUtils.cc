/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "PlatesFormatUtils.h"

#include "property-values/XsBoolean.h"
#include "property-values/Enumeration.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlOldPlatesHeader.h"


namespace
{
	/**
	 * If specified feature has "isActive" boolean property then return active data type code
	 * if its value is true otherwise return inactive data type code.
	 * If specified feature does not have "isActive" boolean property then
	 * return inactive data type code.
	 */
	GPlatesUtils::UnicodeString
	get_data_type_code_for_active_inactive_feature(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature,
			const GPlatesUtils::UnicodeString &active_data_type_code,
			const GPlatesUtils::UnicodeString &inactive_data_type_code)
	{
		static const GPlatesModel::PropertyName is_active_property_name = 
			GPlatesModel::PropertyName::create_gpml("isActive");

		// See if active or not.
		// Note: set to NULL due to "may be used uninitialized in this function"
		// error on g++ 4.3.3-3.
		const GPlatesPropertyValues::XsBoolean *is_active_property_value = NULL;
		if (GPlatesFeatureVisitors::get_property_value(
				feature, is_active_property_name, is_active_property_value))
		{
			return is_active_property_value->value() ? active_data_type_code : inactive_data_type_code;
		}

		// No "isActive" property on feature so assume inactive.
		return inactive_data_type_code;
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_aseismic_ridge(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "AR";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_bathymetry(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "BA";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_basin(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "BS";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_continental_boundary(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		// Could also be "CM" or "CO" according to "PlatesLineFormatHeaderVisitor.h".
		return "CB";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_continental_fragment(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "CF";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_craton(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "CR";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_coastline(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "CS";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_extended_continental_crust(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "EC";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_fault(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		//
		// This function is effectively the reverse of what's in "PlatesLineFormatReader.cc".
		//

		static const GPlatesModel::PropertyName dipslip_property_name = 
			GPlatesModel::PropertyName::create_gpml("dipSlip");

		const GPlatesPropertyValues::Enumeration *dipslip_property_value;
		if (GPlatesFeatureVisitors::get_property_value(
				feature, dipslip_property_name, dipslip_property_value))
		{
			static const GPlatesPropertyValues::EnumerationType dipslip_enumeration_type =
					GPlatesPropertyValues::EnumerationType::create_gpml("DipSlipEnumeration");
			static const GPlatesPropertyValues::EnumerationContent
					dipslip_enumeration_value_compression("Compression");
			static const GPlatesPropertyValues::EnumerationContent
					dipslip_enumeration_value_extension("Extension");

			if (dipslip_enumeration_type.is_equal_to(dipslip_property_value->type()))
			{
				if (dipslip_enumeration_value_compression.is_equal_to(dipslip_property_value->value()))
				{
					return "NF";
				}
				if (dipslip_enumeration_value_extension.is_equal_to(dipslip_property_value->value()))
				{
					static const GPlatesModel::PropertyName subcategory_property_name = 
						GPlatesModel::PropertyName::create_gpml("subcategory");

					const GPlatesPropertyValues::XsString *subcategory_property_value;
					if (GPlatesFeatureVisitors::get_property_value(
							feature, subcategory_property_name, subcategory_property_value))
					{
						static const GPlatesPropertyValues::TextContent thrust_string("Thrust");

						if (subcategory_property_value->value().is_equal_to(thrust_string))
						{
							return "TH";
						}
					}

					return "RF";
				}
			}
		}

		static const GPlatesModel::PropertyName strike_slip_property_name = 
			GPlatesModel::PropertyName::create_gpml("strikeSlip");

		const GPlatesPropertyValues::Enumeration *strike_slip_property_value;
		if (GPlatesFeatureVisitors::get_property_value(
				feature, strike_slip_property_name, strike_slip_property_value))
		{
			static const GPlatesPropertyValues::EnumerationType strike_slip_enumeration_type =
					GPlatesPropertyValues::EnumerationType::create_gpml("StrikeSlipEnumeration");
			static const GPlatesPropertyValues::EnumerationContent
					strike_slip_enumeration_value_unknown("Unknown");

			if (strike_slip_enumeration_type.is_equal_to(strike_slip_property_value->type()))
			{
				if (strike_slip_enumeration_value_unknown.is_equal_to(strike_slip_property_value->value()))
				{
					return "SS";
				}
			}
		}

		return "FT";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_fracture_zone(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "FZ";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_grid_mark(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "GR";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_gravimetry(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "GV";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_heat_flow(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "HF";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_hot_spot(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "HS";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_hot_spot_trail(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "HT";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_island_arc(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		return get_data_type_code_for_active_inactive_feature(feature, "IA", "IR");
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_isochron(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		// Could also be "IM" according to "PlatesLineFormatHeaderVisitor.h".
		return "IC";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_isopach(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "IP";
	}

	// -might- be Ice Shelf, might be Isochron. We don't know.
	GPlatesUtils::UnicodeString
	get_data_type_code_for_unclassified_feature(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "UN";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_geological_lineation(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "LI";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_magnetics(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "MA";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_orogenic_belt(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		// Could also be "OR" according to "PlatesLineFormatHeaderVisitor.h".
		return "OB";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_ophiolite_belt(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "OP";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_inferred_paleo_boundary(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "PB";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_magnetic_pick(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		// Could also be "PC" according to "PlatesLineFormatHeaderVisitor.h".
		return "PM";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_ridge_segment(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		return get_data_type_code_for_active_inactive_feature(feature, "RI", "XR");
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_seamount(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "SM";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_slab(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "SL";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_suture(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "SU";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_terrane_boundary(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "TB";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_transitional_crust(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "TC";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_transform(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "TF";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_topography(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "TO";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_subduction_zone(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature)
	{
		//
		// First test to see if subduction zone is subducting on left or right of geometry.
		//

		static const GPlatesModel::PropertyName subduction_polarity_property_name = 
			GPlatesModel::PropertyName::create_gpml("subductionPolarity");

		const GPlatesPropertyValues::Enumeration *subduction_polarity_property_value = NULL;
		if (GPlatesFeatureVisitors::get_property_value(
				feature, subduction_polarity_property_name, subduction_polarity_property_value))
		{
			static const GPlatesPropertyValues::EnumerationType subduction_polarity_enumeration_type =
					GPlatesPropertyValues::EnumerationType::create_gpml("SubductionPolarityEnumeration");
			static const GPlatesPropertyValues::EnumerationContent
					subduction_polarity_enumeration_value_left("Left");
			static const GPlatesPropertyValues::EnumerationContent
					subduction_polarity_enumeration_value_right("Right");

			if (subduction_polarity_enumeration_type.is_equal_to(subduction_polarity_property_value->type()))
			{
				if (subduction_polarity_enumeration_value_left.is_equal_to(
						subduction_polarity_property_value->value()))
				{
					return "sL";
				}
				if (subduction_polarity_enumeration_value_right.is_equal_to(
						subduction_polarity_property_value->value()))
				{
					return "sR";
				}
			}
		}

		static const GPlatesModel::PropertyName is_active_property_name = 
			GPlatesModel::PropertyName::create_gpml("isActive");

		// See if active or not.
		// Note: set to NULL due to "may be used uninitialized in this function"
		// error on g++ 4.3.3-3.
		const GPlatesPropertyValues::XsBoolean *is_active_property_value = NULL;
		if (GPlatesFeatureVisitors::get_property_value(
				feature, is_active_property_name, is_active_property_value))
		{
			return is_active_property_value->value() ? "TR" : "XT";
		}

		// No "isActive" property on feature so assume inactive.
		return "XT";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_volcano(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "VO";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_pluton(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "PL";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_ophiolite(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "OH";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_political_boundary(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "PO";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_large_igneous_province(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "VP";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_navdat_1(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "N1";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_navdat_2(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "N2";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_navdat_3(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "N3";
	}

	GPlatesUtils::UnicodeString
	get_data_type_code_for_navdat_4(
			const GPlatesModel::FeatureHandle::const_weak_ref &)
	{
		return "N4";
	}


	/**
	 * Typedef for a function that queries a feature and returns a PLATES data type code.
	 */
	typedef GPlatesUtils::UnicodeString (*get_data_type_code_function_type)(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature);


	/**
	 * Maps feature type to plates header data type code.
	 */
	typedef std::map<GPlatesModel::FeatureType, get_data_type_code_function_type>
			plates_data_type_code_map_type;


	/**
	 * Returns the plates data type code map.
	 *
	 * If this is the first time called then the map is built.
	 */
	const plates_data_type_code_map_type &
	get_data_type_code_map()
	{
		static bool initialised = false;
		static plates_data_type_code_map_type plates_data_type_code_map;

		if (!initialised)
		{
			//
			// This is effectively the inverse of the mapping found in "PlatesLineFormatReader.cc".
			//

			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("AseismicRidge")] =
					get_data_type_code_for_aseismic_ridge;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Bathymetry")] = 
					get_data_type_code_for_bathymetry;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Basin")] = 
					get_data_type_code_for_basin;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("PassiveContinentalBoundary")] = 
					get_data_type_code_for_continental_boundary;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("ContinentalFragment")] = 
					get_data_type_code_for_continental_fragment;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Craton")] = 
					get_data_type_code_for_craton;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Coastline")] = 
					get_data_type_code_for_coastline;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("ExtendedContinentalCrust")] = 
					get_data_type_code_for_extended_continental_crust;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Fault")] = 
					get_data_type_code_for_fault;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("FractureZone")] = 
					get_data_type_code_for_fracture_zone;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("OldPlatesGridMark")] = 
					get_data_type_code_for_grid_mark;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Gravimetry")] = 
					get_data_type_code_for_gravimetry;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("HeatFlow")] = 
					get_data_type_code_for_heat_flow;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("HotSpot")] = 
					get_data_type_code_for_hot_spot;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("HotSpotTrail")] = 
					get_data_type_code_for_hot_spot_trail;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("IslandArc")] = 
					get_data_type_code_for_island_arc;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Isochron")] = 
					get_data_type_code_for_isochron;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("SedimentThickness")] = 
					get_data_type_code_for_isopach;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature")] = 
					get_data_type_code_for_unclassified_feature;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("GeologicalLineation")] = 
					get_data_type_code_for_geological_lineation;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Magnetics")] = 
					get_data_type_code_for_magnetics;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("OrogenicBelt")] = 
					get_data_type_code_for_orogenic_belt;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("BasicRockUnit")] = 
					get_data_type_code_for_ophiolite_belt;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("InferredPaleoBoundary")] = 
					get_data_type_code_for_inferred_paleo_boundary;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("MagneticAnomalyIdentification")] = 
					get_data_type_code_for_magnetic_pick;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("MidOceanRidge")] = 
					get_data_type_code_for_ridge_segment;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Seamount")] = 
					get_data_type_code_for_seamount;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Suture")] = 
					get_data_type_code_for_suture;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("TerraneBoundary")] = 
					get_data_type_code_for_terrane_boundary;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("TransitionalCrust")] = 
					get_data_type_code_for_transitional_crust;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Transform")] = 
					get_data_type_code_for_transform;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Topography")] = 
					get_data_type_code_for_topography;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("SubductionZone")] = 
					get_data_type_code_for_subduction_zone;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Volcano")] = 
					get_data_type_code_for_volcano;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Pluton")] = 
					get_data_type_code_for_pluton;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Ophiolite")] = 
					get_data_type_code_for_ophiolite;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("PoliticalBoundary")] = 
					get_data_type_code_for_political_boundary;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("LargeIgneousProvince")] = 
					get_data_type_code_for_large_igneous_province;

			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("NavdatSampleMafic")] = 
					get_data_type_code_for_navdat_1;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("NavdatSampleIntermediate")] = 
					get_data_type_code_for_navdat_2;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("NavdatSampleFelsicLow")] = 
					get_data_type_code_for_navdat_3;
			plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("NavdatSampleFelsicHigh")] = 
					get_data_type_code_for_navdat_4;

			initialised = true;
		}

		return plates_data_type_code_map;
	}
}


GPlatesUtils::UnicodeString
GPlatesFileIO::PlatesFormatUtils::get_plates_data_type_code(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature)
{
	const GPlatesModel::FeatureType &feature_type = feature->feature_type();

	const plates_data_type_code_map_type &plates_data_type_code_map =
			get_data_type_code_map();

	// Use the feature type to lookup up the function used to determine the data code type.
	plates_data_type_code_map_type::const_iterator data_type_code_iter =
			plates_data_type_code_map.find(feature_type);
	if (data_type_code_iter == plates_data_type_code_map.end())
	{
		// Cannot map feature to a Plates data type so indicate this.
		return INVALID_DATA_TYPE_CODE;
	}

	// The function required to determine the PLATES data type code.
	get_data_type_code_function_type get_data_type_code_function = data_type_code_iter->second;

	// Call function to determine data type code.
	return get_data_type_code_function(feature);
}
