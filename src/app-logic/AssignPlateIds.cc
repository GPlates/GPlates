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

#include <algorithm>
#include <list>
#include <map>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/operators.hpp>
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

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/FiniteRotation.h"

#include "model/FeatureType.h"
#include "model/FeatureVisitor.h"
#include "model/Model.h"
#include "model/ModelInterface.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"
#include "model/ReconstructionTree.h"
#include "model/ResolvedTopologicalBoundary.h"
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
	 * Returns the 'gpml:reconstructionPlateId' plate id if one exists.
	 */
	boost::optional<GPlatesModel::integer_plate_id_type>
	get_reconstruction_plate_id_from_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
	{
		static const GPlatesModel::PropertyName RECONSTRUCTION_PLATE_ID_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id = NULL;
		boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id;
		if (!GPlatesFeatureVisitors::get_property_value(
				feature_ref,
				RECONSTRUCTION_PLATE_ID_PROPERTY_NAME,
				recon_plate_id))
		{
			return boost::none;
		}

		return recon_plate_id->value();
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
		static const GPlatesModel::PropertyName RECONSTRUCTION_PLATE_ID_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		// First remove any that might already exist.
		GPlatesModel::ModelUtils::remove_properties_from_feature_by_name(
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
	 * Calculate polyline distance along unit radius sphere.
	 */
	template <typename GreatCircleArcForwardIteratorType>
	GPlatesMaths::real_t
	calculate_arc_distance(
			GreatCircleArcForwardIteratorType gca_begin,
			GreatCircleArcForwardIteratorType gca_end)
	{
		GPlatesMaths::real_t distance = 0;

		GreatCircleArcForwardIteratorType gca_iter = gca_begin;
		for ( ; gca_iter != gca_end; ++gca_iter)
		{
			distance += acos(gca_iter->dot_of_endpoints());
		}

		return distance;
	}


	/**
	 * Visits a @a GeometryOnSphere and accumulates a size metric for it;
	 * for points/multipoints this is number of points and for polylines/polygons
	 * this is arc distance.
	 */
	class GeometrySizeMetric :
			public boost::less_than_comparable<GeometrySizeMetric>
	{
	public:
		GeometrySizeMetric() :
			d_num_points(0),
			d_arc_distance(0),
			d_using_arc_distance(false)
		{  }

		unsigned int
		get_num_points() const
		{
			return d_num_points;
		}

		GPlatesMaths::real_t
		get_arc_distance() const
		{
			return d_arc_distance;
		}

		/**
		 * For points and multipoints adds number of points to current total
		 * number of points; for polylines and polygons adds the
		 * arc distance (unit sphere) to the current total arc distance.
		 */
		void
		accumulate(
				const GPlatesMaths::GeometryOnSphere &geometry)
		{
			GeometrySize geometry_size(d_num_points, d_arc_distance, d_using_arc_distance);
			geometry.accept_visitor(geometry_size);
		}

		/**
		 * Less than operator.
		 * Greater than operator provided by base class boost::less_than_comparable.
		 */
		bool
		operator<(
				const GeometrySizeMetric &rhs) const
		{
			// Prefer to compare arc distance if we have visited any
			// line geometry.
			if (d_using_arc_distance || rhs.d_using_arc_distance)
			{
				return d_arc_distance < rhs.d_arc_distance;
			}

			return d_num_points < rhs.d_num_points;
		}

	private:
		class GeometrySize :
				public GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:
			GeometrySize(
					unsigned int &num_points,
					GPlatesMaths::real_t &arc_distance,
					bool &using_arc_distance) :
				d_num_points(num_points),
				d_arc_distance(arc_distance),
				d_using_arc_distance(using_arc_distance)
			{  }

			virtual
			void
			visit_multi_point_on_sphere(
					GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				d_num_points += multi_point_on_sphere->number_of_points();
			}

			virtual
			void
			visit_point_on_sphere(
					GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type /*point_on_sphere*/)
			{
				++d_num_points;
			}

			virtual
			void
			visit_polygon_on_sphere(
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_arc_distance += calculate_arc_distance(
						polygon_on_sphere->begin(), polygon_on_sphere->end());
				d_using_arc_distance = true;
			}

			virtual
			void
			visit_polyline_on_sphere(
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_arc_distance += calculate_arc_distance(
						polyline_on_sphere->begin(), polyline_on_sphere->end());
				d_using_arc_distance = true;
			}

			unsigned int &d_num_points;
			GPlatesMaths::real_t &d_arc_distance;
			bool &d_using_arc_distance;
		};

		unsigned int d_num_points;
		GPlatesMaths::real_t d_arc_distance;
		bool d_using_arc_distance;
	};


	/**
	 * Visits a @a GeometryOnSphere and creates a suitable property value for it.
	 */
	class CreateGeometryProperty :
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
		create_geometry_property(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry)
		{
			d_geometry_property = boost::none;

			geometry->accept_visitor(*this);

			return d_geometry_property;
		}

	protected:
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type gml_multi_point =
					GPlatesPropertyValues::GmlMultiPoint::create(multi_point_on_sphere);

			d_geometry_property = GPlatesModel::ModelUtils::create_gpml_constant_value(
					gml_multi_point, 
					GPlatesPropertyValues::TemplateTypeParameterType::create_gml("MultiPoint"));
		}

		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			const GPlatesModel::PropertyValue::non_null_ptr_type gml_point =
					GPlatesPropertyValues::GmlPoint::create(*point_on_sphere);

			d_geometry_property = GPlatesModel::ModelUtils::create_gpml_constant_value(
					gml_point, 
					GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point"));
		}

		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			GPlatesPropertyValues::GmlPolygon::non_null_ptr_type gml_polygon =
					GPlatesPropertyValues::GmlPolygon::create(polygon_on_sphere);

			d_geometry_property = GPlatesModel::ModelUtils::create_gpml_constant_value(
					gml_polygon, 
					GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Polygon"));
		}

		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
					GPlatesPropertyValues::GmlLineString::create(polyline_on_sphere);
			GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type gml_orientable_curve =
					GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);
			d_geometry_property = GPlatesModel::ModelUtils::create_gpml_constant_value(
					gml_orientable_curve, 
					GPlatesPropertyValues::TemplateTypeParameterType::create_gml("OrientableCurve"));
		}

	private:
		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> d_geometry_property;
	};


	/**
	 * Creates a property value suitable for @a geometry and appends it
	 * to @a feature_ref with the property name @a geometry_property_name.
	 *
	 * It doesn't attempt to remove any existing properties named @a geometry_property_name.
	 */
	void
	append_geometry_to_feature(
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
			const GPlatesModel::PropertyName &geometry_property_name,
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
	{
		CreateGeometryProperty geometry_property_creator;

		boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
				geometry_property =
						geometry_property_creator.create_geometry_property(geometry);

		if (geometry_property)
		{
			GPlatesModel::ModelUtils::append_property_value_to_feature(
					*geometry_property, geometry_property_name, feature_ref);
		}
	}


	/**
	 * Returns true if @a feature_properties_iter is a geometry property.
	 */
	bool
	is_geometry_property(
			const GPlatesModel::FeatureHandle::children_iterator &feature_properties_iter)
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
			const GPlatesModel::FeatureHandle::children_iterator &feature_properties_iter)
	{
		static const GPlatesModel::PropertyName RECONSTRUCTION_PLATE_ID_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

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
		GPlatesModel::FeatureHandle::children_iterator feature_properties_iter =
				feature_ref->children_begin();
		GPlatesModel::FeatureHandle::children_iterator feature_properties_end =
				feature_ref->children_end();
		while (feature_properties_iter != feature_properties_end)
		{
			// Increment iterator before we remove property.
			// I don't think this is currently necessary but it doesn't hurt.
			GPlatesModel::FeatureHandle::children_iterator current_feature_properties_iter =
					feature_properties_iter;
			++feature_properties_iter;

			if(!current_feature_properties_iter.is_valid())
			{
				continue;
			}

			if (is_geometry_property(current_feature_properties_iter))
			{
				GPlatesModel::ModelUtils::remove_property_from_feature(
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
						old_feature_ref->handle_data().feature_type(),
						feature_collection_ref);

		// Iterate over the feature properties of the original feature
		// and clone them and append the cloned versions to the new feature.
		GPlatesModel::FeatureHandle::children_iterator old_feature_properties_iter =
				old_feature_ref->children_begin();
		GPlatesModel::FeatureHandle::children_iterator old_feature_properties_end =
				old_feature_ref->children_end();
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
			const GPlatesModel::TopLevelProperty::non_null_ptr_type cloned_property =
					(*old_feature_properties_iter)->clone();

			// Add the cloned property value to the new feature.
			GPlatesModel::ModelUtils::append_property_to_feature(
					cloned_property, new_feature_ref);
		}

		return new_feature_ref;
	}


	/**
	 * RAII class makes sure a geometry properties iterator gets removed from its feature.
	 */
	class GeometryPropertiesIteratorRemover
	{
	public:
		GeometryPropertiesIteratorRemover(
				const GPlatesModel::FeatureHandle::children_iterator &geometry_properties_iterator) :
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
					GPlatesModel::ModelUtils::remove_property_from_feature(
							d_geometry_properties_iterator,
							d_geometry_properties_iterator.collection_handle_ptr()->reference());
				}
			}
			catch (...)
			{
			}
		}

	private:
		GPlatesModel::FeatureHandle::children_iterator d_geometry_properties_iterator;
	};


	class GeometryPropertyAllPartitionedGeometries
	{
	public:
		GeometryPropertyAllPartitionedGeometries(
				const GPlatesModel::PropertyName &geometry_property_name_,
				const GPlatesModel::FeatureHandle::children_iterator &geometry_properties_iterator_) :
			geometry_property_name(geometry_property_name_),
			geometry_properties_iterator(geometry_properties_iterator_)
		{  }

		GPlatesModel::PropertyName geometry_property_name;
		GPlatesModel::FeatureHandle::children_iterator geometry_properties_iterator;
		GPlatesAppLogic::TopologyUtils::resolved_boundary_partitioned_geometries_seq_type
				partitioned_inside_geometries_seq;
		GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type
				partitioned_outside_geometries;
	};
	typedef std::list<GeometryPropertyAllPartitionedGeometries>
			geometry_property_all_partitioned_geometries_seq_type;


	class GeometryPropertyPartitionedInsideGeometries
	{
	public:
		GeometryPropertyPartitionedInsideGeometries(
				const GPlatesModel::PropertyName &geometry_property_name_,
				const GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type &
						partitioned_geometries_) :
			geometry_property_name(geometry_property_name_),
			partitioned_geometries(partitioned_geometries_)
		{  }

		GPlatesModel::PropertyName geometry_property_name;
		GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type partitioned_geometries;
	};
	typedef std::list<GeometryPropertyPartitionedInsideGeometries>
			geometry_property_partitioned_inside_geometries_seq_type;

	class ResolvedBoundaryGeometryProperties
	{
	public:
		geometry_property_partitioned_inside_geometries_seq_type
				geometry_property_partitioned_inside_geometries_seq;
	};
	typedef std::map<
			const GPlatesModel::ResolvedTopologicalBoundary *,
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
				geometry_property_all_partitioned_geometries_seq_type &
						geometry_property_all_partitioned_geometries_seq,
				const GPlatesAppLogic::TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type &
						geometry_partition_query,
				const boost::optional<GPlatesMaths::FiniteRotation> &reconstruction_rotation,
				const boost::optional<double> &reconstruction_time = boost::none) :
			d_geometry_partition_query(geometry_partition_query),
			d_reconstruction_rotation(reconstruction_rotation),
			d_reconstruction_time(reconstruction_time),
			d_geometry_property_all_partitioned_geometries_seq(
					geometry_property_all_partitioned_geometries_seq)
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
			GeometryPropertyAllPartitionedGeometries geometry_property_all_partitioned_geometries(
					*current_top_level_propname(),
					*current_top_level_propiter());

			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type reconstructed_polyline =
					d_reconstruction_rotation
					? *d_reconstruction_rotation * gml_line_string.polyline()
					: gml_line_string.polyline();

			GPlatesAppLogic::TopologyUtils::partition_geometry_using_resolved_topology_boundaries(
					geometry_property_all_partitioned_geometries.partitioned_inside_geometries_seq,
					geometry_property_all_partitioned_geometries.partitioned_outside_geometries,
					reconstructed_polyline,
					d_geometry_partition_query);

			d_geometry_property_all_partitioned_geometries_seq.push_back(
					geometry_property_all_partitioned_geometries);
		}


		void
		visit_gml_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
		{
			GeometryPropertyAllPartitionedGeometries geometry_property_all_partitioned_geometries(
					*current_top_level_propname(),
					*current_top_level_propiter());

			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type reconstructed_multipoint =
					d_reconstruction_rotation
					? *d_reconstruction_rotation * gml_multi_point.multipoint()
					: gml_multi_point.multipoint();

			GPlatesAppLogic::TopologyUtils::partition_geometry_using_resolved_topology_boundaries(
					geometry_property_all_partitioned_geometries.partitioned_inside_geometries_seq,
					geometry_property_all_partitioned_geometries.partitioned_outside_geometries,
					reconstructed_multipoint,
					d_geometry_partition_query);

			d_geometry_property_all_partitioned_geometries_seq.push_back(
					geometry_property_all_partitioned_geometries);
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
			GeometryPropertyAllPartitionedGeometries geometry_property_all_partitioned_geometries(
					*current_top_level_propname(),
					*current_top_level_propiter());

			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type reconstructed_point =
					d_reconstruction_rotation
					? *d_reconstruction_rotation * gml_point.point()
					: gml_point.point();

			GPlatesAppLogic::TopologyUtils::partition_geometry_using_resolved_topology_boundaries(
					geometry_property_all_partitioned_geometries.partitioned_inside_geometries_seq,
					geometry_property_all_partitioned_geometries.partitioned_outside_geometries,
					reconstructed_point,
					d_geometry_partition_query);

			d_geometry_property_all_partitioned_geometries_seq.push_back(
					geometry_property_all_partitioned_geometries);
		}


		void
		visit_gml_polygon(
				GPlatesPropertyValues::GmlPolygon &gml_polygon)
		{
			GeometryPropertyAllPartitionedGeometries geometry_property_all_partitioned_geometries(
					*current_top_level_propname(),
					*current_top_level_propiter());

			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type reconstructed_polygon =
					d_reconstruction_rotation
					? *d_reconstruction_rotation * gml_polygon.exterior()
					: gml_polygon.exterior();

			// Partition the exterior polygon.
			GPlatesAppLogic::TopologyUtils::partition_geometry_using_resolved_topology_boundaries(
					geometry_property_all_partitioned_geometries.partitioned_inside_geometries_seq,
					geometry_property_all_partitioned_geometries.partitioned_outside_geometries,
					reconstructed_polygon,
					d_geometry_partition_query);
			d_geometry_property_all_partitioned_geometries_seq.push_back(
					geometry_property_all_partitioned_geometries);

			// Iterate over the interior polygons.
			//
			// FIXME: We are losing the distinction between exterior polygon and interior
			// polygons because when it comes time to build the GmlPolygon (which only
			// happens if we're partitioning geometry as opposed to assigning the entire
			// geometry property to a plate) then if there are interior polygons that
			// are fully inside a plate boundary they will be added to a feature as
			// an exterior polygon.
			// For now this is adequate since at least the user don't lose there
			// interior polygons (they just can reappear as exterior polygons).
			GPlatesPropertyValues::GmlPolygon::ring_const_iterator interior_iter =
					gml_polygon.interiors_begin();
			GPlatesPropertyValues::GmlPolygon::ring_const_iterator interior_end =
					gml_polygon.interiors_end();
			for ( ; interior_iter != interior_end; ++interior_iter)
			{
				const GPlatesPropertyValues::GmlPolygon::ring_type &interior_polygon = *interior_iter;

				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type reconstructed_interior_polygon =
						d_reconstruction_rotation
						? *d_reconstruction_rotation * interior_polygon
						: interior_polygon;

				// Partition interior polygon.
				GPlatesAppLogic::TopologyUtils::partition_geometry_using_resolved_topology_boundaries(
						geometry_property_all_partitioned_geometries.partitioned_inside_geometries_seq,
						geometry_property_all_partitioned_geometries.partitioned_outside_geometries,
						reconstructed_interior_polygon,
						d_geometry_partition_query);
				d_geometry_property_all_partitioned_geometries_seq.push_back(
						geometry_property_all_partitioned_geometries);
			}
		}


		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}

	private:
		GPlatesAppLogic::TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type
				d_geometry_partition_query;

		boost::optional<GPlatesMaths::FiniteRotation> d_reconstruction_rotation;

		boost::optional<double> d_reconstruction_time;

		geometry_property_all_partitioned_geometries_seq_type &
				d_geometry_property_all_partitioned_geometries_seq;
	};


	void
	map_resolved_boundaries_to_geometry_properties(
			resolved_boundary_geometry_properties_map_type &
					resolved_boundary_geometry_properties_map,
			const geometry_property_all_partitioned_geometries_seq_type &
					geometry_property_all_partitioned_geometries_seq)
	{
		geometry_property_all_partitioned_geometries_seq_type::const_iterator
				geometry_property_all_partitioned_geometries_iter =
						geometry_property_all_partitioned_geometries_seq.begin();
		geometry_property_all_partitioned_geometries_seq_type::const_iterator
				geometry_property_all_partitioned_geometries_end =
						geometry_property_all_partitioned_geometries_seq.end();
		for ( ;
			geometry_property_all_partitioned_geometries_iter !=
				geometry_property_all_partitioned_geometries_end;
			++geometry_property_all_partitioned_geometries_iter)
		{
			const GeometryPropertyAllPartitionedGeometries &
					geometry_property_all_partitioned_geometries = 
							*geometry_property_all_partitioned_geometries_iter;

			const GPlatesModel::PropertyName &geometry_property_name =
					geometry_property_all_partitioned_geometries.geometry_property_name;

			const GPlatesAppLogic::TopologyUtils::resolved_boundary_partitioned_geometries_seq_type &
					resolved_boundary_partitioned_geometries_seq =
							geometry_property_all_partitioned_geometries
									.partitioned_inside_geometries_seq;

			GPlatesAppLogic::TopologyUtils
					::resolved_boundary_partitioned_geometries_seq_type::const_iterator
							resolved_boundary_partitioned_geometries_iter =
									resolved_boundary_partitioned_geometries_seq.begin();
			GPlatesAppLogic::TopologyUtils
					::resolved_boundary_partitioned_geometries_seq_type::const_iterator
							resolved_boundary_partitioned_geometries_end =
									resolved_boundary_partitioned_geometries_seq.end();
			for ( ;
				resolved_boundary_partitioned_geometries_iter !=
					resolved_boundary_partitioned_geometries_end;
				++resolved_boundary_partitioned_geometries_iter)
			{
				const GPlatesAppLogic::TopologyUtils::ResolvedBoundaryPartitionedGeometries &
						resolved_boundary_partitioned_geometries =
								*resolved_boundary_partitioned_geometries_iter;

				const GPlatesModel::ResolvedTopologicalBoundary *resolved_topological_boundary =
						resolved_boundary_partitioned_geometries.resolved_topological_boundary;

				GeometryPropertyPartitionedInsideGeometries
						geometry_property_partitioned_inside_geometries(
								geometry_property_name,
								resolved_boundary_partitioned_geometries
										.partitioned_inside_geometries);

				resolved_boundary_geometry_properties_map[resolved_topological_boundary]
						.geometry_property_partitioned_inside_geometries_seq
						.push_back(geometry_property_partitioned_inside_geometries);
			}
		}
	}


	bool
	assign_partitioned_geometry_outside_all_resolved_boundaries(
			const geometry_property_all_partitioned_geometries_seq_type &
					geometry_property_all_partitioned_geometries_seq,
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
			const boost::optional<GPlatesMaths::FiniteRotation> &original_reconstruction_rotation)
	{
		bool assigned_to_feature = false;

		geometry_property_all_partitioned_geometries_seq_type::const_iterator
				geometry_property_all_partitioned_geometries_iter =
						geometry_property_all_partitioned_geometries_seq.begin();
		geometry_property_all_partitioned_geometries_seq_type::const_iterator
				geometry_property_all_partitioned_geometries_end =
						geometry_property_all_partitioned_geometries_seq.end();
		for ( ;
			geometry_property_all_partitioned_geometries_iter !=
				geometry_property_all_partitioned_geometries_end;
			++geometry_property_all_partitioned_geometries_iter)
		{
			const GeometryPropertyAllPartitionedGeometries &
					geometry_property_all_partitioned_geometries = 
							*geometry_property_all_partitioned_geometries_iter;

			if (geometry_property_all_partitioned_geometries.partitioned_outside_geometries.empty())
			{
				continue;
			}

			const GPlatesModel::PropertyName &geometry_property_name =
					geometry_property_all_partitioned_geometries.geometry_property_name;
			const GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type &
					outside_geometry_seq =
						geometry_property_all_partitioned_geometries.partitioned_outside_geometries;

			// Iterate over the outside geometries.
			GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type::const_iterator
					outside_geometry_iter = outside_geometry_seq.begin();
			GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type::const_iterator
					outside_geometry_end = outside_geometry_seq.end();
			for ( ; outside_geometry_iter != outside_geometry_end; ++outside_geometry_iter)
			{
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &
						outside_geometry = *outside_geometry_iter;

				// Since we're appending reconstructed geometry we need to reverse
				// reconstruct it using the original rotation. That way if the user
				// explicitly gives it the same original reconstruction id again then
				// it's reconstructed position will move back again.
				if (original_reconstruction_rotation)
				{
					const GPlatesMaths::FiniteRotation rotate_to_present_day =
							GPlatesMaths::get_reverse(*original_reconstruction_rotation);

					append_geometry_to_feature(
							rotate_to_present_day * outside_geometry,
							geometry_property_name,
							feature_ref);
				}
				else
				{
					append_geometry_to_feature(
							outside_geometry,
							geometry_property_name,
							feature_ref);
				}
			}

			assigned_to_feature = true;
		}

		return assigned_to_feature;
	}


	/**
	 * Returns true if all geometry properties in
	 * @a geometry_property_all_partitioned_geometries_seq are outside
	 * all resolved boundaries.
	 */
	bool
	is_geometry_outside_all_resolved_boundaries(
			const geometry_property_all_partitioned_geometries_seq_type &
					geometry_property_all_partitioned_geometries_seq)
	{
		geometry_property_all_partitioned_geometries_seq_type::const_iterator
				geometry_property_all_partitioned_geometries_iter =
						geometry_property_all_partitioned_geometries_seq.begin();
		geometry_property_all_partitioned_geometries_seq_type::const_iterator
				geometry_property_all_partitioned_geometries_end =
						geometry_property_all_partitioned_geometries_seq.end();
		for ( ;
			geometry_property_all_partitioned_geometries_iter !=
				geometry_property_all_partitioned_geometries_end;
			++geometry_property_all_partitioned_geometries_iter)
		{
			const GeometryPropertyAllPartitionedGeometries &
					geometry_property_all_partitioned_geometries = 
							*geometry_property_all_partitioned_geometries_iter;

			if (!geometry_property_all_partitioned_geometries
				.partitioned_inside_geometries_seq.empty())
			{
				// We found some inside geometries which means not all geometries
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
			const GPlatesModel::ResolvedTopologicalBoundary *&resolved_boundary_containing_most_geometry,
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

		GeometrySizeMetric max_resolved_boundary_partitioned_geometry_size_metric;
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
			const GPlatesModel::ResolvedTopologicalBoundary *resolved_topological_boundary =
					resolved_boundary_geometry_properties_iter->first;
			const ResolvedBoundaryGeometryProperties &resolved_boundary_geometry_properties =
					resolved_boundary_geometry_properties_iter->second;

			if (!resolved_topological_boundary->plate_id())
			{
				// Each resolved boundary should have a plate id so this shouldn't happen.
				continue;
			}

			GeometrySizeMetric resolved_boundary_partitioned_geometry_size_metric;

			// Iterate over the geometry properties that have partitioned geometries
			// for the current resolved boundary.
			const geometry_property_partitioned_inside_geometries_seq_type &
					geometry_property_partitioned_inside_geometries_seq =
							resolved_boundary_geometry_properties
									.geometry_property_partitioned_inside_geometries_seq;
			geometry_property_partitioned_inside_geometries_seq_type::const_iterator
					geometry_property_partitioned_inside_geometries_iter =
							geometry_property_partitioned_inside_geometries_seq.begin();
			geometry_property_partitioned_inside_geometries_seq_type::const_iterator
					geometry_property_partitioned_inside_geometries_end =
							geometry_property_partitioned_inside_geometries_seq.end();
			for ( ;
				geometry_property_partitioned_inside_geometries_iter !=
					geometry_property_partitioned_inside_geometries_end;
				++geometry_property_partitioned_inside_geometries_iter)
			{
				const GeometryPropertyPartitionedInsideGeometries &
						geometry_property_partitioned_inside_geometries =
								*geometry_property_partitioned_inside_geometries_iter;

				const GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type &
						partitioned_inside_geometry_seq =
								geometry_property_partitioned_inside_geometries
										.partitioned_geometries;

				// Iterate over the partitioned geometries of the current geometry.
				GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type::const_iterator
						partitioned_inside_geometry_iter = partitioned_inside_geometry_seq.begin();
				GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type::const_iterator
						paritioned_inside_geometry_end = partitioned_inside_geometry_seq.end();
				for ( ;
					partitioned_inside_geometry_iter != paritioned_inside_geometry_end;
					++partitioned_inside_geometry_iter)
				{
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &
							inside_geometry = *partitioned_inside_geometry_iter;

					resolved_boundary_partitioned_geometry_size_metric.accumulate(*inside_geometry);
				}
			}

			if (resolved_boundary_partitioned_geometry_size_metric >
					max_resolved_boundary_partitioned_geometry_size_metric)
			{
				max_resolved_boundary_partitioned_geometry_size_metric =
						resolved_boundary_partitioned_geometry_size_metric;

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
			const GPlatesModel::ResolvedTopologicalBoundary *&resolved_boundary_containing_most_geometry,
			const GPlatesAppLogic::TopologyUtils::resolved_boundary_partitioned_geometries_seq_type &
					resolved_boundary_partitioned_geometry_seq)
	{
		resolved_boundary_containing_most_geometry = NULL;

		if (resolved_boundary_partitioned_geometry_seq.empty())
		{
			// We shouldn't get here as shouldn't have been called with an empty sequence.
			return false;
		}

		// Shortcut if there's only one resolved boundary since we don't need
		// to calculate the partitioned geometry distances to find the maximum.
		if (resolved_boundary_partitioned_geometry_seq.size() == 1)
		{
			resolved_boundary_containing_most_geometry =
					resolved_boundary_partitioned_geometry_seq.front().resolved_topological_boundary;

			if (!resolved_boundary_containing_most_geometry->plate_id())
			{
				// Each resolved boundary should have a plate id so this shouldn't happen.
				return false;
			}
			return true;
		}

		GeometrySizeMetric max_resolved_boundary_partitioned_geometry_size_metric;

		// Iterate over the resolved boundaries that the current geometry property
		// was partitioned into and determine which one contains the most geometry
		// from the current geometry property.
		GPlatesAppLogic::TopologyUtils
				::resolved_boundary_partitioned_geometries_seq_type::const_iterator
						resolved_boundary_partitioned_geometries_iter =
								resolved_boundary_partitioned_geometry_seq.begin();
		GPlatesAppLogic::TopologyUtils
				::resolved_boundary_partitioned_geometries_seq_type::const_iterator
						resolved_boundary_partitioned_geometries_end =
								resolved_boundary_partitioned_geometry_seq.end();

		for ( ;
			resolved_boundary_partitioned_geometries_iter !=
				resolved_boundary_partitioned_geometries_end;
			++resolved_boundary_partitioned_geometries_iter)
		{
			const GPlatesAppLogic::TopologyUtils::ResolvedBoundaryPartitionedGeometries &
					resolved_boundary_partitioned_geometries =
							*resolved_boundary_partitioned_geometries_iter;

			const GPlatesModel::ResolvedTopologicalBoundary *resolved_topological_boundary =
					resolved_boundary_partitioned_geometries.resolved_topological_boundary;

			if (!resolved_topological_boundary->plate_id())
			{
				// Each resolved boundary should have a plate id so this shouldn't happen.
				continue;
			}

			const GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type &
					partitioned_inside_geometry_seq =
							resolved_boundary_partitioned_geometries.partitioned_inside_geometries;

			GeometrySizeMetric resolved_boundary_partitioned_geometry_size_metric;

			// Iterate over the partitioned inside geometries of the current
			// resolved boundary of the current geometry property.
			GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type::const_iterator
					partitioned_inside_geometry_iter = partitioned_inside_geometry_seq.begin();
			GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type::const_iterator
					paritioned_inside_geometry_end = partitioned_inside_geometry_seq.end();
			for ( ;
				partitioned_inside_geometry_iter != paritioned_inside_geometry_end;
				++partitioned_inside_geometry_iter)
			{
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &
						inside_geometry = *partitioned_inside_geometry_iter;

				resolved_boundary_partitioned_geometry_size_metric.accumulate(*inside_geometry);
			}

			if (resolved_boundary_partitioned_geometry_size_metric >
					max_resolved_boundary_partitioned_geometry_size_metric)
			{
				max_resolved_boundary_partitioned_geometry_size_metric =
						resolved_boundary_partitioned_geometry_size_metric;

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
			const geometry_property_all_partitioned_geometries_seq_type &
					geometry_property_all_partitioned_geometries_seq,
			const resolved_boundary_geometry_properties_map_type &
					resolved_boundary_geometry_properties_map,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
			GPlatesModel::ModelInterface &model,
			const double &reconstruction_time,
			const GPlatesModel::ReconstructionTree &reconstruction_tree,
			const boost::optional<GPlatesMaths::FiniteRotation> &original_reconstruction_rotation)
	{
		static const GPlatesModel::PropertyName RECONSTRUCTION_PLATE_ID_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		// Start out by removing the 'gpml:reconstructionPlateId' property.
		GPlatesModel::ModelUtils::remove_properties_from_feature_by_name(
				RECONSTRUCTION_PLATE_ID_PROPERTY_NAME, feature_ref);

		// If no geometry is partitioned into any plates then simply return.
		// Note: We don't have to reverse reconstruct its geometry because
		// this feature  has not been assigned a plate id and therefore
		// is now considered to be present day geometry.
		if (is_geometry_outside_all_resolved_boundaries(
				geometry_property_all_partitioned_geometries_seq))
		{
			return false;
		}

		//
		// Iterate over the resolved boundaries and calculate the line distance
		// of all polyline segments partitioned into each plate.
		// Give the feature the plate id of the plate that has the
		// largest distance contains the most geometry of the feature).
		//
		const GPlatesModel::ResolvedTopologicalBoundary *
				resolved_boundary_containing_most_geometry = NULL;
		if (!find_resolved_boundary_with_valid_plate_id_that_contains_most_geometry(
				resolved_boundary_containing_most_geometry,
				resolved_boundary_geometry_properties_map))
		{
			// We shouldn't get here since we should have at least one resolved boundary
			// with a valid plate id and it should have at least one partitioned geometry.
			return false;
		}

		// Assign feature to plate that contains largest proportion of
		// feature's geometry.
		const GPlatesModel::integer_plate_id_type reconstruction_plate_id =
				*resolved_boundary_containing_most_geometry->plate_id();

		// Get the composed absolute rotation needed to reverse reconstruct
		// geometries to present day.
		GPlatesMaths::FiniteRotation rotate_to_present_day =
				GPlatesMaths::get_reverse(
						reconstruction_tree.get_composed_absolute_rotation(
								reconstruction_plate_id).first);

		// If the original feature had a reconstruction plate id then it was
		// rotated to the reconstruction time so we need to do that too.
		if (original_reconstruction_rotation)
		{
			rotate_to_present_day = compose(
					rotate_to_present_day, *original_reconstruction_rotation);
		}

		// Visit the feature and rotate all its geometry using 'reverse_rotation'.
		GPlatesFeatureVisitors::GeometryRotator geometry_rotator(rotate_to_present_day);
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
			const geometry_property_all_partitioned_geometries_seq_type &
					geometry_property_all_partitioned_geometries_seq,
			const resolved_boundary_geometry_properties_map_type &
					resolved_boundary_geometry_properties_map,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
			GPlatesModel::ModelInterface &model,
			const double &reconstruction_time,
			const GPlatesModel::ReconstructionTree &reconstruction_tree,
			const boost::optional<GPlatesMaths::FiniteRotation> &original_reconstruction_rotation)
	{
		static const GPlatesModel::PropertyName RECONSTRUCTION_PLATE_ID_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		// Start out by removing the 'gpml:reconstructionPlateId' property.
		// But don't remove any geometry properties because we are going to clone
		// them as we go and put them in new cloned features.
		GPlatesModel::ModelUtils::remove_properties_from_feature_by_name(
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
		geometry_property_all_partitioned_geometries_seq_type::const_iterator
				geometry_property_all_partitioned_geometries_iter =
						geometry_property_all_partitioned_geometries_seq.begin();
		geometry_property_all_partitioned_geometries_seq_type::const_iterator
				geometry_property_all_partitioned_geometries_end =
						geometry_property_all_partitioned_geometries_seq.end();
		for ( ;
			geometry_property_all_partitioned_geometries_iter !=
				geometry_property_all_partitioned_geometries_end;
			++geometry_property_all_partitioned_geometries_iter)
		{
			const GeometryPropertyAllPartitionedGeometries &
					geometry_property_all_partitioned_geometries = 
							*geometry_property_all_partitioned_geometries_iter;

			GPlatesModel::FeatureHandle::children_iterator geometry_properties_iterator =
					geometry_property_all_partitioned_geometries.geometry_properties_iterator;

			// Make sure geometry property gets removed from the original feature
			// before continuing to the next geometry property.
			GeometryPropertiesIteratorRemover geometry_properties_iterator_remover(
					geometry_properties_iterator);

			if (geometry_property_all_partitioned_geometries.partitioned_inside_geometries_seq.empty())
			{
				// There were no inside geometries which means the current geometry
				// property is outside all resolved boundaries.
				// Hence we cannot assign a plate id to it and so cannot
				// reverse reconstruct to present day.

				// Clone the current source geometry property value.
				const GPlatesModel::TopLevelProperty::non_null_ptr_type cloned_geometry_property =
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
				GPlatesModel::ModelUtils::append_property_to_feature(
						cloned_geometry_property, no_plate_id_feature_ref);

				continue;
			}

			const GPlatesModel::ResolvedTopologicalBoundary *
					resolved_boundary_containing_most_geometry = NULL;
			if (!find_resolved_boundary_with_valid_plate_id_that_contains_most_geometry(
					resolved_boundary_containing_most_geometry,
					geometry_property_all_partitioned_geometries.partitioned_inside_geometries_seq))
			{
				// We shouldn't get here since we should have at least one
				// resolved boundary and it should have at least one partitioned geometry
				// in the current geometry property.
				continue;
			}

			// Assign geometry property to plate that contains largest proportion of
			// its geometry.
			const GPlatesModel::integer_plate_id_type reconstruction_plate_id =
					*resolved_boundary_containing_most_geometry->plate_id();

			// Get the composed absolute rotation needed to reverse reconstruct
			// geometries to present day.
			GPlatesMaths::FiniteRotation rotate_to_present_day =
					GPlatesMaths::get_reverse(
							reconstruction_tree.get_composed_absolute_rotation(
									reconstruction_plate_id).first);

			// If the original feature had a reconstruction plate id then it was
			// rotated to the reconstruction time so we need to do that too.
			if (original_reconstruction_rotation)
			{
				rotate_to_present_day = compose(
						rotate_to_present_day, *original_reconstruction_rotation);
			}

			// Clone the current source geometry property value.
			const GPlatesModel::TopLevelProperty::non_null_ptr_type cloned_geometry_property =
					(*geometry_properties_iterator)->clone();

			// Visit the geometry property and rotate its geometry using 'reverse_rotation'.
			GPlatesFeatureVisitors::GeometryRotator geometry_rotator(rotate_to_present_day);
			cloned_geometry_property->accept_visitor(geometry_rotator);

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
			GPlatesModel::ModelUtils::append_property_to_feature(
					cloned_geometry_property, recon_plate_id_feature_ref);

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
			const geometry_property_all_partitioned_geometries_seq_type &
					geometry_property_all_partitioned_geometries_seq,
			const resolved_boundary_geometry_properties_map_type &
					resolved_boundary_geometry_properties_map,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
			GPlatesModel::ModelInterface &model,
			const double &reconstruction_time,
			const GPlatesModel::ReconstructionTree &reconstruction_tree,
			const boost::optional<GPlatesMaths::FiniteRotation> &original_reconstruction_rotation)
	{
		bool assigned_any_plate_ids = false;

		static const GPlatesModel::PropertyName RECONSTRUCTION_PLATE_ID_PROPERTY_NAME =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		// First strip off any geometry properties from the feature.
		// Also remove any 'gpml:reconstructionPlateId' property(s).
		// This is so we can add new geometry properties and possibly
		// add a plate id (if inside a resolved boundary) later.
		remove_geometry_properties_from_feature(feature_ref);
		GPlatesModel::ModelUtils::remove_properties_from_feature_by_name(
				RECONSTRUCTION_PLATE_ID_PROPERTY_NAME, feature_ref);

		// Append any geometries that are partitioned outside all resolved boundaries.
		bool has_original_feature_been_assigned_to =
				assign_partitioned_geometry_outside_all_resolved_boundaries(
						geometry_property_all_partitioned_geometries_seq,
						feature_ref,
						original_reconstruction_rotation);

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
			const GPlatesModel::ResolvedTopologicalBoundary *resolved_topological_boundary =
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

			// Iterate over the geometry properties that have partitioned geometries
			// for the current resolved boundary.
			const geometry_property_partitioned_inside_geometries_seq_type &
					geometry_property_partitioned_inside_geometries_seq =
							resolved_boundary_geometry_properties
									.geometry_property_partitioned_inside_geometries_seq;
			geometry_property_partitioned_inside_geometries_seq_type::const_iterator
					geometry_property_partitioned_inside_geometries_iter =
							geometry_property_partitioned_inside_geometries_seq.begin();
			geometry_property_partitioned_inside_geometries_seq_type::const_iterator
					geometry_property_partitioned_inside_geometries_end =
							geometry_property_partitioned_inside_geometries_seq.end();
			for ( ;
				geometry_property_partitioned_inside_geometries_iter !=
					geometry_property_partitioned_inside_geometries_end;
				++geometry_property_partitioned_inside_geometries_iter)
			{
				const GeometryPropertyPartitionedInsideGeometries &
						geometry_property_partitioned_inside_geometries =
								*geometry_property_partitioned_inside_geometries_iter;

				const GPlatesModel::PropertyName &geometry_property_name =
						geometry_property_partitioned_inside_geometries.geometry_property_name;

				const GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type &
						partitioned_inside_geometry_seq =
								geometry_property_partitioned_inside_geometries
										.partitioned_geometries;

				GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type::const_iterator
						partitioned_inside_geometry_iter = partitioned_inside_geometry_seq.begin();
				GPlatesAppLogic::TopologyUtils::partitioned_geometry_seq_type::const_iterator
						paritioned_inside_geometry_end = partitioned_inside_geometry_seq.end();
				for ( ;
					partitioned_inside_geometry_iter != paritioned_inside_geometry_end;
					++partitioned_inside_geometry_iter)
				{
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &
							inside_geometry = *partitioned_inside_geometry_iter;

					// If the reconstruction time is not present day then we'll need to
					// reverse reconstruct back to present day before storing in feature.
					// Since we are working with geometry that has already been rotated
					// to the reconstruction time we do not need to take that into account.
					GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
							reverse_reconstructed_inside_geometry =
									(reconstruction_time > 0)
									? reverse_rotation * inside_geometry
									: inside_geometry;

					// Append a geometry property to the new feature.
					append_geometry_to_feature(
							reverse_reconstructed_inside_geometry,
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
				const GPlatesAppLogic::TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type &
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
				const GPlatesAppLogic::TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type &
						geometry_partition_query)
		{
			// Get the original reconstruction plate id before we remove it.
			boost::optional<GPlatesModel::integer_plate_id_type> original_reconstruction_plate_id =
					get_reconstruction_plate_id_from_feature(feature_ref);

			// Calculate the rotation used to reconstruct the feature geometry from
			// present day to the reconstruction time.
			// If there's no reconstruction plate id then the feature geometry is
			// interpreted to be a geometry snapshot at the current reconstruction time
			// and hence is already rotated to the reconstruction time.
			boost::optional<GPlatesMaths::FiniteRotation> original_reconstruction_rotation;
			if (original_reconstruction_plate_id)
			{
				// Get the composed absolute rotation needed to reconstruct
				// the feature to the reconstruction time specified by the user.
				original_reconstruction_rotation =
						d_reconstruction_tree.get_composed_absolute_rotation(
								*original_reconstruction_plate_id).first;
			}

			// Iterate over the geometry properties in the feature, rotate the
			// feature geometries from present day to the reconstruction time and
			// partition the rotated geometries using 'geometry_partition_query'.
			geometry_property_all_partitioned_geometries_seq_type
					geometry_property_all_partitioned_polylines_seq;
			PartitionFeatureGeometryVisitor partition_feature_geom_visitor(
					geometry_property_all_partitioned_polylines_seq,
					geometry_partition_query,
					original_reconstruction_rotation);
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
						d_reconstruction_tree,
						original_reconstruction_rotation);
				break;

			case GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_SUB_GEOMETRY_TO_MOST_OVERLAPPING_PLATE:
				assigned_any_plate_ids = assign_feature_sub_geometry_to_plate_it_overlaps_the_most(
						feature_ref,
						geometry_property_all_partitioned_polylines_seq,
						resolved_boundary_geometry_properties_map,
						d_feature_collection,
						d_model,
						d_reconstruction_time,
						d_reconstruction_tree,
						original_reconstruction_rotation);
				break;

			case GPlatesAppLogic::AssignPlateIds::PARTITION_FEATURE:
				assigned_any_plate_ids = partition_feature(
						feature_ref,
						geometry_property_all_partitioned_polylines_seq,
						resolved_boundary_geometry_properties_map,
						d_feature_collection,
						d_model,
						d_reconstruction_time,
						d_reconstruction_tree,
						original_reconstruction_rotation);
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

			return feature_ref->handle_data().feature_type() == vgp_feature_type;
		}


		bool
		assign_plate_id_to_feature(
				const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
				const GPlatesAppLogic::TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type &
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
						feature_ref->handle_data().feature_id().get());
				return false;
			}
			const GPlatesMaths::PointOnSphere &sample_site_point = *sample_site_gml_point->point();

			// Find all resolved boundaries that contain the sample site point.
			GPlatesAppLogic::TopologyUtils::resolved_topological_boundary_seq_type resolved_boundary_seq;
			if (!GPlatesAppLogic::TopologyUtils::find_resolved_topology_boundaries_containing_point(
					resolved_boundary_seq,
					sample_site_point,
					geometry_partition_query))
			{
				qDebug() << "WARNING: Unable to assign 'reconstructionPlateId' to "
						"'VirtualGeomagneticPole' with feature id = ";
				qDebug() << GPlatesUtils::make_qstring_from_icu_string(
						feature_ref->handle_data().feature_id().get());
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
						feature_ref->handle_data().feature_id().get());
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


GPlatesAppLogic::AssignPlateIds::AssignPlateIds(
		AssignPlateIdMethodType assign_plate_id_method,
		const GPlatesModel::ModelInterface &model,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				topological_boundary_feature_collections,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
				reconstruction_feature_collections,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchor_plate_id) :
	d_assign_plate_id_method(assign_plate_id_method),
	d_model(model),
	d_reconstruction_time(reconstruction_time),
	d_reconstruction(
			GPlatesAppLogic::ReconstructUtils::create_reconstruction(
					topological_boundary_feature_collections,
					reconstruction_feature_collections,
					reconstruction_time,
					anchor_plate_id))
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

	GPlatesModel::FeatureCollectionHandle::children_iterator feature_iter =
			feature_collection_ref->children_begin();
	GPlatesModel::FeatureCollectionHandle::children_iterator feature_end =
			feature_collection_ref->children_end();
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
