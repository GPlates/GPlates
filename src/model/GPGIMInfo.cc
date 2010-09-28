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

#include <QObject>
#include <QDebug>

#include "GPGIMInfo.h"


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

namespace
{
	using namespace GPlatesModel::GPGIMInfo;


	/**
	 * This struct is used to build a static table of all possible geometric properties
	 * we can fill in with this dialog.
	 */
	struct GeometryPropInfo
	{
		/**
		 * The name of the geometric property, without the gpml: prefix.
		 */
		const char *prop_name;
		
		/**
		 * The human-friendly name of the geometric property, made ready
		 * for translation by being wrapped in a QT_TR_NOOP() macro.
		 */
		const char *friendly_name;
		
		/**
		 * Whether the property should have a time-dependent wrapper.
		 */
		bool expects_time_dependent_wrapper;
	};


	/**
	 * Information about geometric properties we can fill in with this dialog.
	 */
	static const GeometryPropInfo geometry_prop_info_table[] = {
		{ "centerLineOf", QT_TR_NOOP("Centre line"), true },
		{ "outlineOf", QT_TR_NOOP("Outline"), true },
		{ "errorBounds", QT_TR_NOOP("Error boundary"), false },
		{ "boundary", QT_TR_NOOP("Boundary"), false },
		{ "position", QT_TR_NOOP("Position"), false },
		{ "locations", QT_TR_NOOP("Locations"), false },
		{ "unclassifiedGeometry", QT_TR_NOOP("Unclassified / miscellaneous"), true },
	};

	/**
	 * Converts the above table into a map of PropertyName -> QString.
	 */
	const geometry_prop_name_map_type &
	build_geometry_prop_name_map()
	{
		static geometry_prop_name_map_type map;

		// Add all the friendly names from the table.
		const GeometryPropInfo *it = geometry_prop_info_table;
		const GeometryPropInfo *end = it + NUM_ELEMS(geometry_prop_info_table);
		for ( ; it != end; ++it)
		{
			map.insert(std::make_pair(GPlatesModel::PropertyName::create_gpml(it->prop_name),
					QObject::tr(it->friendly_name) ));
		}
		return map;
	}

	/**
	 * Converts the above table into a map of PropertyName -> bool.
	 */
	const geometry_prop_timedependency_map_type &
	build_geometry_prop_timedependency_map()
	{
		static geometry_prop_timedependency_map_type map;

		// Add all the expects_time_dependent_wrapper flags from the table.
		const GeometryPropInfo *it = geometry_prop_info_table;
		const GeometryPropInfo *end = it + NUM_ELEMS(geometry_prop_info_table);
		for ( ; it != end; ++it)
		{
			map.insert(std::make_pair(GPlatesModel::PropertyName::create_gpml(it->prop_name),
					it->expects_time_dependent_wrapper ));
		}
		return map;
	}


	/**
	 * This struct is used to build a static table of all possible FeatureTypes we can
	 * create with this dialog.
	 */
	struct FeatureTypeInfo
	{
		/**
		 * The name of the feature, without the gpml: prefix.
		 */
		const char *gpml_type;
		
		/**
		 * The name of a geometric property you can associate with this feature.
		 */
		const char *geometric_property;
	};
	

	/**
	 * This list was created by a Python script which processed the maps of
	 *   feature-type -> feature-creation function
	 * and
	 *   feature-creation function -> property-creation function
	 * in "src/file-io/FeaturePropertiesMap.cc" (part of the GPML parser).
	 */
	static const FeatureTypeInfo feature_type_info_table[] = {
		{ "AseismicRidge", "centerLineOf" },
		{ "AseismicRidge", "outlineOf" },
		{ "AseismicRidge", "unclassifiedGeometry" },
		{ "BasicRockUnit", "outlineOf" },
		{ "BasicRockUnit", "unclassifiedGeometry" },
		{ "Basin", "outlineOf" },
		{ "Basin", "unclassifiedGeometry" },
		{ "Bathymetry", "outlineOf" },
		{ "ClosedContinentalBoundary", "boundary" },
		{ "ClosedPlateBoundary", "boundary" },
		{ "Coastline", "centerLineOf" },
		{ "Coastline", "unclassifiedGeometry" },
		{ "ComputationalMesh", "locations" },
		{ "ContinentalFragment", "outlineOf" },
		{ "ContinentalFragment", "unclassifiedGeometry" },
		{ "ContinentalRift", "centerLineOf" },
		{ "ContinentalRift", "outlineOf" },
		{ "ContinentalRift", "unclassifiedGeometry" },
		{ "Craton", "outlineOf" },
		{ "Craton", "unclassifiedGeometry" },
		{ "CrustalThickness", "outlineOf" },
		{ "DynamicTopography", "outlineOf" },
		{ "ExtendedContinentalCrust", "outlineOf" },
		{ "ExtendedContinentalCrust", "unclassifiedGeometry" },
		{ "Fault", "centerLineOf" },
		{ "Fault", "unclassifiedGeometry" },
		{ "FoldPlane", "centerLineOf" },
		{ "FoldPlane", "unclassifiedGeometry" },
		{ "FractureZone", "centerLineOf" },
		{ "FractureZone", "outlineOf" },
		{ "FractureZone", "unclassifiedGeometry" },
		{ "FractureZoneIdentification", "position" },
		{ "GeologicalLineation", "centerLineOf" },
		{ "GeologicalLineation", "unclassifiedGeometry" },
		{ "GeologicalPlane", "centerLineOf" },
		{ "GeologicalPlane", "unclassifiedGeometry" },
		{ "GlobalElevation", "outlineOf" },
		{ "Gravimetry", "outlineOf" },
		{ "HeatFlow", "outlineOf" },
		{ "HotSpot", "position" },
		{ "HotSpot", "unclassifiedGeometry" },
		{ "HotSpotTrail", "errorBounds" },
		{ "HotSpotTrail", "unclassifiedGeometry" },
		{ "InferredPaleoBoundary", "centerLineOf" },
		{ "InferredPaleoBoundary", "errorBounds" },
		{ "InferredPaleoBoundary", "unclassifiedGeometry" },
		{ "IslandArc", "outlineOf" },
		{ "IslandArc", "unclassifiedGeometry" },
		{ "Isochron", "centerLineOf" },
		{ "Isochron", "unclassifiedGeometry" },
		{ "LargeIgneousProvince", "outlineOf" },
		{ "LargeIgneousProvince", "unclassifiedGeometry" },
		{ "MagneticAnomalyIdentification", "position" },
		{ "MagneticAnomalyShipTrack", "centerLineOf" },
		{ "MagneticAnomalyShipTrack", "unclassifiedGeometry" },
		{ "Magnetics", "outlineOf" },
		{ "MantleDensity", "outlineOf" },
		{ "MidOceanRidge", "centerLineOf" },
		{ "MidOceanRidge", "outlineOf" },
		{ "MidOceanRidge", "unclassifiedGeometry" },
		{ "OceanicAge", "outlineOf" },
		{ "OldPlatesGridMark", "centerLineOf" },
		{ "OldPlatesGridMark", "unclassifiedGeometry" },
		{ "OrogenicBelt", "centerLineOf" },
		{ "OrogenicBelt", "outlineOf" },
		{ "OrogenicBelt", "unclassifiedGeometry" },
		{ "PassiveContinentalBoundary", "centerLineOf" },
		{ "PassiveContinentalBoundary", "outlineOf" },
		{ "PassiveContinentalBoundary", "unclassifiedGeometry" },
		{ "PseudoFault", "centerLineOf" },
		{ "PseudoFault", "unclassifiedGeometry" },
		{ "Roughness", "outlineOf" },
		{ "Seamount", "outlineOf" },
		{ "Seamount", "position" },
		{ "Seamount", "unclassifiedGeometry" },
		{ "SedimentThickness", "outlineOf" },
		{ "Slab", "centerLineOf" },
		{ "Slab", "outlineOf" },
		{ "Slab", "unclassifiedGeometry" },
		{ "SpreadingAsymmetry", "outlineOf" },
		{ "SpreadingRate", "outlineOf" },
		{ "Stress", "outlineOf" },
		{ "SubductionZone", "centerLineOf" },
		{ "SubductionZone", "outlineOf" },
		{ "SubductionZone", "unclassifiedGeometry" },
		{ "Suture", "centerLineOf" },
		{ "Suture", "outlineOf" },
		{ "Suture", "unclassifiedGeometry" },
		{ "TerraneBoundary", "centerLineOf" },
		{ "TerraneBoundary", "unclassifiedGeometry" },
		{ "Topography", "outlineOf" },
		{ "TopologicalClosedPlateBoundary", "boundary" },
		{ "Transform", "centerLineOf" },
		{ "Transform", "outlineOf" },
		{ "Transform", "unclassifiedGeometry" },
		{ "TransitionalCrust", "outlineOf" },
		{ "TransitionalCrust", "unclassifiedGeometry" },
		{ "UnclassifiedFeature", "centerLineOf" },
		{ "UnclassifiedFeature", "outlineOf" },
		{ "UnclassifiedFeature", "unclassifiedGeometry" },
		{ "Unconformity", "centerLineOf" },
		{ "Unconformity", "unclassifiedGeometry" },
		{ "UnknownContact", "centerLineOf" },
		{ "UnknownContact", "unclassifiedGeometry" },
		{ "Volcano", "outlineOf" },
		{ "Volcano", "position" },
		{ "Volcano", "unclassifiedGeometry" },
	};
	

	static const FeatureTypeInfo topological_feature_type_info_table[] = {
		{ "TopologicalClosedPlateBoundary", "boundary" },
	};


	/**
	 * Converts the above table into a multimap.
	 */
	const feature_geometric_prop_map_type &
	build_feature_geometric_prop_map()
	{
		static feature_geometric_prop_map_type map;

		// Add all the feature types -> geometric props from the feature_type_info_table.
		const FeatureTypeInfo *it = feature_type_info_table;
		const FeatureTypeInfo *end = it + NUM_ELEMS(feature_type_info_table);
		for ( ; it != end; ++it)
		{
			map.insert(std::make_pair(GPlatesModel::FeatureType::create_gpml(it->gpml_type),
					GPlatesModel::PropertyName::create_gpml(it->geometric_property) ));
		}
		return map;
	}


	/**
	 * Converts the above table into a set.
	 */
	feature_set_type
	build_normal_feature_set()
	{
		feature_set_type set;

		// Add all feature types from the feature_type_info_table.
		const FeatureTypeInfo *it = feature_type_info_table;
		const FeatureTypeInfo *end = it + NUM_ELEMS(feature_type_info_table);
		for ( ; it != end; ++it)
		{
			set.insert(GPlatesModel::FeatureType::create_gpml(it->gpml_type));
		}

		return set;
	}


	/**
	 * Converts the above table into a set.
	 */
	feature_set_type
	build_topological_feature_set()
	{
		feature_set_type set;

		// Add all feature types from the topological_feature_type_info_table.
		const FeatureTypeInfo *it = topological_feature_type_info_table;
		const FeatureTypeInfo *end = it + NUM_ELEMS(topological_feature_type_info_table);
		for ( ; it != end; ++it)
		{
			set.insert(GPlatesModel::FeatureType::create_gpml(it->gpml_type));
		}

		return set;
	}


	const feature_set_type &
	normal_feature_set()
	{
		static const feature_set_type feature_set = build_normal_feature_set();
		return feature_set;
	}


	const feature_set_type &
	topological_feature_set()
	{
		static const feature_set_type feature_set = build_topological_feature_set();
		return feature_set;
	}
}


const GPlatesModel::GPGIMInfo::geometry_prop_name_map_type &
GPlatesModel::GPGIMInfo::get_geometry_prop_name_map()
{
	static const geometry_prop_name_map_type &map = build_geometry_prop_name_map();
	return map;
}


const GPlatesModel::GPGIMInfo::geometry_prop_timedependency_map_type &
GPlatesModel::GPGIMInfo::get_geometry_prop_timedependency_map()
{
	static const geometry_prop_timedependency_map_type &map = build_geometry_prop_timedependency_map();
	return map;
}


const GPlatesModel::GPGIMInfo::feature_geometric_prop_map_type &
GPlatesModel::GPGIMInfo::get_feature_geometric_prop_map()
{
	static const feature_geometric_prop_map_type &map = build_feature_geometric_prop_map();
	return map;
}


const GPlatesModel::GPGIMInfo::feature_set_type &
GPlatesModel::GPGIMInfo::get_feature_set(
		bool topological)
{
	if (topological)
	{
		return topological_feature_set();
	}
	else
	{
		return normal_feature_set();
	}
}


bool
GPlatesModel::GPGIMInfo::is_topological(
		const FeatureType &feature_type)
{
	const feature_set_type &topological_features = topological_feature_set();
	return topological_features.find(feature_type) != topological_features.end();
}

