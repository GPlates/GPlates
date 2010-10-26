/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "FeaturePropertiesMap.h"

#include "PropertyCreationUtils.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"

#define GET_PROP_VAL_NAME(create_prop) GPlatesFileIO::PropertyCreationUtils::create_prop##_as_prop_val


namespace
{
	using namespace GPlatesFileIO;
	using GPlatesModel::PropertyName;
	using GPlatesModel::FeatureType;
	namespace Utils = GpmlReaderUtils;

	const PropertyCreationUtils::PropertyCreatorMap
	get_gml_abstract_feature_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map;

		map[ PropertyName::create_gml("name") ] =
			GET_PROP_VAL_NAME(create_xs_string);
		map[ PropertyName::create_gml("description") ] =
			GET_PROP_VAL_NAME(create_xs_string);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_abstract_feature_properties()
	{
		using namespace GPlatesFileIO;

		PropertyCreationUtils::PropertyCreatorMap map = get_gml_abstract_feature_properties();

		map[ PropertyName::create_gpml("subcategory") ] =
			GET_PROP_VAL_NAME(create_xs_string);
		map[ PropertyName::create_gpml("supersededRevision") ] =
			GET_PROP_VAL_NAME(create_gpml_revision_id);
		map[ PropertyName::create_gpml("oldPlatesHeader") ] =
			GET_PROP_VAL_NAME(create_old_plates_header);
		map[ PropertyName::create_gpml("shapefileAttributes") ] =
			GET_PROP_VAL_NAME(create_key_value_dictionary);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_time_variant_feature_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_abstract_feature_properties();
		
		map[ PropertyName::create_gpml("validTime") ] =
			GET_PROP_VAL_NAME(create_time_period);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_reconstructable_feature_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_time_variant_feature_properties();

		map[ PropertyName::create_gpml("reconstructionPlateId") ] =
			GET_PROP_VAL_NAME(create_constant_value);
		map[ PropertyName::create_gpml("truncatedSection") ] = 
			GET_PROP_VAL_NAME(create_feature_reference);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_tangible_feature_properties()
	{
		return get_reconstructable_feature_properties();
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_abstract_geological_plane_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();
		
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("dipSide") ] = 
			GET_PROP_VAL_NAME(create_gpml_dip_side_enumeration);
#if 0
		map[ PropertyName::create_gpml("dipAngle") ] = 
			GET_PROP_VAL_NAME(create_angle);
#endif
		
		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_abstract_geological_contact_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_abstract_geological_plane_properties();
		
		map[ PropertyName::create_gpml("leftUnit") ] = 
			GET_PROP_VAL_NAME(create_feature_reference);
		map[ PropertyName::create_gpml("rightUnit") ] = 
			GET_PROP_VAL_NAME(create_feature_reference);
		
		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_geological_plane_properties()
	{
		return get_abstract_geological_plane_properties();
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_fold_plane_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_abstract_geological_plane_properties();
		
		map[ PropertyName::create_gpml("foldAnnotation") ] = 
			GET_PROP_VAL_NAME(create_gpml_fold_plane_annotation_enumeration);
		
		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_fault_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_abstract_geological_contact_properties();
		
		map[ PropertyName::create_gpml("strikeSlip") ] = 
			GET_PROP_VAL_NAME(create_gpml_strike_slip_enumeration);
		map[ PropertyName::create_gpml("dipSlip") ] = 
			GET_PROP_VAL_NAME(create_gpml_dip_slip_enumeration);
		map[ PropertyName::create_gpml("primarySlipComponent") ] = 
			GET_PROP_VAL_NAME(create_gpml_slip_component_enumeration);
		
		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_terrane_boundary_properties()
	{
		return get_abstract_geological_contact_properties();
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_unconformity_properties()
	{
		return get_abstract_geological_contact_properties();
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_unknown_contact_properties()
	{
		return get_abstract_geological_contact_properties();
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_isochron_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("conjugatePlateId") ] = 
			GET_PROP_VAL_NAME(create_plate_id);
		map[ PropertyName::create_gpml("polarityChronId") ] = 
			GET_PROP_VAL_NAME(create_polarity_chron_id);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("conjugate") ] = 
			GET_PROP_VAL_NAME(create_feature_reference);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_magnetic_anomaly_identification_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] = 
			GET_PROP_VAL_NAME(create_point);
		map[ PropertyName::create_gpml("multiPosition") ] = 
			GET_PROP_VAL_NAME(create_gml_multi_point);
		map[ PropertyName::create_gpml("polarityChronId") ] = 
			GET_PROP_VAL_NAME(create_polarity_chron_id);
		map[ PropertyName::create_gpml("polarityChronOffset") ] =
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("shipTrack") ] =
			GET_PROP_VAL_NAME(create_feature_reference);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_magnetic_anomaly_ship_track_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("pick") ] = 
			GET_PROP_VAL_NAME(create_feature_reference);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_fracture_zone_identification_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] = 
			GET_PROP_VAL_NAME(create_point);
		map[ PropertyName::create_gpml("polarityChronId") ] = 
			GET_PROP_VAL_NAME(create_polarity_chron_id);
		map[ PropertyName::create_gpml("polarityChronOffset") ] =
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("shipTrack") ] =
			GET_PROP_VAL_NAME(create_feature_reference);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_suture_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_psuedo_fault_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_island_arc_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("isActive") ] =
			GET_PROP_VAL_NAME(create_piecewise_aggregation);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_hot_spot_trail_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("mark") ] =
			GET_PROP_VAL_NAME(create_hot_spot_trail_mark);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("errorBounds") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("evidence") ] =
			GET_PROP_VAL_NAME(create_feature_reference);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_hot_spot_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] =
			GET_PROP_VAL_NAME(create_point);
		map[ PropertyName::create_gpml("multiPosition") ] = 
			GET_PROP_VAL_NAME(create_gml_multi_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("trail") ] = 
			GET_PROP_VAL_NAME(create_feature_reference);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_seamount_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] = 
			GET_PROP_VAL_NAME(create_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}

	const PropertyCreationUtils::PropertyCreatorMap
	get_slab_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}



	const PropertyCreationUtils::PropertyCreatorMap
	get_volcano_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] = 
			GET_PROP_VAL_NAME(create_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_aseismic_ridge_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_coastline_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_craton_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_large_igneous_province_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_basin_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_extended_continental_crust_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_transitional_crust_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_continental_fragment_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_geological_lineation_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_pseudo_fault_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}
	
	const PropertyCreationUtils::PropertyCreatorMap
	get_virtual_geomagnetic_pole_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("averageSampleSitePosition") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("polePosition") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);			
		map[ PropertyName::create_gpml("averageInclination") ] = 
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("averageDeclination") ] = 
			GET_PROP_VAL_NAME(create_xs_double);	
		map[ PropertyName::create_gpml("poleA95") ] = 
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("poleDp") ] = 
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("poleDm") ] = 
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("averageAge") ] = 
			GET_PROP_VAL_NAME(create_xs_double);

		return map;
	}	


	const PropertyCreationUtils::PropertyCreatorMap
	get_unclassified_feature_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_mesh_node_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();

		map[ PropertyName::create_gpml("meshPoints") ] = 
			GET_PROP_VAL_NAME(create_gml_multi_point);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_abstract_field_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}

	const PropertyCreationUtils::PropertyCreatorMap
	get_tectonic_section_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("leftPlate") ] =
			GET_PROP_VAL_NAME(create_plate_id);
		map[ PropertyName::create_gpml("rightPlate") ] =
			GET_PROP_VAL_NAME(create_plate_id);
		map[ PropertyName::create_gpml("conjugatePlateId") ] = 
			GET_PROP_VAL_NAME(create_plate_id);
		map[ PropertyName::create_gpml("reconstructionMethod") ] = 
			GET_PROP_VAL_NAME(create_reconstruction_method_enumeration);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_mid_ocean_ridge_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tectonic_section_properties();

		map[ PropertyName::create_gpml("isActive") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_subduction_zone_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tectonic_section_properties();

		map[ PropertyName::create_gpml("subductionPolarity") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("islandArc") ] =
			GET_PROP_VAL_NAME(create_feature_reference);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_orogenic_belt_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tectonic_section_properties();

		map[ PropertyName::create_gpml("subductionPolarity") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_transform_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tectonic_section_properties();

		map[ PropertyName::create_gpml("motion") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_passive_continental_boundary_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tectonic_section_properties();

		map[ PropertyName::create_gpml("edge") ] =
			GET_PROP_VAL_NAME(create_gpml_continental_boundary_edge_enumeration);
		map[ PropertyName::create_gpml("side") ] =
			GET_PROP_VAL_NAME(create_gpml_continental_boundary_side_enumeration);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_instantaneous_feature_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_abstract_feature_properties();

#if 0
		map[ PropertyName::create_gpml("derivedFrom") ] =
			GET_PROP_VAL_NAME(create_feature_snapshot_reference);
#endif
		map[ PropertyName::create_gpml("reconstructedTime") ] =
			GET_PROP_VAL_NAME(create_time_instant);
		map[ PropertyName::create_gpml("validTime") ] =
			GET_PROP_VAL_NAME(create_time_period);
		map[ PropertyName::create_gpml("reconstructionPlateId") ] =
			GET_PROP_VAL_NAME(create_plate_id);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_abstract_rock_unit_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_basic_rock_unit_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_abstract_rock_unit_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_artificial_feature_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_closed_plate_boundary_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_artificial_feature_properties();

		map[ PropertyName::create_gpml("boundary") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_closed_continental_boundary_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_artificial_feature_properties();

		map[ PropertyName::create_gpml("type") ] =
			GET_PROP_VAL_NAME(create_gpml_continental_boundary_crust_enumeration);
		map[ PropertyName::create_gpml("edge") ] =
			GET_PROP_VAL_NAME(create_gpml_continental_boundary_edge_enumeration);
		map[ PropertyName::create_gpml("boundary") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_inferred_paleo_boundary_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_artificial_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("errorBounds") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_old_plates_grid_mark_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_artificial_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_topological_feature_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_time_variant_feature_properties();

		return map;
	}

	const PropertyCreationUtils::PropertyCreatorMap
	get_topological_closed_plate_boundary_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_topological_feature_properties();

		// NOTE: this might change to effectivePlateId
		map[ PropertyName::create_gpml("reconstructionPlateId") ] =
			GET_PROP_VAL_NAME(create_constant_value);

		map[ PropertyName::create_gpml("boundary") ] =
			GET_PROP_VAL_NAME(create_piecewise_aggregation);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_reconstruction_feature_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_abstract_feature_properties();

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_total_reconstruction_sequence_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_reconstruction_feature_properties();

		map[ PropertyName::create_gpml("fixedReferenceFrame") ] =
			GET_PROP_VAL_NAME(create_plate_id);
		map[ PropertyName::create_gpml("movingReferenceFrame") ] =
			GET_PROP_VAL_NAME(create_plate_id);
		map[ PropertyName::create_gpml("totalReconstructionPole") ] =
			GET_PROP_VAL_NAME(create_irregular_sampling);

		return map;
	}


	const PropertyCreationUtils::PropertyCreatorMap
	get_absolute_reference_frame_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_total_reconstruction_sequence_properties();

		map[ PropertyName::create_gpml("type") ] =
			GET_PROP_VAL_NAME(create_gpml_absolute_reference_frame_enumeration);

		return map;
	}

	
	const PropertyCreationUtils::PropertyCreatorMap
	get_raster_properties()
	{
		PropertyCreationUtils::PropertyCreatorMap map = get_abstract_feature_properties();

		map[ PropertyName::create_gpml("domainSet") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("rangeSet") ] =
			GET_PROP_VAL_NAME(create_time_dependent_property_value);
		map[ PropertyName::create_gpml("bandNames") ] =
			GET_PROP_VAL_NAME(create_raster_band_names);

		return map;
	}
}


GPlatesFileIO::FeaturePropertiesMap::FeaturePropertiesMap()
{
	// FIXME: As-yet unimplemented features of the GPGIM are:
	// All Instantaneous*
	// All Topological*
	
	// Instantaneous features.
//	d_map[ FeatureType::create_gpml("InstantaneousClosedPlateBoundary") ] = 
//		get_instantaneous_closed_plate_boundary_properties();

	// Topological features.
	d_map[ FeatureType::create_gpml("TopologicalClosedPlateBoundary") ] =
		get_topological_closed_plate_boundary_properties();
	d_map[ FeatureType::create_gpml("TopologicalNetwork") ] =
		get_topological_closed_plate_boundary_properties();

	// Reconstruction features.
	d_map[ FeatureType::create_gpml("TotalReconstructionSequence") ] = 
		get_total_reconstruction_sequence_properties();
	d_map[ FeatureType::create_gpml("AbsoluteReferenceFrame") ] = 
		get_absolute_reference_frame_properties();
	
	// Artificial features.
	d_map[ FeatureType::create_gpml("ClosedPlateBoundary") ] =
		get_closed_plate_boundary_properties();
	d_map[ FeatureType::create_gpml("ClosedContinentalBoundary") ] =
		get_closed_continental_boundary_properties();
	d_map[ FeatureType::create_gpml("InferredPaleoBoundary") ] =
		get_inferred_paleo_boundary_properties();
	d_map[ FeatureType::create_gpml("OldPlatesGridMark") ] =
		get_old_plates_grid_mark_properties();
	d_map[ FeatureType::create_gpml("MeshNode") ] =
		get_mesh_node_properties();

	// Rock units.
	d_map[ FeatureType::create_gpml("BasicRockUnit") ] =
		get_basic_rock_unit_properties();
		
	// Abstract Geological Plane & Contact features.
	d_map[ FeatureType::create_gpml("GeologicalPlane") ] =
		get_geological_plane_properties();
	d_map[ FeatureType::create_gpml("FoldPlane") ] =
		get_fold_plane_properties();
	d_map[ FeatureType::create_gpml("Fault") ] =
		get_fault_properties();
	d_map[ FeatureType::create_gpml("TerraneBoundary") ] =
		get_terrane_boundary_properties();
	d_map[ FeatureType::create_gpml("Unconformity") ] =
		get_unconformity_properties();
	d_map[ FeatureType::create_gpml("UnknownContact") ] =
		get_unknown_contact_properties();

	// Tectonic sections.
	d_map[ FeatureType::create_gpml("MidOceanRidge") ] =
		get_mid_ocean_ridge_properties();
	d_map[ FeatureType::create_gpml("ContinentalRift") ] =
		get_tectonic_section_properties();
	d_map[ FeatureType::create_gpml("SubductionZone") ] =
		get_subduction_zone_properties();
	d_map[ FeatureType::create_gpml("OrogenicBelt") ] =
		get_orogenic_belt_properties();
	d_map[ FeatureType::create_gpml("Transform") ] =
		get_transform_properties();
	d_map[ FeatureType::create_gpml("FractureZone") ] =
		get_tectonic_section_properties();
	d_map[ FeatureType::create_gpml("PassiveContinentalBoundary") ] =
		get_passive_continental_boundary_properties();

	// Fields.
	d_map[ FeatureType::create_gpml("Bathymetry") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("Topography") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("Gravimetry") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("Magnetics") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("GlobalElevation") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("OceanicAge") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("CrustalThickness") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("DynamicTopography") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("MantleDensity") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("HeatFlow") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("SedimentThickness") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("Roughness") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("SpreadingRate") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("SpreadingAsymmetry") ] =
		get_abstract_field_properties();
	d_map[ FeatureType::create_gpml("Stress") ] =
		get_abstract_field_properties();

	// Tangible features.
	d_map[ FeatureType::create_gpml("Isochron") ] = 
		get_isochron_properties();
	d_map[ FeatureType::create_gpml("MagneticAnomalyIdentification") ] = 
		get_magnetic_anomaly_identification_properties();
	d_map[ FeatureType::create_gpml("MagneticAnomalyShipTrack") ] = 
		get_magnetic_anomaly_ship_track_properties();
	d_map[ FeatureType::create_gpml("FractureZoneIdentification") ] = 
		get_fracture_zone_identification_properties();
	d_map[ FeatureType::create_gpml("Suture") ] = 
		get_suture_properties();
	d_map[ FeatureType::create_gpml("IslandArc") ] = 
		get_island_arc_properties();
	d_map[ FeatureType::create_gpml("HotSpot") ] = 
		get_hot_spot_properties();
	d_map[ FeatureType::create_gpml("HotSpotTrail") ] = 
		get_hot_spot_trail_properties();
	d_map[ FeatureType::create_gpml("Seamount") ] =
		get_seamount_properties();
	d_map[ FeatureType::create_gpml("Slab") ] =
		get_slab_properties();
	d_map[ FeatureType::create_gpml("Volcano") ] =
	d_map[ FeatureType::create_gpml("Volcano") ] =
		get_volcano_properties();
	d_map[ FeatureType::create_gpml("AseismicRidge") ] =
		get_aseismic_ridge_properties();
	d_map[ FeatureType::create_gpml("Coastline") ] =
		get_coastline_properties();
	d_map[ FeatureType::create_gpml("Craton") ] =
		get_craton_properties();
	d_map[ FeatureType::create_gpml("LargeIgneousProvince") ] =
		get_large_igneous_province_properties();
	d_map[ FeatureType::create_gpml("Basin") ] =
		get_basin_properties();
	d_map[ FeatureType::create_gpml("ExtendedContinentalCrust") ] =
		get_extended_continental_crust_properties();
	d_map[ FeatureType::create_gpml("TransitionalCrust") ] =
		get_transitional_crust_properties();
	d_map[ FeatureType::create_gpml("ContinentalFragment") ] =
		get_continental_fragment_properties();
	d_map[ FeatureType::create_gpml("GeologicalLineation") ] =
		get_geological_lineation_properties();
	d_map[ FeatureType::create_gpml("PseudoFault") ] =
		get_pseudo_fault_properties();
	d_map[FeatureType::create_gpml("VirtualGeomagneticPole") ] =
		get_virtual_geomagnetic_pole_properties();
	d_map[ FeatureType::create_gpml("UnclassifiedFeature") ] =
		get_unclassified_feature_properties();

	// Rasters.
	d_map[ FeatureType::create_gpml("Raster") ] =
		get_raster_properties();

	// Time variant features.
}


GPlatesFileIO::FeaturePropertiesMap::const_iterator
GPlatesFileIO::FeaturePropertiesMap::find(
		const GPlatesModel::FeatureType &key) const
{
	return d_map.find(key);
}


GPlatesFileIO::FeaturePropertiesMap::const_iterator
GPlatesFileIO::FeaturePropertiesMap::begin() const
{
	return d_map.begin();
}


GPlatesFileIO::FeaturePropertiesMap::const_iterator
GPlatesFileIO::FeaturePropertiesMap::end() const
{
	return d_map.end();
}


bool
GPlatesFileIO::FeaturePropertiesMap::is_valid_property(
		const GPlatesModel::FeatureType &feature_type,
		const GPlatesModel::PropertyName &property_name) const
{
	const_iterator iter = find(feature_type);
	if (iter == end())
	{
		return false;
	}

	const PropertyCreationUtils::PropertyCreatorMap &prop_map = iter->second;
	return prop_map.find(property_name) != prop_map.end();
}

