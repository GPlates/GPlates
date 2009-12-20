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

#include <algorithm>
#include <list>
#include <map>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QDebug>
#include <QString>

#include "AssignPlateIds.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/ClassifyFeatureCollection.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Reconstruct.h"
#include "app-logic/ReconstructionFeatureProperties.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/TopologyUtils.h"

#include "feature-visitors/GeometryRotator.h"
#include "feature-visitors/GeometryTypeFinder.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "maths/FiniteRotation.h"

#include "model/FeatureType.h"
#include "model/FeatureVisitor.h"
#include "model/Model.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"
#include "model/ReconstructionTree.h"
#include "model/ResolvedTopologicalGeometry.h"
#include "model/TopLevelProperty.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	static const GPlatesModel::PropertyName RECONSTRUCTION_PLATE_ID_PROPERTY_NAME =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	/**
	 * A feature needs a 'reconstructionPlateId' if it doesn't already have one *and*
	 * it is not a reconstruction feature.
	 */
	bool
	needs_reconstruction_plate_id(
			const GPlatesAppLogic::ClassifyFeatureCollection::classifications_type &classification)
	{
		return
			!GPlatesAppLogic::ClassifyFeatureCollection::found_reconstructable_feature(classification) &&
			!GPlatesAppLogic::ClassifyFeatureCollection::found_reconstruction_feature(classification);
	}


	void
	get_feature_collections_from_file_info_collection(
			const GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator_range &active_files,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &features_collection)
	{
		GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator iter = active_files.begin;
		GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator end = active_files.end;
		for ( ; iter != end; ++iter)
		{
			features_collection.push_back(iter->get_feature_collection());
		}
	}


	void
	get_active_feature_collections_from_application_state(
			GPlatesAppLogic::FeatureCollectionFileState &file_state,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
					reconstructable_features_collection,
			std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
					reconstruction_features_collection)
	{
		// Get the active reconstructable feature collections from the application state.
		get_feature_collections_from_file_info_collection(
				file_state.get_active_reconstructable_files(),
				reconstructable_features_collection);

		// Get the active reconstruction feature collections from the application state.
		get_feature_collections_from_file_info_collection(
				file_state.get_active_reconstruction_files(),
				reconstruction_features_collection);
	}


	/**
	 * Creates a new @a Reconstruction using the active reconstructable/reconstruction
	 * files in the @a ApplicationState.
	 */
	GPlatesModel::Reconstruction::non_null_ptr_type
	create_reconstruction(
			GPlatesAppLogic::FeatureCollectionFileState &file_state,
			const double &reconstruction_time,
			GPlatesModel::integer_plate_id_type anchor_plate_id)
	{
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
				reconstructable_features_collection,
				reconstruction_features_collection;

		get_active_feature_collections_from_application_state(
				file_state,
				reconstructable_features_collection,
				reconstruction_features_collection);

		// Perform a new reconstruction.
		return GPlatesAppLogic::ReconstructUtils::create_reconstruction(
				reconstructable_features_collection,
				reconstruction_features_collection,
				reconstruction_time,
				anchor_plate_id);
	}


	/**
	 * Assigns a 'gpml:reconstructionPlateId' property value to @a feature_ref.
	 * Removes any properties with this name that might already exist in @a feature_ref.
	 */
	void
	assign_reconstruction_plate_id_to_feature(
			GPlatesModel::integer_plate_id_type reconstruction_plate_id,
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
	{
		// First remove any that might already exist.
		GPlatesModel::ModelUtils::remove_property_values_from_feature(
				RECONSTRUCTION_PLATE_ID_PROPERTY_NAME, feature_ref);

		// Append a new property to the feature.
		GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_plate_id = 
				GPlatesPropertyValues::GpmlPlateId::create(reconstruction_plate_id);
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				GPlatesModel::ModelUtils::create_gpml_constant_value(gpml_plate_id,
						GPlatesPropertyValues::TemplateTypeParameterType::create_gpml("plateId")),
				RECONSTRUCTION_PLATE_ID_PROPERTY_NAME,
				feature_ref);
	}


	/**
	 * Creates a property value suitable for @a polyline and appends it
	 * to @a feature_ref with the property name @a geometry_property_name.
	 *
	 * It doesn't attempt to remove any existing properties named @a geometry_property_name.
	 */
	void
	append_polyline_to_feature(
			const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &polyline,
			const GPlatesModel::PropertyName &geometry_property_name,
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
	{
		GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
				GPlatesPropertyValues::GmlLineString::create(polyline);
		GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type gml_orientable_curve =
				GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);
		GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
				GPlatesModel::ModelUtils::create_gpml_constant_value(
						gml_orientable_curve, 
						GPlatesPropertyValues::TemplateTypeParameterType::create_gml("OrientableCurve"));
		GPlatesModel::ModelUtils::append_property_value_to_feature(
				property_value, geometry_property_name, feature_ref);
	}


	/**
	 * Returns true if @a feature_properties_iter is a geometry property.
	 */
	bool
	is_geometry_property(
			const GPlatesModel::FeatureHandle::properties_iterator &feature_properties_iter)
	{
		GPlatesFeatureVisitors::GeometryTypeFinder geom_type_finder;
		(*feature_properties_iter)->accept_visitor(geom_type_finder);

		return geom_type_finder.has_found_geometries();
	}


	/**
	 * Returns true if @a feature_properties_iter is a 'gpml:reconstructionPlateId' property.
	 */
	bool
	is_reconstruction_plate_id_property(
			const GPlatesModel::FeatureHandle::properties_iterator &feature_properties_iter)
	{
		return (*feature_properties_iter)->property_name() ==
				RECONSTRUCTION_PLATE_ID_PROPERTY_NAME;
	}


	/**
	 * Removes any properties that contain geometry from @a feature_ref.
	 */
	void
	remove_geometry_properties_from_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
	{
		// Iterate over the feature properties of the feature.
		GPlatesModel::FeatureHandle::properties_iterator feature_properties_iter =
				feature_ref->properties_begin();
		GPlatesModel::FeatureHandle::properties_iterator feature_properties_end =
				feature_ref->properties_end();
		while (feature_properties_iter != feature_properties_end)
		{
			// Increment iterator before we remove property.
			// I don't think this is currently necessary but it doesn't hurt.
			GPlatesModel::FeatureHandle::properties_iterator current_feature_properties_iter =
					feature_properties_iter;
			++feature_properties_iter;

			if(!current_feature_properties_iter.is_valid())
			{
				continue;
			}

			if (is_geometry_property(current_feature_properties_iter))
			{
				GPlatesModel::ModelUtils::remove_property_value_from_feature(
						current_feature_properties_iter, feature_ref);
				continue;
			}
		}
	}


	/**
	 * Creates a clone of @a old_feature_ref in @a feature_collection_ref.
	 *
	 * NOTE: Does not clone geometry properties and 'gpml:reconstructionPlateId' property(s).
	 */
	GPlatesModel::FeatureHandle::weak_ref
	clone_feature(
			const GPlatesModel::FeatureHandle::weak_ref &old_feature_ref,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
			GPlatesModel::ModelInterface &model)
	{
		// Create a new feature.
		const GPlatesModel::FeatureHandle::weak_ref new_feature_ref =
				model->create_feature(
						old_feature_ref->feature_type(),
						feature_collection_ref);

		// Iterate over the feature properties of the original feature
		// and clone them and append the cloned versions to the new feature.
		GPlatesModel::FeatureHandle::properties_iterator old_feature_properties_iter =
				old_feature_ref->properties_begin();
		GPlatesModel::FeatureHandle::properties_iterator old_feature_properties_end =
				old_feature_ref->properties_end();
		for ( ; old_feature_properties_iter != old_feature_properties_end;
			++old_feature_properties_iter)
		{
			if(!old_feature_properties_iter.is_valid())
			{
				continue;
			}

			// Don't clone geometry properties or 'gpml:reconstructionPlateId' properties.
			if (is_geometry_property(old_feature_properties_iter) ||
				is_reconstruction_plate_id_property(old_feature_properties_iter))
			{
				continue;
			}

			// Clone the current property value.
			const GPlatesModel::TopLevelProperty::non_null_ptr_type cloned_property_value =
					(*old_feature_properties_iter)->clone();

			// Add the cloned property value to the new feature.
			GPlatesModel::ModelUtils::append_property_value_to_feature(
					cloned_property_value, new_feature_ref);
		}

		return new_feature_ref;
	}


	/**
	 * Calculate polyline distance along unit radius sphere.
	 */
	GPlatesMaths::real_t
	calculate_polyline_distance(
			const GPlatesMaths::PolylineOnSphere &polyline)
	{
		GPlatesMaths::real_t distance = 0;

		GPlatesMaths::PolylineOnSphere::const_iterator gca_iter = polyline.begin();
		GPlatesMaths::PolylineOnSphere::const_iterator gca_end = polyline.end();
		for ( ; gca_iter != gca_end; ++gca_iter)
		{
			distance += acos(gca_iter->dot_of_endpoints());
		}

		return distance;
	}


	/**
	 * RAII class makes sure a geometry properties iterator gets removed from its feature.
	 */
	class GeometryPropertiesIteratorRemover
	{
	public:
		GeometryPropertiesIteratorRemover(
				const GPlatesModel::FeatureHandle::properties_iterator &geometry_properties_iterator) :
			d_geometry_properties_iterator(geometry_properties_iterator)
		{
		}

		~GeometryPropertiesIteratorRemover()
		{
			// Destructor cannot throw exceptions.
			try
			{
				if (d_geometry_properties_iterator.is_valid())
				{
					GPlatesModel::ModelUtils::remove_property_value_from_feature(
							d_geometry_properties_iterator,
							d_geometry_properties_iterator.collection_handle_ptr()->reference());
				}
			}
			catch (...)
			{
			}
		}

	private:
		GPlatesModel::FeatureHandle::properties_iterator d_geometry_properties_iterator;
	};


	class GeometryPropertyAllPartitionedPolylines
	{
	public:
		GeometryPropertyAllPartitionedPolylines(
				const GPlatesModel::PropertyName &geometry_property_name_,
				const GPlatesModel::FeatureHandle::properties_iterator &geometry_properties_iterator_) :
			geometry_property_name(geometry_property_name_),
			geometry_properties_iterator(geometry_properties_iterator_)
		{  }

		GPlatesModel::PropertyName geometry_property_name;
		GPlatesModel::FeatureHandle::properties_iterator geometry_properties_iterator;
		GPlatesAppLogic::TopologyUtils::resolved_boundary_partitioned_polylines_seq_type
				partitioned_inside_polylines_seq;
		GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type
				partitioned_outside_polylines;
	};
	typedef std::list<GeometryPropertyAllPartitionedPolylines>
			geometry_property_all_partitioned_polylines_seq_type;


	class GeometryPropertyPartitionedInsidePolylines
	{
	public:
		GeometryPropertyPartitionedInsidePolylines(
				const GPlatesModel::PropertyName &geometry_property_name_,
				const GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type &
						partitioned_polylines_) :
			geometry_property_name(geometry_property_name_),
			partitioned_polylines(partitioned_polylines_)
		{  }

		GPlatesModel::PropertyName geometry_property_name;
		GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type partitioned_polylines;
	};
	typedef std::list<GeometryPropertyPartitionedInsidePolylines>
			geometry_property_partitioned_inside_polylines_seq_type;

	class ResolvedBoundaryGeometryProperties
	{
	public:
		geometry_property_partitioned_inside_polylines_seq_type
				geometry_property_partitioned_inside_polylines_seq;
	};
	typedef std::map<
			const GPlatesModel::ResolvedTopologicalGeometry *,
			ResolvedBoundaryGeometryProperties>
					resolved_boundary_geometry_properties_map_type;


	/**
	 * Visits geometry properties in a feature and partitions each geometry using
	 * resolved topological boundaries.
	 */
	class PartitionFeatureGeometryVisitor :
			public GPlatesModel::FeatureVisitor,
			private boost::noncopyable
	{
	public:
		PartitionFeatureGeometryVisitor(
				geometry_property_all_partitioned_polylines_seq_type &
						geometry_property_all_partitioned_polylines_seq,
				const GPlatesAppLogic::TopologyUtils::resolved_geometries_for_geometry_partitioning_query_type &
						geometry_partition_query,
				const boost::optional<double> &reconstruction_time = boost::none) :
			d_geometry_partition_query(geometry_partition_query),
			d_reconstruction_time(reconstruction_time),
			d_geometry_property_all_partitioned_polylines_seq(
					geometry_property_all_partitioned_polylines_seq)
		{  }

	protected:
		bool
		initialise_pre_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle)
		{
			if (d_reconstruction_time)
			{
				// Gathers some useful reconstruction parameters.
				GPlatesAppLogic::ReconstructionFeatureProperties reconstruction_params(
						*d_reconstruction_time);
				reconstruction_params.visit_feature(feature_handle.reference());

				// If the feature is not defined at the reconstruction time then
				// don't visit the properties.
				if ( ! reconstruction_params.is_feature_defined_at_recon_time())
				{
					return false;
				}
			}

			// Now visit each of the properties in turn.
			return true;
		}


		void
		visit_gml_line_string(
				GPlatesPropertyValues::GmlLineString &gml_line_string)
		{
			GeometryPropertyAllPartitionedPolylines geometry_property_all_partitioned_polylines(
					*current_top_level_propname(),
					*current_top_level_propiter());

			GPlatesAppLogic::TopologyUtils::partition_polyline_using_resolved_topology_boundaries(
					geometry_property_all_partitioned_polylines.partitioned_inside_polylines_seq,
					geometry_property_all_partitioned_polylines.partitioned_outside_polylines,
					gml_line_string.polyline(),
					d_geometry_partition_query);

			d_geometry_property_all_partitioned_polylines_seq.push_back(
					geometry_property_all_partitioned_polylines);
		}


		void
		visit_gml_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
		{
			using namespace GPlatesMaths;
		}


		void
		visit_gml_orientable_curve(
				GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
		{
			gml_orientable_curve.base_curve()->accept_visitor(*this);
		}


		void
		visit_gml_point(
				GPlatesPropertyValues::GmlPoint &gml_point)
		{
			using namespace GPlatesMaths;
		}


		void
		visit_gml_polygon(
				GPlatesPropertyValues::GmlPolygon &gml_polygon)
		{
			using namespace GPlatesMaths;
		}


		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}

	private:
		GPlatesAppLogic::TopologyUtils::resolved_geometries_for_geometry_partitioning_query_type
				d_geometry_partition_query;
		boost::optional<double> d_reconstruction_time;

		geometry_property_all_partitioned_polylines_seq_type &d_geometry_property_all_partitioned_polylines_seq;
	};


	void
	map_resolved_boundaries_to_geometry_properties(
			resolved_boundary_geometry_properties_map_type &
					resolved_boundary_geometry_properties_map,
			const geometry_property_all_partitioned_polylines_seq_type &
					geometry_property_all_partitioned_polylines_seq)
	{
		geometry_property_all_partitioned_polylines_seq_type::const_iterator
				geometry_property_all_partitioned_polylines_iter =
						geometry_property_all_partitioned_polylines_seq.begin();
		geometry_property_all_partitioned_polylines_seq_type::const_iterator
				geometry_property_all_partitioned_polylines_end =
						geometry_property_all_partitioned_polylines_seq.end();
		for ( ;
			geometry_property_all_partitioned_polylines_iter !=
				geometry_property_all_partitioned_polylines_end;
			++geometry_property_all_partitioned_polylines_iter)
		{
			const GeometryPropertyAllPartitionedPolylines &
					geometry_property_all_partitioned_polylines = 
							*geometry_property_all_partitioned_polylines_iter;

			const GPlatesModel::PropertyName &geometry_property_name =
					geometry_property_all_partitioned_polylines.geometry_property_name;

			const GPlatesAppLogic::TopologyUtils::resolved_boundary_partitioned_polylines_seq_type &
					resolved_boundary_partitioned_polylines_seq =
							geometry_property_all_partitioned_polylines
									.partitioned_inside_polylines_seq;

			GPlatesAppLogic::TopologyUtils
					::resolved_boundary_partitioned_polylines_seq_type::const_iterator
							resolved_boundary_partitioned_polylines_iter =
									resolved_boundary_partitioned_polylines_seq.begin();
			GPlatesAppLogic::TopologyUtils
					::resolved_boundary_partitioned_polylines_seq_type::const_iterator
							resolved_boundary_partitioned_polylines_end =
									resolved_boundary_partitioned_polylines_seq.end();
			for ( ;
				resolved_boundary_partitioned_polylines_iter !=
					resolved_boundary_partitioned_polylines_end;
				++resolved_boundary_partitioned_polylines_iter)
			{
				const GPlatesAppLogic::TopologyUtils::ResolvedBoundaryPartitionedPolylines &
						resolved_boundary_partitioned_polylines =
								*resolved_boundary_partitioned_polylines_iter;

				const GPlatesModel::ResolvedTopologicalGeometry *resolved_topological_boundary =
						resolved_boundary_partitioned_polylines.resolved_topological_boundary;

				GeometryPropertyPartitionedInsidePolylines
						geometry_property_partitioned_inside_polylines(
								geometry_property_name,
								resolved_boundary_partitioned_polylines
										.partitioned_inside_polylines);

				resolved_boundary_geometry_properties_map[resolved_topological_boundary]
						.geometry_property_partitioned_inside_polylines_seq
						.push_back(geometry_property_partitioned_inside_polylines);
			}
		}
	}


	bool
	assign_partitioned_geometry_outside_all_resolved_boundaries(
			const geometry_property_all_partitioned_polylines_seq_type &
					geometry_property_all_partitioned_polylines_seq,
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
	{
		bool assigned_to_feature = false;

		geometry_property_all_partitioned_polylines_seq_type::const_iterator
				geometry_property_all_partitioned_polylines_iter =
						geometry_property_all_partitioned_polylines_seq.begin();
		geometry_property_all_partitioned_polylines_seq_type::const_iterator
				geometry_property_all_partitioned_polylines_end =
						geometry_property_all_partitioned_polylines_seq.end();
		for ( ;
			geometry_property_all_partitioned_polylines_iter !=
				geometry_property_all_partitioned_polylines_end;
			++geometry_property_all_partitioned_polylines_iter)
		{
			const GeometryPropertyAllPartitionedPolylines &
					geometry_property_all_partitioned_polylines = 
							*geometry_property_all_partitioned_polylines_iter;

			if (geometry_property_all_partitioned_polylines.partitioned_outside_polylines.empty())
			{
				continue;
			}

			const GPlatesModel::PropertyName &geometry_property_name =
					geometry_property_all_partitioned_polylines.geometry_property_name;
			const GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type &
					outside_polyline_seq =
						geometry_property_all_partitioned_polylines.partitioned_outside_polylines;

			// Iterate over the outside polylines.
			GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type::const_iterator
					outside_polyline_iter = outside_polyline_seq.begin();
			GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type::const_iterator
					outside_polyline_end = outside_polyline_seq.end();
			for ( ; outside_polyline_iter != outside_polyline_end; ++outside_polyline_iter)
			{
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &
						outside_polyline = *outside_polyline_iter;

				append_polyline_to_feature(
						outside_polyline, geometry_property_name, feature_ref);
			}

			assigned_to_feature = true;
		}

		return assigned_to_feature;
	}


	/**
	 * Returns true if all geometry properties in
	 * @a geometry_property_all_partitioned_polylines_seq are outside
	 * all resolved boundaries.
	 */
	bool
	is_geometry_outside_all_resolved_boundaries(
			const geometry_property_all_partitioned_polylines_seq_type &
					geometry_property_all_partitioned_polylines_seq)
	{
		geometry_property_all_partitioned_polylines_seq_type::const_iterator
				geometry_property_all_partitioned_polylines_iter =
						geometry_property_all_partitioned_polylines_seq.begin();
		geometry_property_all_partitioned_polylines_seq_type::const_iterator
				geometry_property_all_partitioned_polylines_end =
						geometry_property_all_partitioned_polylines_seq.end();
		for ( ;
			geometry_property_all_partitioned_polylines_iter !=
				geometry_property_all_partitioned_polylines_end;
			++geometry_property_all_partitioned_polylines_iter)
		{
			const GeometryPropertyAllPartitionedPolylines &
					geometry_property_all_partitioned_polylines = 
							*geometry_property_all_partitioned_polylines_iter;

			if (!geometry_property_all_partitioned_polylines
				.partitioned_inside_polylines_seq.empty())
			{
				// We found some inside polylines which means not all geometries
				// are outside all resolved boundaries.
				return false;
			}
		}

		return true;
	}


	/**
	 * Iterate over the resolved boundaries and calculate the line distance
	 * of all polyline segments partitioned into each plate and return the
	 * resolved boundary of the plate with the maximum distance.
	 *
	 * Returns true if a resolved boundary was found that also has a valid plate id
	 * (which they all should so this function should always return true).
	 */
	bool
	find_resolved_boundary_with_valid_plate_id_that_contains_most_geometry(
			const GPlatesModel::ResolvedTopologicalGeometry *&resolved_boundary_containing_most_geometry,
			const resolved_boundary_geometry_properties_map_type &resolved_boundary_geometry_properties_map)
	{
		resolved_boundary_containing_most_geometry = NULL;

		if (resolved_boundary_geometry_properties_map.empty())
		{
			// We shouldn't get here as shouldn't have been called with an empty sequence.
			return false;
		}

		// Shortcut if there's only one resolved boundary since we don't need
		// to calculate the partitioned geometry distances to find the maximum.
		if (resolved_boundary_geometry_properties_map.size() == 1)
		{
			resolved_boundary_containing_most_geometry =
					resolved_boundary_geometry_properties_map.begin()->first;

			if (!resolved_boundary_containing_most_geometry->plate_id())
			{
				// Each resolved boundary should have a plate id so this shouldn't happen.
				return false;
			}
			return true;
		}

		GPlatesMaths::real_t max_resolved_boundary_partitioned_geometry_distance = 0;
		resolved_boundary_geometry_properties_map_type::const_iterator
				resolved_boundary_containing_most_geometry_iter =
						resolved_boundary_geometry_properties_map.end();

		// Iterate over the resolved boundaries and determine which one contains
		// the most geometry from all geometry properties of the feature being tested.
		resolved_boundary_geometry_properties_map_type::const_iterator
				resolved_boundary_geometry_properties_iter = 
						resolved_boundary_geometry_properties_map.begin();
		resolved_boundary_geometry_properties_map_type::const_iterator
				resolved_boundary_geometry_properties_end = 
						resolved_boundary_geometry_properties_map.end();
		for ( ;
			resolved_boundary_geometry_properties_iter !=
				resolved_boundary_geometry_properties_end;
			++resolved_boundary_geometry_properties_iter)
		{
			const GPlatesModel::ResolvedTopologicalGeometry *resolved_topological_boundary =
					resolved_boundary_geometry_properties_iter->first;
			const ResolvedBoundaryGeometryProperties &resolved_boundary_geometry_properties =
					resolved_boundary_geometry_properties_iter->second;

			if (!resolved_topological_boundary->plate_id())
			{
				// Each resolved boundary should have a plate id so this shouldn't happen.
				continue;
			}

			GPlatesMaths::real_t resolved_boundary_partitioned_geometry_distance = 0;

			// Iterate over the geometry properties that have partitioned polylines
			// for the current resolved boundary.
			const geometry_property_partitioned_inside_polylines_seq_type &
					geometry_property_partitioned_inside_polylines_seq =
							resolved_boundary_geometry_properties
									.geometry_property_partitioned_inside_polylines_seq;
			geometry_property_partitioned_inside_polylines_seq_type::const_iterator
					geometry_property_partitioned_inside_polylines_iter =
							geometry_property_partitioned_inside_polylines_seq.begin();
			geometry_property_partitioned_inside_polylines_seq_type::const_iterator
					geometry_property_partitioned_inside_polylines_end =
							geometry_property_partitioned_inside_polylines_seq.end();
			for ( ;
				geometry_property_partitioned_inside_polylines_iter !=
					geometry_property_partitioned_inside_polylines_end;
				++geometry_property_partitioned_inside_polylines_iter)
			{
				const GeometryPropertyPartitionedInsidePolylines &
						geometry_property_partitioned_inside_polylines =
								*geometry_property_partitioned_inside_polylines_iter;

				const GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type &
						partitioned_inside_polyline_seq =
								geometry_property_partitioned_inside_polylines
										.partitioned_polylines;

				// Iterate over the partitioned polylines of the current geometry.
				GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type::const_iterator
						partitioned_inside_polyline_iter = partitioned_inside_polyline_seq.begin();
				GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type::const_iterator
						paritioned_inside_polyline_end = partitioned_inside_polyline_seq.end();
				for ( ;
					partitioned_inside_polyline_iter != paritioned_inside_polyline_end;
					++partitioned_inside_polyline_iter)
				{
					const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &
							inside_polyline = *partitioned_inside_polyline_iter;

					resolved_boundary_partitioned_geometry_distance +=
							calculate_polyline_distance(*inside_polyline);
				}
			}

			if (resolved_boundary_partitioned_geometry_distance >
					max_resolved_boundary_partitioned_geometry_distance)
			{
				max_resolved_boundary_partitioned_geometry_distance =
						resolved_boundary_partitioned_geometry_distance;

				resolved_boundary_containing_most_geometry_iter =
						resolved_boundary_geometry_properties_iter;
			}
		}

		if (resolved_boundary_containing_most_geometry_iter ==
			resolved_boundary_geometry_properties_map.end())
		{
			// We shouldn't get here since we should have at least one
			// resolved boundary and it should have at least one partitioned polyline.
			return false;
		}

		resolved_boundary_containing_most_geometry =
				resolved_boundary_containing_most_geometry_iter->first;

		return true;
	}


	/**
	 * Iterate over the resolved boundaries and calculate the line distance
	 * of all polyline segments partitioned into each plate and return the
	 * resolved boundary of the plate with the maximum distance.
	 *
	 * Returns true if a resolved boundary was found that also has a valid plate id
	 * (which they all should so this function should always return true).
	 */
	bool
	find_resolved_boundary_with_valid_plate_id_that_contains_most_geometry(
			const GPlatesModel::ResolvedTopologicalGeometry *&resolved_boundary_containing_most_geometry,
			const GPlatesAppLogic::TopologyUtils::resolved_boundary_partitioned_polylines_seq_type &
					resolved_boundary_partitioned_polylines_seq)
	{
		resolved_boundary_containing_most_geometry = NULL;

		if (resolved_boundary_partitioned_polylines_seq.empty())
		{
			// We shouldn't get here as shouldn't have been called with an empty sequence.
			return false;
		}

		// Shortcut if there's only one resolved boundary since we don't need
		// to calculate the partitioned geometry distances to find the maximum.
		if (resolved_boundary_partitioned_polylines_seq.size() == 1)
		{
			resolved_boundary_containing_most_geometry =
					resolved_boundary_partitioned_polylines_seq.front().resolved_topological_boundary;

			if (!resolved_boundary_containing_most_geometry->plate_id())
			{
				// Each resolved boundary should have a plate id so this shouldn't happen.
				return false;
			}
			return true;
		}

		GPlatesMaths::real_t max_resolved_boundary_partitioned_geometry_distance = 0;

		// Iterate over the resolved boundaries that the current geometry property
		// was partitioned into and determine which one contains the most geometry
		// from the current geometry property.
		GPlatesAppLogic::TopologyUtils
				::resolved_boundary_partitioned_polylines_seq_type::const_iterator
						resolved_boundary_partitioned_polylines_iter =
								resolved_boundary_partitioned_polylines_seq.begin();
		GPlatesAppLogic::TopologyUtils
				::resolved_boundary_partitioned_polylines_seq_type::const_iterator
						resolved_boundary_partitioned_polylines_end =
								resolved_boundary_partitioned_polylines_seq.end();

		for ( ;
			resolved_boundary_partitioned_polylines_iter !=
				resolved_boundary_partitioned_polylines_end;
			++resolved_boundary_partitioned_polylines_iter)
		{
			const GPlatesAppLogic::TopologyUtils::ResolvedBoundaryPartitionedPolylines &
					resolved_boundary_partitioned_polylines =
							*resolved_boundary_partitioned_polylines_iter;

			const GPlatesModel::ResolvedTopologicalGeometry *resolved_topological_boundary =
					resolved_boundary_partitioned_polylines.resolved_topological_boundary;

			if (!resolved_topological_boundary->plate_id())
			{
				// Each resolved boundary should have a plate id so this shouldn't happen.
				continue;
			}

			const GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type &
					partitioned_inside_polyline_seq =
							resolved_boundary_partitioned_polylines.partitioned_inside_polylines;

			GPlatesMaths::real_t resolved_boundary_partitioned_geometry_distance = 0;

			// Iterate over the partitioned inside polylines of the current
			// resolved boundary of the current geometry property.
			GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type::const_iterator
					partitioned_inside_polyline_iter = partitioned_inside_polyline_seq.begin();
			GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type::const_iterator
					paritioned_inside_polyline_end = partitioned_inside_polyline_seq.end();
			for ( ;
				partitioned_inside_polyline_iter != paritioned_inside_polyline_end;
				++partitioned_inside_polyline_iter)
			{
				const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &
						inside_polyline = *partitioned_inside_polyline_iter;

				resolved_boundary_partitioned_geometry_distance +=
						calculate_polyline_distance(*inside_polyline);
			}

			if (resolved_boundary_partitioned_geometry_distance >
					max_resolved_boundary_partitioned_geometry_distance)
			{
				max_resolved_boundary_partitioned_geometry_distance =
						resolved_boundary_partitioned_geometry_distance;

				resolved_boundary_containing_most_geometry = resolved_topological_boundary;
			}
		}

		if (resolved_boundary_containing_most_geometry == NULL)
		{
			// We shouldn't get here since we should have at least one
			// resolved boundary and it should have at least one partitioned polyline
			// in the current geometry property.
			return false;
		}

		return true;
	}


	/**
	 * Iterates over the geometry properties and see if any of the geometry was
	 * outside of all the resolved boundaries; if this happens to any of the
	 * geometry properties then we'll keep the feature for storing these outside
	 * geometries and we'll not assign a plate id to the feature; otherwise
	 * we are free to use it for geometry inside a resolved boundary;
	 * any remaining inside geometries will get stored in cloned features.
	 */
	bool
	assign_feature_to_plate_it_overlaps_the_most(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
			const geometry_property_all_partitioned_polylines_seq_type &
					geometry_property_all_partitioned_polylines_seq,
			const resolved_boundary_geometry_properties_map_type &
					resolved_boundary_geometry_properties_map,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
			GPlatesModel::ModelInterface &model,
			const double &reconstruction_time,
			const GPlatesModel::ReconstructionTree &reconstruction_tree)
	{
		// Start out by removing the 'gpml:reconstructionPlateId' property.
		GPlatesModel::ModelUtils::remove_property_values_from_feature(
				RECONSTRUCTION_PLATE_ID_PROPERTY_NAME, feature_ref);

		// If no geometry is partitioned into any plates then simply return.
		// Note: We don't have to reverse reconstruct its geometry because
		// this feature  has not been assigned a plate id and therefore
		// is now considered to be present day geometry.
		if (is_geometry_outside_all_resolved_boundaries(
				geometry_property_all_partitioned_polylines_seq))
		{
			return false;
		}

		//
		// Iterate over the resolved boundaries and calculate the line distance
		// of all polyline segments partitioned into each plate.
		// Give the feature the plate id of the plate that has the
		// largest distance contains the most geometry of the feature).
		//
		const GPlatesModel::ResolvedTopologicalGeometry *
				resolved_boundary_containing_most_geometry = NULL;
		if (!find_resolved_boundary_with_valid_plate_id_that_contains_most_geometry(
				resolved_boundary_containing_most_geometry,
				resolved_boundary_geometry_properties_map))
		{
			// We shouldn't get here since we should have at least one resolved boundary
			// with a valid plate id and it should have at least one partitioned polyline.
			return false;
		}

		// Assign feature to plate that contains largest proportion of
		// feature's geometry.
		const GPlatesModel::integer_plate_id_type reconstruction_plate_id =
				*resolved_boundary_containing_most_geometry->plate_id();

		// Get the composed absolute rotation needed to reverse reconstruct
		// geometries to present day.
		const GPlatesMaths::FiniteRotation reverse_rotation =
				GPlatesMaths::get_reverse(
						reconstruction_tree.get_composed_absolute_rotation(
								reconstruction_plate_id).first);

		// Visit the feature and rotate all its geometry using 'reverse_rotation'.
		GPlatesFeatureVisitors::GeometryRotator geometry_rotator(reverse_rotation);
		geometry_rotator.visit_feature(feature_ref);

		// Assign the plate id to the feature.
		assign_reconstruction_plate_id_to_feature(reconstruction_plate_id, feature_ref);

		return true;
	}


	/**
	 * Iterates over the geometry properties and for each one assigns to
	 * plate that its geometry overlaps the most; if any of them are
	 * outside of all the resolved boundaries then no plate id assigned for those
	 * geometry properties.
	 */
	bool
	assign_feature_sub_geometry_to_plate_it_overlaps_the_most(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
			const geometry_property_all_partitioned_polylines_seq_type &
					geometry_property_all_partitioned_polylines_seq,
			const resolved_boundary_geometry_properties_map_type &
					resolved_boundary_geometry_properties_map,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
			GPlatesModel::ModelInterface &model,
			const double &reconstruction_time,
			const GPlatesModel::ReconstructionTree &reconstruction_tree)
	{
		// Start out by removing the 'gpml:reconstructionPlateId' property.
		// But don't remove any geometry properties because we are going to clone
		// them as we go and put them in new cloned features.
		GPlatesModel::ModelUtils::remove_property_values_from_feature(
				RECONSTRUCTION_PLATE_ID_PROPERTY_NAME, feature_ref);

		bool assigned_any_plate_ids = false;
		bool has_original_feature_been_assigned_to = false;

		// Maps keeps track of which reconstruction plate id has been
		// mapped to which feature.
		typedef std::map<
				GPlatesModel::integer_plate_id_type,
				GPlatesModel::FeatureHandle::weak_ref> recon_plate_id_to_feature_map_type;
		recon_plate_id_to_feature_map_type recon_plate_id_to_feature_map;

		// Keeps track of the feature used to store geometry properties that
		// will not get assigned any plate id (because their geometry is outside
		// all resolved boundaries).
		// Is initially set to empty.
		GPlatesModel::FeatureHandle::weak_ref no_plate_id_feature_ref;

		// Iterate over the geometry properties.
		geometry_property_all_partitioned_polylines_seq_type::const_iterator
				geometry_property_all_partitioned_polylines_iter =
						geometry_property_all_partitioned_polylines_seq.begin();
		geometry_property_all_partitioned_polylines_seq_type::const_iterator
				geometry_property_all_partitioned_polylines_end =
						geometry_property_all_partitioned_polylines_seq.end();
		for ( ;
			geometry_property_all_partitioned_polylines_iter !=
				geometry_property_all_partitioned_polylines_end;
			++geometry_property_all_partitioned_polylines_iter)
		{
			const GeometryPropertyAllPartitionedPolylines &
					geometry_property_all_partitioned_polylines = 
							*geometry_property_all_partitioned_polylines_iter;

			GPlatesModel::FeatureHandle::properties_iterator geometry_properties_iterator =
					geometry_property_all_partitioned_polylines.geometry_properties_iterator;

			// Make sure geometry property gets removed from the original feature
			// before continuing to the next geometry property.
			GeometryPropertiesIteratorRemover geometry_properties_iterator_remover(
					geometry_properties_iterator);

			if (geometry_property_all_partitioned_polylines.partitioned_inside_polylines_seq.empty())
			{
				// There were no inside polylines which means the current geometry
				// property is outside all resolved boundaries.
				// Hence we cannot assign a plate id to it and so cannot
				// reverse reconstruct to present day.

				// Clone the current source geometry property value.
				const GPlatesModel::TopLevelProperty::non_null_ptr_type cloned_geometry_property_value =
						(*geometry_properties_iterator)->clone();

				// See if there's already a feature that's used for no plate id geometry.
				if (!no_plate_id_feature_ref.is_valid())
				{
					// If the original feature hasn't been used yet then use it.
					if (!has_original_feature_been_assigned_to)
					{
						// Use the original feature since it hasn't been used yet instead
						// of cloning to get a new one.
						no_plate_id_feature_ref = feature_ref;
						has_original_feature_been_assigned_to = true;
					}
					else
					{
						no_plate_id_feature_ref = clone_feature(
								feature_ref,
								feature_collection,
								model);
					}
				}

				// Add the cloned geometry property value to the feature that has no plate id.
				GPlatesModel::ModelUtils::append_property_value_to_feature(
						cloned_geometry_property_value, no_plate_id_feature_ref);

				continue;
			}

			const GPlatesModel::ResolvedTopologicalGeometry *
					resolved_boundary_containing_most_geometry = NULL;
			if (!find_resolved_boundary_with_valid_plate_id_that_contains_most_geometry(
					resolved_boundary_containing_most_geometry,
					geometry_property_all_partitioned_polylines.partitioned_inside_polylines_seq))
			{
				// We shouldn't get here since we should have at least one
				// resolved boundary and it should have at least one partitioned polyline
				// in the current geometry property.
				continue;
			}

			// Assign geometry property to plate that contains largest proportion of
			// its geometry.
			const GPlatesModel::integer_plate_id_type reconstruction_plate_id =
					*resolved_boundary_containing_most_geometry->plate_id();

			// Get the composed absolute rotation needed to reverse reconstruct
			// geometries to present day.
			const GPlatesMaths::FiniteRotation reverse_rotation =
					GPlatesMaths::get_reverse(
							reconstruction_tree.get_composed_absolute_rotation(
									reconstruction_plate_id).first);

			// Clone the current source geometry property value.
			const GPlatesModel::TopLevelProperty::non_null_ptr_type cloned_geometry_property_value =
					(*geometry_properties_iterator)->clone();

			// Visit the geometry property and rotate its geometry using 'reverse_rotation'.
			GPlatesFeatureVisitors::GeometryRotator geometry_rotator(reverse_rotation);
			cloned_geometry_property_value->accept_visitor(geometry_rotator);

			GPlatesModel::FeatureHandle::weak_ref recon_plate_id_feature_ref;

			// See if there's already a feature that's been assigned to our
			// reconstruction plate id.
			recon_plate_id_to_feature_map_type::const_iterator plate_id_to_feature_map_iter =
					recon_plate_id_to_feature_map.find(reconstruction_plate_id);
			if (plate_id_to_feature_map_iter != recon_plate_id_to_feature_map.end())
			{
				recon_plate_id_feature_ref = plate_id_to_feature_map_iter->second;
			}
			// If the original feature hasn't been used yet then use it.
			else if (!has_original_feature_been_assigned_to)
			{
				// Use the original feature since it hasn't been used yet instead
				// of cloning to get a new one.
				recon_plate_id_feature_ref = feature_ref;
				has_original_feature_been_assigned_to = true;
			}
			else
			{
				recon_plate_id_feature_ref = clone_feature(
						feature_ref,
						feature_collection,
						model);
			}
			// Make sure we register the feature in the map (if isn't already).
			recon_plate_id_to_feature_map[reconstruction_plate_id] =
					recon_plate_id_feature_ref;

			// Add the cloned and rotated geometry property value to the feature
			// that has the reconstruction plate id we are assigning.
			GPlatesModel::ModelUtils::append_property_value_to_feature(
					cloned_geometry_property_value, recon_plate_id_feature_ref);

			// Assign the plate id to the new feature.
			assign_reconstruction_plate_id_to_feature(
					reconstruction_plate_id, recon_plate_id_feature_ref);

			assigned_any_plate_ids = true;
		}

		return assigned_any_plate_ids;
	}


	/**
	 * Iterates over the geometry properties and see if any of the geometry was
	 * outside of all the resolved boundaries; if this happens to any of the
	 * geometry properties then we'll keep the feature for storing these outside
	 * geometries and we'll not assign a plate id to the feature; otherwise
	 * we are free to use it for geometry inside a resolved boundary;
	 * any remaining inside geometries will get stored in cloned features.
	 */
	bool
	partition_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
			const geometry_property_all_partitioned_polylines_seq_type &
					geometry_property_all_partitioned_polylines_seq,
			const resolved_boundary_geometry_properties_map_type &
					resolved_boundary_geometry_properties_map,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
			GPlatesModel::ModelInterface &model,
			const double &reconstruction_time,
			const GPlatesModel::ReconstructionTree &reconstruction_tree)
	{
		bool assigned_any_plate_ids = false;

		// First strip off any geometry properties from the feature.
		// Also remove any 'gpml:reconstructionPlateId' property(s).
		// This is so we can add new geometry properties and possibly
		// add a plate id (if inside a resolved boundary) later.
		remove_geometry_properties_from_feature(feature_ref);
		GPlatesModel::ModelUtils::remove_property_values_from_feature(
				RECONSTRUCTION_PLATE_ID_PROPERTY_NAME, feature_ref);

		// Append any polyline geometries that are partitioned
		// outside all resolved boundaries.
		bool has_original_feature_been_assigned_to =
				assign_partitioned_geometry_outside_all_resolved_boundaries(
						geometry_property_all_partitioned_polylines_seq,
						feature_ref);

		//
		// Iterate over the resolved boundaries and, for all except the first one,
		// create a clone of the feature.
		// Only the non-geometry properties should be cloned because we are going
		// to insert the partitioned geometries into the cloned features.
		//

		resolved_boundary_geometry_properties_map_type::const_iterator
				resolved_boundary_geometry_properties_iter = 
						resolved_boundary_geometry_properties_map.begin();
		resolved_boundary_geometry_properties_map_type::const_iterator
				resolved_boundary_geometry_properties_end = 
						resolved_boundary_geometry_properties_map.end();
		for ( ;
			resolved_boundary_geometry_properties_iter !=
				resolved_boundary_geometry_properties_end;
			++resolved_boundary_geometry_properties_iter)
		{
			const GPlatesModel::ResolvedTopologicalGeometry *resolved_topological_boundary =
					resolved_boundary_geometry_properties_iter->first;
			const ResolvedBoundaryGeometryProperties &resolved_boundary_geometry_properties =
					resolved_boundary_geometry_properties_iter->second;

			if (!resolved_topological_boundary->plate_id())
			{
				// Each resolved boundary should have a plate id so this shouldn't happen.
				continue;
			}

			const GPlatesModel::integer_plate_id_type reconstruction_plate_id =
					*resolved_topological_boundary->plate_id();

			// Get the composed absolute rotation needed to reverse reconstruct
			// geometries to present day. It'll get used multiple times so
			// calculate it once here.
			const GPlatesMaths::FiniteRotation reverse_rotation =
					GPlatesMaths::get_reverse(
							reconstruction_tree.get_composed_absolute_rotation(
									reconstruction_plate_id).first);

			GPlatesModel::FeatureHandle::weak_ref new_feature_ref;

			// If the original feature hasn't been used yet then use it.
			if (!has_original_feature_been_assigned_to)
			{
				// Use the original feature since it hasn't been used yet instead
				// of cloning to get a new one.
				new_feature_ref = feature_ref;
				has_original_feature_been_assigned_to = true;
			}
			else
			{
				new_feature_ref = clone_feature(
						feature_ref,
						feature_collection,
						model);
			}

			// Iterate over the geometry properties that have partitioned polylines
			// for the current resolved boundary.
			const geometry_property_partitioned_inside_polylines_seq_type &
					geometry_property_partitioned_inside_polylines_seq =
							resolved_boundary_geometry_properties
									.geometry_property_partitioned_inside_polylines_seq;
			geometry_property_partitioned_inside_polylines_seq_type::const_iterator
					geometry_property_partitioned_inside_polylines_iter =
							geometry_property_partitioned_inside_polylines_seq.begin();
			geometry_property_partitioned_inside_polylines_seq_type::const_iterator
					geometry_property_partitioned_inside_polylines_end =
							geometry_property_partitioned_inside_polylines_seq.end();
			for ( ;
				geometry_property_partitioned_inside_polylines_iter !=
					geometry_property_partitioned_inside_polylines_end;
				++geometry_property_partitioned_inside_polylines_iter)
			{
				const GeometryPropertyPartitionedInsidePolylines &
						geometry_property_partitioned_inside_polylines =
								*geometry_property_partitioned_inside_polylines_iter;

				const GPlatesModel::PropertyName &geometry_property_name =
						geometry_property_partitioned_inside_polylines.geometry_property_name;

				const GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type &
						partitioned_inside_polyline_seq =
								geometry_property_partitioned_inside_polylines
										.partitioned_polylines;

				GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type::const_iterator
						partitioned_inside_polyline_iter = partitioned_inside_polyline_seq.begin();
				GPlatesAppLogic::TopologyUtils::partitioned_polyline_seq_type::const_iterator
						paritioned_inside_polyline_end = partitioned_inside_polyline_seq.end();
				for ( ;
					partitioned_inside_polyline_iter != paritioned_inside_polyline_end;
					++partitioned_inside_polyline_iter)
				{
					const GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type &
							inside_polyline = *partitioned_inside_polyline_iter;

					// If the reconstruction time is not present day then we'll need to
					// reverse reconstruct back to present day before storing in feature.
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type
							reverse_reconstructed_inside_polyline =
									(reconstruction_time != 0)
									? reverse_rotation * inside_polyline
									: inside_polyline;

					// Append a polyline property to the new feature.
					append_polyline_to_feature(
							reverse_reconstructed_inside_polyline,
							geometry_property_name,
							new_feature_ref);
				}
			}

			// Now assign the reconstruction plate id to the feature.
			assign_reconstruction_plate_id_to_feature(
					reconstruction_plate_id, new_feature_ref);

			assigned_any_plate_ids = true;
		}

		return assigned_any_plate_ids;
	}


	/**
	 * Interface for a task that can be queried to see if it can assign a plate id
	 * to a specific feature and asked to assign the plate id.
	 */
	class AssignPlateIdTask
	{
	public:
		virtual
		~AssignPlateIdTask()
		{  }

		virtual
		bool
		can_assign_plate_id_to_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref) = 0;

		/**
		 * Assigned reconstruction plate id to @a feature_ref and any clones of it
		 * created to support different partitioned geometries.
		 *
		 * Returns true if @a feature_ref or any of its clones were assigned plate ids.
		 */
		virtual
		bool
		assign_plate_id_to_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesAppLogic::TopologyUtils::resolved_geometries_for_geometry_partitioning_query_type &
						geometry_partition_query) = 0;
	};

	typedef boost::shared_ptr<AssignPlateIdTask> assign_plate_id_task_ptr_type;
	typedef std::vector<assign_plate_id_task_ptr_type> assign_plate_id_task_ptr_seq_type;


	/**
	 * Generic assign plate id task that gets all geometry properties in a feature
	 * .
	 */
	class GenericAssignPlateIdTask :
			public AssignPlateIdTask
	{
	public:
		GenericAssignPlateIdTask(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
				const GPlatesModel::ModelInterface &model,
				const double &reconstruction_time,
				const GPlatesModel::ReconstructionTree &reconstruction_tree,
				GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType assign_plate_id_method) :
			d_feature_collection(feature_collection),
			d_model(model),
			d_reconstruction_time(reconstruction_time),
			d_reconstruction_tree(reconstruction_tree),
			d_assign_plate_id_method(assign_plate_id_method)
		{  }


		bool
		can_assign_plate_id_to_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
		{
			return feature_ref.is_valid();
		}


		bool
		assign_plate_id_to_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesAppLogic::TopologyUtils::resolved_geometries_for_geometry_partitioning_query_type &
						geometry_partition_query)
		{
			// Iterate over the geometry properties in the feature and partition them
			// using 'geometry_partition_query'.
			geometry_property_all_partitioned_polylines_seq_type
					geometry_property_all_partitioned_polylines_seq;
			PartitionFeatureGeometryVisitor partition_feature_geom_visitor(
					geometry_property_all_partitioned_polylines_seq,
					geometry_partition_query);
			partition_feature_geom_visitor.visit_feature(feature_ref);

			if (geometry_property_all_partitioned_polylines_seq.empty())
			{
				// There were no geometry properties so we can't assign plate ids.
				return false;
			}
			// We now have a sequence mapping geometry properties to resolved boundaries.

			//
			// Get a sequence mapping resolved boundaries to geometry properties -
			// each resolved boundary has its own plate id and
			// that's what we will be assigning to the partitioned features.
			//

			resolved_boundary_geometry_properties_map_type
					resolved_boundary_geometry_properties_map;
			map_resolved_boundaries_to_geometry_properties(
					resolved_boundary_geometry_properties_map,
					geometry_property_all_partitioned_polylines_seq);

			bool assigned_any_plate_ids = false;

			// Assigned plate ids based on the method selected by the caller.
			switch (d_assign_plate_id_method)
			{
			case GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE:
				assigned_any_plate_ids = assign_feature_to_plate_it_overlaps_the_most(
						feature_ref,
						geometry_property_all_partitioned_polylines_seq,
						resolved_boundary_geometry_properties_map,
						d_feature_collection,
						d_model,
						d_reconstruction_time,
						d_reconstruction_tree);
				break;

			case GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_SUB_GEOMETRY_TO_MOST_OVERLAPPING_PLATE:
				assigned_any_plate_ids = assign_feature_sub_geometry_to_plate_it_overlaps_the_most(
						feature_ref,
						geometry_property_all_partitioned_polylines_seq,
						resolved_boundary_geometry_properties_map,
						d_feature_collection,
						d_model,
						d_reconstruction_time,
						d_reconstruction_tree);
				break;

			case GPlatesAppLogic::AssignPlateIds::PARTITION_FEATURE:
				assigned_any_plate_ids = partition_feature(
						feature_ref,
						geometry_property_all_partitioned_polylines_seq,
						resolved_boundary_geometry_properties_map,
						d_feature_collection,
						d_model,
						d_reconstruction_time,
						d_reconstruction_tree);
				break;

			default:
				// Shouldn't get here.
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}

			return assigned_any_plate_ids;
		}

	private:
		GPlatesModel::FeatureCollectionHandle::weak_ref d_feature_collection;
		GPlatesModel::ModelInterface d_model;
		double d_reconstruction_time;
		const GPlatesModel::ReconstructionTree &d_reconstruction_tree;
		GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType d_assign_plate_id_method;
	};


	/**
	 * Tasks for assigning plate ids to a VirtualGeomagneticPole feature.
	 */
	class VgpAssignPlateIdTask :
			public AssignPlateIdTask
	{
	public:
		bool
		can_assign_plate_id_to_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
		{
			// See if the feature is a VirtualGeomagneticPole.
			static const GPlatesModel::FeatureType vgp_feature_type = 
					GPlatesModel::FeatureType::create_gpml("VirtualGeomagneticPole");

			return feature_ref->feature_type() == vgp_feature_type;
		}


		bool
		assign_plate_id_to_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesAppLogic::TopologyUtils::resolved_geometries_for_geometry_partitioning_query_type &
						geometry_partition_query)
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
				return false;
			}
			const GPlatesMaths::PointOnSphere &sample_site_point = *sample_site_gml_point->point();

			// Find all resolved boundaries that contain the sample site point.
			GPlatesAppLogic::TopologyUtils::resolved_topological_geometry_seq_type resolved_boundary_seq;
			if (!GPlatesAppLogic::TopologyUtils::find_resolved_topologies_containing_point(
					resolved_boundary_seq,
					sample_site_point,
					geometry_partition_query))
			{
				qDebug() << "WARNING: Unable to assign 'reconstructionPlateId' to "
						"'VirtualGeomagneticPole' with feature id = ";
				qDebug() << GPlatesUtils::make_qstring_from_icu_string(
						feature_ref->feature_id().get());
				qDebug() << "because it's sample site is not inside any topological "
						"closed plate boundaries.";
				return false;
			}

			// Find the reconstruction plate id to use for the VGP feature.
			const boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id_opt =
					GPlatesAppLogic::TopologyUtils::
							find_reconstruction_plate_id_furthest_from_anchor_in_plate_circuit(
									resolved_boundary_seq);
			if (!reconstruction_plate_id_opt)
			{
				qDebug() << "WARNING: Unable to find 'gpml:reconstructionPlateId' property "
						"in 'VirtualGeomagneticPole' with feature id = ";
				qDebug() << GPlatesUtils::make_qstring_from_icu_string(
						feature_ref->feature_id().get());
				// This shouldn't really happen since all resolved boundaries should
				// have a reconstruction plate id.
				return false;
			}
			const GPlatesModel::integer_plate_id_type reconstruction_plate_id =
					*reconstruction_plate_id_opt;

			// Now assign the reconstruction plate id to the feature.
			assign_reconstruction_plate_id_to_feature(
					reconstruction_plate_id, feature_ref);

			// NOTE: This paleomag data is present day data - even though the VGP
			// has an age (corresponding to the rock sample age) the location of the sample
			// site and the VGP are actually present day locations.
			// So we don't assume that the reconstruction time (of the resolved boundaries)
			// corresponds to VGP locations at that time. All VGP locations are present day
			// and so it only makes sense for the user to have resolved boundaries at
			// present day.

			return true;
		}
	};


	/**
	 * Register all assign plate id tasks here.
	 */
	void
	get_all_assign_plate_id_tasks(
			assign_plate_id_task_ptr_seq_type &assign_plate_id_tasks,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
			const GPlatesModel::ModelInterface &model,
			const double &reconstruction_time,
			const GPlatesModel::ReconstructionTree &reconstruction_tree,
			GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType assign_plate_id_method)
	{
		// Order the tasks from most specific to least specific
		// since they'll get processed from front to back of the list.

		// VirtualGeomagneticPole task.
		assign_plate_id_tasks.push_back(
				assign_plate_id_task_ptr_type(
						new VgpAssignPlateIdTask()));

		// Generic default task.
		// NOTE: Must be last since it can process any feature type.
		assign_plate_id_tasks.push_back(
				assign_plate_id_task_ptr_type(
						new GenericAssignPlateIdTask(
								feature_collection_ref,
								model,
								reconstruction_time,
								reconstruction_tree,
								assign_plate_id_method)));
	}
}


bool
GPlatesAppLogic::AssignPlateIds::find_reconstructable_features_without_reconstruction_plate_id(
		std::vector<GPlatesModel::FeatureHandle::weak_ref> &features_needing_plate_id,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref)
{
	// Find any features in the current feature collection that are have no
	// 'reconstructionPlateId' plate id and are also not reconstruction features.
	return GPlatesAppLogic::ClassifyFeatureCollection::find_classified_features(
			features_needing_plate_id,
			feature_collection_ref,
			&needs_reconstruction_plate_id);
}


GPlatesAppLogic::AssignPlateIds::AssignPlateIds(
		AssignPlateIdMethodType assign_plate_id_method,
		const GPlatesModel::ModelInterface &model,
		GPlatesAppLogic::Reconstruct &reconstruct) :
	d_assign_plate_id_method(assign_plate_id_method),
	d_model(model),
	d_reconstruction_time(reconstruct.get_current_reconstruction_time()),
	d_reconstruction(reconstruct.get_current_reconstruction_non_null_ptr())
{
	// Query the resolved boundaries in the reconstruction for partitioning geometry.
	d_resolved_boundaries_geometry_partitioning_query = GPlatesAppLogic::TopologyUtils::
			query_resolved_topologies_for_geometry_partitioning(
					reconstruct.get_current_reconstruction());
}


GPlatesAppLogic::AssignPlateIds::AssignPlateIds(
		AssignPlateIdMethodType assign_plate_id_method,
		const GPlatesModel::ModelInterface &model,
		FeatureCollectionFileState &file_state,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id) :
	d_assign_plate_id_method(assign_plate_id_method),
	d_model(model),
	d_reconstruction_time(reconstruction_time),
	d_reconstruction(
			create_reconstruction(
					file_state, reconstruction_time, anchor_plate_id))
{
	// Query the resolved boundaries in the reconstruction for partitioning geometry.
	d_resolved_boundaries_geometry_partitioning_query = GPlatesAppLogic::TopologyUtils::
			query_resolved_topologies_for_geometry_partitioning(*d_reconstruction);
}


bool
GPlatesAppLogic::AssignPlateIds::assign_reconstruction_plate_ids(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref)
{
	if (!feature_collection_ref.is_valid())
	{
		return false;
	}

	bool assigned_any_plate_ids = false;

	GPlatesModel::FeatureCollectionHandle::features_iterator feature_iter =
			feature_collection_ref->features_begin();
	GPlatesModel::FeatureCollectionHandle::features_iterator feature_end =
			feature_collection_ref->features_end();
	for ( ; feature_iter != feature_end; ++feature_iter)
	{
		if (feature_iter.is_valid())
		{
			const bool assigned_plate_id = assign_reconstruction_plate_id(
					(*feature_iter)->reference(), feature_collection_ref);

			assigned_any_plate_ids |= assigned_plate_id;
		}
	}

	return assigned_any_plate_ids;
}


bool
GPlatesAppLogic::AssignPlateIds::assign_reconstruction_plate_ids(
		const std::vector<GPlatesModel::FeatureHandle::weak_ref> &feature_refs,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref)
{
	bool assigned_any_plate_ids = false;

	typedef std::vector<GPlatesModel::FeatureHandle::weak_ref> feature_ref_seq_type;

	for (feature_ref_seq_type::const_iterator feature_ref_iter = feature_refs.begin();
		feature_ref_iter != feature_refs.end();
		++feature_ref_iter)
	{
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref = *feature_ref_iter;

		if (feature_ref.is_valid())
		{
			const bool assigned_plate_id = assign_reconstruction_plate_id(
					feature_ref, feature_collection_ref);

			assigned_any_plate_ids |= assigned_plate_id;
		}
	}

	return assigned_any_plate_ids;
}


bool
GPlatesAppLogic::AssignPlateIds::assign_reconstruction_plate_id(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref)
{
	if (!feature_ref.is_valid())
	{
		return false;
	}

	if (!d_resolved_boundaries_geometry_partitioning_query)
	{
		// Return early if there are no resolved boundaries.
		return false;
	}

	// Register all the assign plate id tasks.
	assign_plate_id_task_ptr_seq_type assign_plate_id_tasks;
	get_all_assign_plate_id_tasks(
			assign_plate_id_tasks,
			feature_collection_ref,
			d_model,
			d_reconstruction_time,
			d_reconstruction->reconstruction_tree(),
			d_assign_plate_id_method);

	// Iterate through the assign plate id tasks until we find one that
	// can assign a plate id to the feature.
	assign_plate_id_task_ptr_seq_type::const_iterator assign_plate_id_task_iter =
			assign_plate_id_tasks.begin();
	assign_plate_id_task_ptr_seq_type::const_iterator assign_plate_id_task_end =
			assign_plate_id_tasks.end();
	for ( ; assign_plate_id_task_iter != assign_plate_id_task_end; ++assign_plate_id_task_iter)
	{
		const assign_plate_id_task_ptr_type &assign_plate_id_task = *assign_plate_id_task_iter;

		if (assign_plate_id_task->can_assign_plate_id_to_feature(feature_ref))
		{
			return assign_plate_id_task->assign_plate_id_to_feature(
					feature_ref, d_resolved_boundaries_geometry_partitioning_query);
		}
	}

	// No assign plate id tasks could handle the feature.
	return false;
}
