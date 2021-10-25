/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
 * Copyright (C) 2010, 2012 Geological Survey of Norway
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

#include "GpmlReaderUtils.h"
#include "GpmlPropertyReaderUtils.h"

#include "model/FeatureType.h"
#include "model/PropertyName.h"

#define GET_PROP_VAL_NAME(create_prop) GPlatesFileIO::GpmlPropertyReaderUtils::create_prop##_as_prop_val


namespace
{
	using namespace GPlatesFileIO;
	using GPlatesModel::PropertyName;
	using GPlatesModel::FeatureType;
	namespace Utils = GpmlReaderUtils;

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_gml_abstract_feature_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map;

		map[ PropertyName::create_gml("name") ] =
			GET_PROP_VAL_NAME(create_xs_string);
		map[ PropertyName::create_gml("description") ] =
			GET_PROP_VAL_NAME(create_xs_string);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_abstract_feature_properties()
	{
		using namespace GPlatesFileIO;

		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_gml_abstract_feature_properties();

		map[ PropertyName::create_gpml("subcategory") ] =
			GET_PROP_VAL_NAME(create_xs_string);
		map[ PropertyName::create_gpml("supersededRevision") ] =
			GET_PROP_VAL_NAME(create_gpml_revision_id);
		map[ PropertyName::create_gpml("oldPlatesHeader") ] =
			GET_PROP_VAL_NAME(create_gpml_old_plates_header);
		map[ PropertyName::create_gpml("shapefileAttributes") ] =
			GET_PROP_VAL_NAME(create_gpml_key_value_dictionary);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_time_variant_feature_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_abstract_feature_properties();
		
		map[ PropertyName::create_gpml("validTime") ] =
			GET_PROP_VAL_NAME(create_gml_time_period);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_reconstructable_feature_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_time_variant_feature_properties();

		map[ PropertyName::create_gpml("reconstructionPlateId") ] =
			GET_PROP_VAL_NAME(create_gpml_constant_value);
		map[ PropertyName::create_gpml("truncatedSection") ] = 
			GET_PROP_VAL_NAME(create_gpml_feature_reference);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_tangible_feature_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();

		map[ PropertyName::create_gpml("rigidBlock") ] =
			GET_PROP_VAL_NAME(create_xs_boolean);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_abstract_geological_plane_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();
		
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("dipSide") ] = 
			GET_PROP_VAL_NAME(create_gpml_dip_side_enumeration);
#if 0
		map[ PropertyName::create_gpml("dipAngle") ] = 
			GET_PROP_VAL_NAME(create_angle);
#endif
		
		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_abstract_geological_contact_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_abstract_geological_plane_properties();
		
		map[ PropertyName::create_gpml("leftUnit") ] = 
			GET_PROP_VAL_NAME(create_gpml_feature_reference);
		map[ PropertyName::create_gpml("rightUnit") ] = 
			GET_PROP_VAL_NAME(create_gpml_feature_reference);
		
		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_geological_plane_properties()
	{
		return get_abstract_geological_plane_properties();
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_fold_plane_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_abstract_geological_plane_properties();
		
		map[ PropertyName::create_gpml("foldAnnotation") ] = 
			GET_PROP_VAL_NAME(create_gpml_fold_plane_annotation_enumeration);
		
		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_fault_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_abstract_geological_contact_properties();
		
		map[ PropertyName::create_gpml("strikeSlip") ] = 
			GET_PROP_VAL_NAME(create_gpml_strike_slip_enumeration);
		map[ PropertyName::create_gpml("dipSlip") ] = 
			GET_PROP_VAL_NAME(create_gpml_dip_slip_enumeration);
		map[ PropertyName::create_gpml("primarySlipComponent") ] = 
			GET_PROP_VAL_NAME(create_gpml_slip_component_enumeration);
		
		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_terrane_boundary_properties()
	{
		return get_abstract_geological_contact_properties();
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_unconformity_properties()
	{
		return get_abstract_geological_contact_properties();
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_unknown_contact_properties()
	{
		return get_abstract_geological_contact_properties();
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_isochron_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("conjugatePlateId") ] = 
			GET_PROP_VAL_NAME(create_gpml_plate_id);
		map[ PropertyName::create_gpml("polarityChronId") ] = 
			GET_PROP_VAL_NAME(create_gpml_polarity_chron_id);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("conjugate") ] = 
			GET_PROP_VAL_NAME(create_gpml_feature_reference);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_magnetic_anomaly_identification_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] = 
			GET_PROP_VAL_NAME(create_gml_point);
		map[ PropertyName::create_gpml("multiPosition") ] = 
			GET_PROP_VAL_NAME(create_gml_multi_point);
		map[ PropertyName::create_gpml("polarityChronId") ] = 
			GET_PROP_VAL_NAME(create_gpml_polarity_chron_id);
		map[ PropertyName::create_gpml("polarityChronOffset") ] =
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("shipTrack") ] =
			GET_PROP_VAL_NAME(create_gpml_feature_reference);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_magnetic_anomaly_ship_track_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("pick") ] = 
			GET_PROP_VAL_NAME(create_gpml_feature_reference);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_fracture_zone_identification_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] = 
			GET_PROP_VAL_NAME(create_gml_point);
		map[ PropertyName::create_gpml("polarityChronId") ] = 
			GET_PROP_VAL_NAME(create_gpml_polarity_chron_id);
		map[ PropertyName::create_gpml("polarityChronOffset") ] =
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("shipTrack") ] =
			GET_PROP_VAL_NAME(create_gpml_feature_reference);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_suture_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_psuedo_fault_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_island_arc_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("isActive") ] =
			GET_PROP_VAL_NAME(create_gpml_piecewise_aggregation);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_hot_spot_trail_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("mark") ] =
			GET_PROP_VAL_NAME(create_gpml_hot_spot_trail_mark);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("errorBounds") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("evidence") ] =
			GET_PROP_VAL_NAME(create_gpml_feature_reference);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_hot_spot_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] =
			GET_PROP_VAL_NAME(create_gml_point);
		map[ PropertyName::create_gpml("multiPosition") ] = 
			GET_PROP_VAL_NAME(create_gml_multi_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("trail") ] = 
			GET_PROP_VAL_NAME(create_gpml_feature_reference);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_seamount_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] = 
			GET_PROP_VAL_NAME(create_gml_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_slab_edge_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		map[ PropertyName::create_gpml("subductionPolarity") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		map[ PropertyName::create_gpml("slabEdgeType") ] = 
			GET_PROP_VAL_NAME(create_xs_string);
		map[ PropertyName::create_gpml("slabFlatLying") ] =
			GET_PROP_VAL_NAME(create_xs_boolean);
		map[ PropertyName::create_gpml("slabFlatLyingDepth") ] =
			GET_PROP_VAL_NAME(create_xs_double);

		map[ PropertyName::create_gpml("subductionZoneAge") ] = 
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("subductionZoneDeepDip") ] = 
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("subductionZoneDepth") ] = 
			GET_PROP_VAL_NAME(create_xs_double);

		return map;
	}



	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_volcano_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] = 
			GET_PROP_VAL_NAME(create_gml_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}

	
	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_navdat_sample_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] = 
			GET_PROP_VAL_NAME(create_gml_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_pluton_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] = 
			GET_PROP_VAL_NAME(create_gml_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_ophiolite_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("position") ] = 
			GET_PROP_VAL_NAME(create_gml_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_aseismic_ridge_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_coastline_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_craton_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_large_igneous_province_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_basin_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_extended_continental_crust_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_transitional_crust_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_continental_fragment_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_geological_lineation_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_pseudo_fault_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}
	
	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_virtual_geomagnetic_pole_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("averageSampleSitePosition") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("polePosition") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);			
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
		// FIXME:  Should gpml:averageAge be a gpml:TimeInstant rather than an xs:double?
		map[ PropertyName::create_gpml("averageAge") ] = 
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("locationNames") ] = 
			GET_PROP_VAL_NAME(create_gpml_string_list);

		return map;
	}	


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_unclassified_feature_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("position") ] =
			GET_PROP_VAL_NAME(create_gml_point);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_mesh_node_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();

		map[ PropertyName::create_gpml("meshPoints") ] = 
			GET_PROP_VAL_NAME(create_gml_multi_point);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_abstract_field_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_tectonic_section_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("leftPlate") ] =
			GET_PROP_VAL_NAME(create_gpml_plate_id);
		map[ PropertyName::create_gpml("rightPlate") ] =
			GET_PROP_VAL_NAME(create_gpml_plate_id);
		map[ PropertyName::create_gpml("conjugatePlateId") ] = 
			GET_PROP_VAL_NAME(create_gpml_plate_id);
		map[ PropertyName::create_gpml("reconstructionMethod") ] = 
			GET_PROP_VAL_NAME(create_gpml_reconstruction_method_enumeration);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_mid_ocean_ridge_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tectonic_section_properties();

		map[ PropertyName::create_gpml("isActive") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_subduction_zone_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tectonic_section_properties();

		map[ PropertyName::create_gpml("subductionPolarity") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("islandArc") ] =
			GET_PROP_VAL_NAME(create_gpml_feature_reference);

		map[ PropertyName::create_gpml("subductionZoneAge") ] = 
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("subductionZoneDeepDip") ] = 
			GET_PROP_VAL_NAME(create_xs_double);
		map[ PropertyName::create_gpml("subductionZoneDepth") ] = 
			GET_PROP_VAL_NAME(create_xs_double);

		map[ PropertyName::create_gpml("isActive") ] =
			GET_PROP_VAL_NAME(create_xs_boolean);

		map[ PropertyName::create_gpml("slabEdgeType") ] = 
			GET_PROP_VAL_NAME(create_xs_string);

		map[ PropertyName::create_gpml("rheaFault") ] = 
			GET_PROP_VAL_NAME(create_xs_string);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_orogenic_belt_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tectonic_section_properties();

		map[ PropertyName::create_gpml("subductionPolarity") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_transform_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tectonic_section_properties();

		map[ PropertyName::create_gpml("motion") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_passive_continental_boundary_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tectonic_section_properties();

		map[ PropertyName::create_gpml("edge") ] =
			GET_PROP_VAL_NAME(create_gpml_continental_boundary_edge_enumeration);
		map[ PropertyName::create_gpml("side") ] =
			GET_PROP_VAL_NAME(create_gpml_continental_boundary_side_enumeration);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_instantaneous_feature_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_abstract_feature_properties();

#if 0
		map[ PropertyName::create_gpml("derivedFrom") ] =
			GET_PROP_VAL_NAME(create_feature_snapshot_reference);
#endif
		map[ PropertyName::create_gpml("reconstructedTime") ] =
			GET_PROP_VAL_NAME(create_gml_time_instant);
		map[ PropertyName::create_gpml("validTime") ] =
			GET_PROP_VAL_NAME(create_gml_time_period);
		map[ PropertyName::create_gpml("reconstructionPlateId") ] =
			GET_PROP_VAL_NAME(create_gpml_plate_id);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_abstract_rock_unit_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_tangible_feature_properties();

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_basic_rock_unit_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_abstract_rock_unit_properties();

		map[ PropertyName::create_gpml("position") ] =
			GET_PROP_VAL_NAME(create_gml_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_artificial_feature_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_closed_plate_boundary_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_artificial_feature_properties();

		map[ PropertyName::create_gpml("boundary") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_closed_continental_boundary_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_artificial_feature_properties();

		map[ PropertyName::create_gpml("type") ] =
			GET_PROP_VAL_NAME(create_gpml_continental_boundary_crust_enumeration);
		map[ PropertyName::create_gpml("edge") ] =
			GET_PROP_VAL_NAME(create_gpml_continental_boundary_edge_enumeration);
		map[ PropertyName::create_gpml("boundary") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_inferred_paleo_boundary_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_artificial_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("errorBounds") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_political_boundary_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_artificial_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_old_plates_grid_mark_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_artificial_feature_properties();

		map[ PropertyName::create_gpml("unclassifiedGeometry") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("centerLineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_topological_feature_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_time_variant_feature_properties();

		return map;
	}

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_topological_closed_plate_boundary_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_topological_feature_properties();

		// NOTE: this might change to effectivePlateId
		map[ PropertyName::create_gpml("reconstructionPlateId") ] =
			GET_PROP_VAL_NAME(create_gpml_constant_value);

		map[ PropertyName::create_gpml("boundary") ] =
			GET_PROP_VAL_NAME(create_gpml_piecewise_aggregation);

		return map;
	}

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_topological_slab_boundary_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_topological_closed_plate_boundary_properties();

		map[ PropertyName::create_gpml("slabFlatLying") ] =
			GET_PROP_VAL_NAME(create_xs_boolean);

		map[ PropertyName::create_gpml("slabFlatLyingDepth") ] =
			GET_PROP_VAL_NAME(create_xs_double);

		map[ PropertyName::create_gpml("dipAngle") ] =
			GET_PROP_VAL_NAME(create_xs_double);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_topological_network_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_topological_closed_plate_boundary_properties();

		map[ PropertyName::create_gpml("interior") ] =
			GET_PROP_VAL_NAME(create_gpml_piecewise_aggregation);

		map[ PropertyName::create_gpml("shapeFactor") ] =
			GET_PROP_VAL_NAME(create_xs_double);

		map[ PropertyName::create_gpml("maxEdge") ] =
			GET_PROP_VAL_NAME(create_xs_double);

		return map;
	}

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_unclassified_topological_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_topological_feature_properties();

		// NOTE: this might change to effectivePlateId
		// Or should it be removed if unclassified topological feature type does not apply to
		// plate polygons ?
		// In which case it applies to other things like deforming subduction zones, for example,
		// where plate id means something else (or is already taken care of by the overiding plate
		// that's part of the subduction zone feature as left or right plate id).
		map[ PropertyName::create_gpml("reconstructionPlateId") ] =
			GET_PROP_VAL_NAME(create_gpml_constant_value);

		map[ PropertyName::create_gpml("boundary") ] =
			GET_PROP_VAL_NAME(create_gpml_piecewise_aggregation);

		// Unclassified topological feature can be a topological line (in addition to boundary)
		// so support the usual unclassified line geometry property name options.
		map[ PropertyName::create_gpml("centerLineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_piecewise_aggregation);
		map[ PropertyName::create_gpml("outlineOf") ] =
			GET_PROP_VAL_NAME(create_gpml_piecewise_aggregation);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] =
			GET_PROP_VAL_NAME(create_gpml_piecewise_aggregation);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_reconstruction_feature_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_abstract_feature_properties();

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_total_reconstruction_sequence_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_reconstruction_feature_properties();

		map[ PropertyName::create_gpml("fixedReferenceFrame") ] =
			GET_PROP_VAL_NAME(create_gpml_plate_id);
		map[ PropertyName::create_gpml("movingReferenceFrame") ] =
			GET_PROP_VAL_NAME(create_gpml_plate_id);
		map[ PropertyName::create_gpml("totalReconstructionPole") ] =
			GET_PROP_VAL_NAME(create_gpml_irregular_sampling);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_absolute_reference_frame_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_total_reconstruction_sequence_properties();

		map[ PropertyName::create_gpml("type") ] =
			GET_PROP_VAL_NAME(create_gpml_absolute_reference_frame_enumeration);

		return map;
	}

	
	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_raster_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_abstract_feature_properties();

		map[ PropertyName::create_gpml("domainSet") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("rangeSet") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);
		map[ PropertyName::create_gpml("bandNames") ] =
			GET_PROP_VAL_NAME(create_gpml_raster_band_names);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_scalar_field_3d_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_abstract_feature_properties();

		map[ PropertyName::create_gpml("file") ] =
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}


	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_flowline_properties()
	{

		// FIXME: Should this be a reconstructable feature?
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();					

		map[ PropertyName::create_gpml("seedPoints") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);	

		map[ PropertyName::create_gpml("times") ] =
			GET_PROP_VAL_NAME(create_gpml_array);			

		map[ PropertyName::create_gpml("reconstructionMethod") ] = 
			GET_PROP_VAL_NAME(create_gpml_reconstruction_method_enumeration);
		map[ PropertyName::create_gpml("leftPlate") ] =
			GET_PROP_VAL_NAME(create_gpml_plate_id);
		map[ PropertyName::create_gpml("rightPlate") ] =
			GET_PROP_VAL_NAME(create_gpml_plate_id);

		return map;
	}

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_motion_path_properties()
	{

		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();					

		map[ PropertyName::create_gpml("seedPoints") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);	

		map[ PropertyName::create_gpml("times") ] =
			GET_PROP_VAL_NAME(create_gpml_array);			

		map[ PropertyName::create_gpml("reconstructionMethod") ] = 
			GET_PROP_VAL_NAME(create_gpml_reconstruction_method_enumeration);

		map[ PropertyName::create_gpml("relativePlate") ] =
			GET_PROP_VAL_NAME(create_gpml_plate_id);

		return map;
	}

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_small_circle_properties()
	{
	/* 
		Some things to consider regarding small-circles-as-features:
		
		* small-circles may have been created via centre-plus-multiple-radii; do we store
		these as separate small circles, or make provision for multiple radii to be stored
		in a single feature?

		* small-circle centres may have been created via a stage pole; do we store this fact, and
		any stage-pole ingredients, so that we can re-create the stage-pole centre dynamically for
		different reconstruction trees?radius

		The simplest answers are "no" to both of the above; so the current implementation (below) stores
		only a centre and a radius. 
	*/

		//GpmlPropertyReaderUtils::PropertyCreatorMap map = get_abstract_feature_properties();	

		// Should small circles be reconstructable? I'm forcing them to be reconstructable for now
		// so that they'll get treated along with other reconstructable features.
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();	

		map[ PropertyName::create_gpml("centre") ] =
			GET_PROP_VAL_NAME(create_gml_point);

		map[ PropertyName::create_gpml("angularRadius") ] =
			GET_PROP_VAL_NAME(create_gpml_measure);

		return map;
	}

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_polygon_centroid_point_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();					
		map[ PropertyName::create_gpml("position") ] =
			GET_PROP_VAL_NAME(create_gml_point);
		map[ PropertyName::create_gpml("multiPosition") ] = 
			GET_PROP_VAL_NAME(create_gml_multi_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}

	const GpmlPropertyReaderUtils::PropertyCreatorMap
	get_displacement_point_properties()
	{
		GpmlPropertyReaderUtils::PropertyCreatorMap map = get_reconstructable_feature_properties();					
		map[ PropertyName::create_gpml("position") ] =
			GET_PROP_VAL_NAME(create_gml_point);
		map[ PropertyName::create_gpml("multiPosition") ] = 
			GET_PROP_VAL_NAME(create_gml_multi_point);
		map[ PropertyName::create_gpml("unclassifiedGeometry") ] = 
			GET_PROP_VAL_NAME(create_gpml_time_dependent_property_value);

		return map;
	}

}


GPlatesFileIO::FeaturePropertiesMap::FeaturePropertiesMap()
{
	// FIXME: As-yet unimplemented features of the GPGIM are:
	// All Instantaneous*
	
	// Instantaneous features.
	//	d_map[ FeatureType::create_gpml("InstantaneousClosedPlateBoundary") ] = 
	//		get_instantaneous_closed_plate_boundary_properties();

	// Topological features.
	d_map[ FeatureType::create_gpml("TopologicalClosedPlateBoundary") ] =
		get_topological_closed_plate_boundary_properties();
	d_map[ FeatureType::create_gpml("TopologicalSlabBoundary") ] =
		get_topological_slab_boundary_properties();
	d_map[ FeatureType::create_gpml("TopologicalNetwork") ] =
		get_topological_network_properties();
	d_map[ FeatureType::create_gpml("UnclassifiedTopologicalFeature") ] =
		get_unclassified_topological_properties();


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
	d_map[ FeatureType::create_gpml("Flowline")] =
		get_flowline_properties();
	d_map[ FeatureType::create_gpml("MotionPath")] =
		get_motion_path_properties();
	d_map[ FeatureType::create_gpml("PolygonCentroidPoint")] =
		get_polygon_centroid_point_properties();
	d_map[ FeatureType::create_gpml("DisplacementPoint")] =
		get_displacement_point_properties();
	d_map[ FeatureType::create_gpml("PoliticalBoundary")] =
		get_political_boundary_properties();
	d_map[ FeatureType::create_gpml("SmallCircle")] =
		get_small_circle_properties();


	// Rock units.
	d_map[ FeatureType::create_gpml("BasicRockUnit") ] =
		get_basic_rock_unit_properties();
		
	d_map[ FeatureType::create_gpml("RockUnit_carbonate") ] = get_basic_rock_unit_properties();
	d_map[ FeatureType::create_gpml("RockUnit_siliciclastic") ] = get_basic_rock_unit_properties();
	d_map[ FeatureType::create_gpml("RockUnit_evaporite") ] = get_basic_rock_unit_properties();
	d_map[ FeatureType::create_gpml("RockUnit_organic") ] = get_basic_rock_unit_properties();
	d_map[ FeatureType::create_gpml("RockUnit_chemical") ] = get_basic_rock_unit_properties();
	d_map[ FeatureType::create_gpml("RockUnit_plutonic") ] = get_basic_rock_unit_properties();
	d_map[ FeatureType::create_gpml("RockUnit_volcanic") ] = get_basic_rock_unit_properties();
	d_map[ FeatureType::create_gpml("RockUnit_metamorphic") ] = get_basic_rock_unit_properties();
	d_map[ FeatureType::create_gpml("RockUnit_indeterminate_igneous") ] = get_basic_rock_unit_properties();

	d_map[ FeatureType::create_gpml("FossilCollection_small") ] = get_basic_rock_unit_properties();
	d_map[ FeatureType::create_gpml("FossilCollection_medium") ] = get_basic_rock_unit_properties();
	d_map[ FeatureType::create_gpml("FossilCollection_large") ] = get_basic_rock_unit_properties();
		
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
	d_map[ FeatureType::create_gpml("SlabEdge") ] =
		get_slab_edge_properties();
	d_map[ FeatureType::create_gpml("Volcano") ] =
		get_volcano_properties();
	d_map[ FeatureType::create_gpml("Pluton") ] =
		get_pluton_properties();
	d_map[ FeatureType::create_gpml("Ophiolite") ] =
		get_ophiolite_properties();
	d_map[ FeatureType::create_gpml("NavdatSampleMafic") ] =
		get_navdat_sample_properties();
	d_map[ FeatureType::create_gpml("NavdatSampleIntermediate") ] =
		get_navdat_sample_properties();
	d_map[ FeatureType::create_gpml("NavdatSampleFelsicLow") ] =
		get_navdat_sample_properties();
	d_map[ FeatureType::create_gpml("NavdatSampleFelsicHigh") ] =
		get_navdat_sample_properties();
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

	// 3D scalar fields.
	d_map[ FeatureType::create_gpml("ScalarField3D") ] =
		get_scalar_field_3d_properties();

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

	const GpmlPropertyReaderUtils::PropertyCreatorMap &prop_map = iter->second;
	return prop_map.find(property_name) != prop_map.end();
}

