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

#include <algorithm>
#include <vector>
#include <boost/noncopyable.hpp>
#include <QDebug>
#include <QString>

#include "PartitionFeatureUtils.h"

#include "GeometryUtils.h"
#include "PartitionFeatureTask.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionTree.h"

#include "feature-visitors/GeometryRotator.h"
#include "feature-visitors/GeometryTypeFinder.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/FiniteRotation.h"

#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"
#include "model/PropertyName.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"


namespace
{
	/**
	 * Visit a feature property and, if it contains geometry, partitions it
	 * using partitioning polygons and stores results for later retrieval.
	 */
	class PartitionFeatureGeometryProperties :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		PartitionFeatureGeometryProperties(
				const GPlatesAppLogic::GeometryCookieCutter &geometry_cookie_cutter) :
			d_cookie_cut_geometry(geometry_cookie_cutter)
		{
			// The partitioning results will go here.
			d_partition_results.reset(
					new GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeature());
		}


		boost::shared_ptr<const GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeature>
		get_partitioned_feature_geometries() const
		{
			return d_partition_results;
		}

	protected:
		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string)
		{
			partition_geometry(gml_line_string.get_polyline());
		}


		virtual
		void
		visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
		{
			partition_geometry(gml_multi_point.get_multipoint());
		}


		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
		{
			gml_orientable_curve.get_base_curve()->accept_visitor(*this);
		}


		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point)
		{
			partition_geometry(gml_point.get_point());
		}


		virtual
		void
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &gml_polygon)
		{
			// We'll partition exterior and the interior polygons into this.
			GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeature::GeometryProperty &
					partition_geometry_property = add_geometry_property();

			// Partition the exterior polygon.
			partition_geometry(gml_polygon.get_exterior(), partition_geometry_property);

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
			const GPlatesPropertyValues::GmlPolygon::ring_sequence_type &interiors = gml_polygon.get_interiors();
			GPlatesPropertyValues::GmlPolygon::ring_sequence_type::const_iterator interior_iter = interiors.begin();
			GPlatesPropertyValues::GmlPolygon::ring_sequence_type::const_iterator interior_end = interiors.end();
			for ( ; interior_iter != interior_end; ++interior_iter)
			{
				const GPlatesPropertyValues::GmlPolygon::ring_type &interior_polygon = *interior_iter;

				// Partition interior polygon.
				partition_geometry(interior_polygon, partition_geometry_property);
			}
		}


		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.get_value()->accept_visitor(*this);
		}

	private:
		//! Does the cookie-cutting.
		const GPlatesAppLogic::GeometryCookieCutter &d_cookie_cut_geometry;

		//! The results of the cookie-cutting.
		boost::shared_ptr<GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeature> d_partition_results;


		/**
		 * Partition the geometry @a geometry of the current geometry property.
		 */
		void
		partition_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry)
		{
			// Create a new partition entry for the current geometry property.
			GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeature::GeometryProperty &
					partitioned_geometry_property = add_geometry_property();

			// Partition the current geometry property and store results.
			partition_geometry(geometry, partitioned_geometry_property);
		}

		GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeature::GeometryProperty &
		add_geometry_property()
		{
			// Property name of current geometry property.
			const GPlatesModel::PropertyName &geometry_property_name = *current_top_level_propname();

			// Create a shallow clone of the current geometry property.
			// This is quite quick to create compared to the deep clone since it's a bunch
			// of intrusive pointer copies.
			// This might be used by the caller to move a geometry property between features.
			// For example, if this geometry property requires a different plate id.
			const GPlatesModel::TopLevelProperty::non_null_ptr_type cloned_geometry_property =
					(**current_top_level_propiter())->clone();

			// Create a new entry for the current geometry property.
			d_partition_results->partitioned_geometry_properties.push_back(
					GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeature::GeometryProperty(
							geometry_property_name,
							cloned_geometry_property));

			// Get a reference to the entry just added.
			return d_partition_results->partitioned_geometry_properties.back();
		}

		inline
		void
		partition_geometry(
				const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
				GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeature::GeometryProperty &
						partitioned_geometry_property)
		{
			// Partition the current geometry property and store results.
			d_cookie_cut_geometry.partition_geometry(
					geometry,
					partitioned_geometry_property.partitioned_inside_geometries,
					partitioned_geometry_property.partitioned_outside_geometries);
		}
	};


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


	/**
	 * Calculates the accumulated size metric for all partitioned inside geometries
	 * of @a partition.
	 */
	GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric
	calculate_partition_size_metric(
			const GPlatesAppLogic::GeometryCookieCutter::Partition &partition)
	{
		typedef GPlatesAppLogic::GeometryCookieCutter::partitioned_geometry_seq_type
				partitioned_geometry_seq_type;

		GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric partition_size_metric;

		//
		// Iterate over the geometries inside the current partitioning polygon.
		//
		partitioned_geometry_seq_type::const_iterator inside_geometries_iter =
				partition.partitioned_geometries.begin();
		partitioned_geometry_seq_type::const_iterator inside_geometries_end =
				partition.partitioned_geometries.end();
		for ( ; inside_geometries_iter != inside_geometries_end; ++inside_geometries_iter)
		{
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &
					inside_geometry = *inside_geometries_iter;

			partition_size_metric.accumulate(*inside_geometry);
		}

		return partition_size_metric;
	}


	/**
	 * Returns "gpml:reconstructionPlateId".
	 */
	const GPlatesModel::PropertyName &
	get_reconstruction_plate_id_property_name()
	{
		static const GPlatesModel::PropertyName reconstruction_plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		return reconstruction_plate_id_property_name;
	}


	/**
	 * Returns true if @a top_level_prop_ptr is a 'gpml:reconstructionPlateId' property.
	 */
	bool
	is_reconstruction_plate_id_property(
			GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type top_level_prop_ptr)
	{
		return top_level_prop_ptr->get_property_name() == get_reconstruction_plate_id_property_name();
	}


	/**
	 * Don't clone geometry properties or 'gpml:reconstructionPlateId' properties.
	 */
	bool
	clone_feature_properties_except_geometry_and_plate_id(
			const GPlatesModel::TopLevelProperty::non_null_ptr_to_const_type &top_level_prop_ptr)
	{
		return !(
			GPlatesFeatureVisitors::is_geometry_property(top_level_prop_ptr) ||
			is_reconstruction_plate_id_property(top_level_prop_ptr)
			);
	}


	/**
	 * Returns "gml:validTime".
	 */
	const GPlatesModel::PropertyName &
	get_valid_time_property_name()
	{
		static const GPlatesModel::PropertyName valid_time_property_name =
				GPlatesModel::PropertyName::create_gml("validTime");

		return valid_time_property_name;
	}
}


boost::shared_ptr<const GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeature>
GPlatesAppLogic::PartitionFeatureUtils::partition_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
		const GPlatesAppLogic::GeometryCookieCutter &geometry_cookie_cutter,
		bool respect_feature_time_period)
{
	// Only partition features that exist at the partitioning reconstruction time if we've been requested.
	if (respect_feature_time_period &&
		!does_feature_exist_at_reconstruction_time(
			feature_ref,
			geometry_cookie_cutter.get_reconstruction_time()))
	{
		return boost::shared_ptr<const GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeature>();
	}

	PartitionFeatureGeometryProperties feature_partitioner(geometry_cookie_cutter);
	feature_partitioner.visit_feature(feature_ref);

	return feature_partitioner.get_partitioned_feature_geometries();
}


GPlatesAppLogic::PartitionFeatureUtils::GenericFeaturePropertyAssigner::GenericFeaturePropertyAssigner(
		const GPlatesModel::FeatureHandle::const_weak_ref &original_feature,
		const GPlatesAppLogic::AssignPlateIds::feature_property_flags_type &feature_property_types_to_assign) :
	d_default_reconstruction_plate_id(
			get_reconstruction_plate_id_from_feature(original_feature)),
	d_default_valid_time(
			get_valid_time_from_feature(original_feature)),
	d_feature_property_types_to_assign(feature_property_types_to_assign)
{
}


void
GPlatesAppLogic::PartitionFeatureUtils::GenericFeaturePropertyAssigner::assign_property_values(
		const GPlatesModel::FeatureHandle::weak_ref &partitioned_feature,
		boost::optional<GPlatesModel::FeatureHandle::const_weak_ref> partitioning_feature)
{
	// Merge model events across this scope to avoid excessive number of model callbacks.
	GPlatesModel::NotificationGuard model_notification_guard(*partitioned_feature->model_ptr());

	// Get the reconstruction plate id.
	// Either from the partitioning feature or the default plate id.
	// If we are not supposed to assign plate ids then use the default plate id as
	// that has the effect of keeping the original plate id.
	boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id;
	if (partitioning_feature &&
		d_feature_property_types_to_assign.test(AssignPlateIds::RECONSTRUCTION_PLATE_ID))
	{
		reconstruction_plate_id = get_reconstruction_plate_id_from_feature(
				partitioning_feature.get());
	}
	else
	{
		reconstruction_plate_id = d_default_reconstruction_plate_id;
	}
	assign_reconstruction_plate_id_to_feature(
			reconstruction_plate_id,
			partitioned_feature);

	// Get the time period.
	// Either from the partitioning feature or the default time period.
	// If we are not supposed to assign plate ids then use the default plate id as
	// that has the effect of keeping the original plate id.
	boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> valid_time;
	if (partitioning_feature &&
		d_feature_property_types_to_assign.test(AssignPlateIds::VALID_TIME))
	{
		valid_time = get_valid_time_from_feature(partitioning_feature.get());
	}
	else
	{
		valid_time = d_default_valid_time;
	}
	assign_valid_time_to_feature(
			valid_time,
			partitioned_feature);
}


void
GPlatesAppLogic::PartitionFeatureUtils::add_partitioned_geometry_to_feature(
		const GPlatesAppLogic::GeometryCookieCutter::partitioned_geometry_seq_type &
				partitioned_geometries,
		const GPlatesModel::PropertyName &geometry_property_name,
		PartitionedFeatureManager &partitioned_feature_manager,
		const ReconstructionTree &reconstruction_tree,
		boost::optional<const ReconstructionGeometry *> partition)
{
	// Calculate the reverse rotation if we did not cookie cut at present day AND
	// if there's a partitioning polygon AND it has a plate id.
	const boost::optional<GPlatesMaths::FiniteRotation> reverse_rotation =
			get_reverse_reconstruction(partition, reconstruction_tree);

	//
	// Iterate over the partitioned geometries.
	//
	GeometryCookieCutter::partitioned_geometry_seq_type::const_iterator partitioned_geometries_iter =
			partitioned_geometries.begin();
	GeometryCookieCutter::partitioned_geometry_seq_type::const_iterator partitioned_geometries_end =
			partitioned_geometries.end();
	for ( ; partitioned_geometries_iter != partitioned_geometries_end; ++partitioned_geometries_iter)
	{
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &partitioned_geometry =
				*partitioned_geometries_iter;

		// If there's a reverse rotation then we'll need to apply it to reconstruct
		// back to present day before storing the geometry in the feature.
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
				present_day_partitioned_geometry =
						reverse_rotation
						? reverse_rotation.get() * partitioned_geometry
						: partitioned_geometry;

		// Note that we only get the partitioned feature when we know we
		// are going to append a geometry property to it.
		// If there are no partitioned geometries then it doesn't get called
		// which means a new feature won't get cloned.
		const GPlatesModel::FeatureHandle::weak_ref partitioned_feature =
				partitioned_feature_manager.get_feature_for_partition(partition);

		append_geometry_to_feature(
				present_day_partitioned_geometry,
				geometry_property_name,
				partitioned_feature);
	}
}


void
GPlatesAppLogic::PartitionFeatureUtils::add_partitioned_geometry_to_feature(
		const GPlatesAppLogic::GeometryCookieCutter::partition_seq_type &partitions,
		const GPlatesModel::PropertyName &geometry_property_name,
		PartitionedFeatureManager &partitioned_feature_manager,
		const ReconstructionTree &reconstruction_tree)
{
	//
	// Iterate over the partitioning polygons and add the inside geometries to features.
	//
	GeometryCookieCutter::partition_seq_type::const_iterator partition_iter = partitions.begin();
	GeometryCookieCutter::partition_seq_type::const_iterator partition_end = partitions.end();
	for ( ; partition_iter != partition_end; ++partition_iter)
	{
		const GeometryCookieCutter::Partition &partition = *partition_iter;

		add_partitioned_geometry_to_feature(
				partition.partitioned_geometries,
				geometry_property_name,
				partitioned_feature_manager,
				reconstruction_tree,
				partition.reconstruction_geometry.get());
	}
}


void
GPlatesAppLogic::PartitionFeatureUtils::add_partitioned_geometry_to_feature(
		const GPlatesModel::TopLevelProperty::non_null_ptr_type &geometry_property,
		PartitionFeatureUtils::PartitionedFeatureManager &partitioned_feature_manager,
		const ReconstructionTree &reconstruction_tree,
		boost::optional<const ReconstructionGeometry *> partition)
{
	const GPlatesModel::FeatureHandle::weak_ref feature =
			partitioned_feature_manager.get_feature_for_partition(partition);

	// We're effectively transferring the geometry property from the original
	// feature to the new feature (note that they could both be the same
	// in which case we've added and then removed the same property from same feature).
	const GPlatesModel::FeatureHandle::iterator feature_iterator = feature->add(geometry_property);

	// See if we need to apply a reverse reconstruction to the geometry property.
	const boost::optional<GPlatesMaths::FiniteRotation> reverse_rotation =
			get_reverse_reconstruction(partition, reconstruction_tree);

	if (reverse_rotation)
	{
		// Visit the geometry property just added and rotate its geometry using 'reverse_rotation'.
		GPlatesFeatureVisitors::GeometryRotator geometry_rotator(reverse_rotation.get());

		const GPlatesModel::TopLevelProperty::non_null_ptr_type geometry_property_clone =
				(*feature_iterator)->clone();
		geometry_property_clone->accept_visitor(geometry_rotator);
		*feature_iterator = geometry_property_clone;
	}
}


boost::optional<const GPlatesAppLogic::ReconstructionGeometry *>
GPlatesAppLogic::PartitionFeatureUtils::find_partition_containing_most_geometry(
		const PartitionedFeature::GeometryProperty &geometry_property)
{
	const GeometryCookieCutter::partition_seq_type &partitions =
			geometry_property.partitioned_inside_geometries;

	// Return early if no partitions.
	if (partitions.empty())
	{
		return boost::none;
	}

	// If there's only one partition then we return it.
	if (partitions.size() == 1)
	{
		return partitions.front().reconstruction_geometry.get();
	}

	GeometrySizeMetric max_partition_size_metric;
	boost::optional<const ReconstructionGeometry *> max_partition;

	//
	// Iterate over the partitioning polygons to see which one contains the most geometry.
	//
	GeometryCookieCutter::partition_seq_type::const_iterator partition_iter = partitions.begin();
	GeometryCookieCutter::partition_seq_type::const_iterator partition_end = partitions.end();
	for ( ; partition_iter != partition_end; ++partition_iter)
	{
		const GeometryCookieCutter::Partition &partition = *partition_iter;

		const GeometrySizeMetric partition_size_metric =
				calculate_partition_size_metric(partition);

		// If the current partition contains more geometry then
		// set it as the maximum partition so far.
		if (partition_size_metric > max_partition_size_metric)
		{
			max_partition_size_metric = partition_size_metric;
			max_partition = partition.reconstruction_geometry.get();
		}
	}

	return max_partition;
}


boost::optional<const GPlatesAppLogic::ReconstructionGeometry *>
GPlatesAppLogic::PartitionFeatureUtils::find_partition_containing_most_geometry(
		const PartitionedFeature &partitioned_feature)
{
	const PartitionedFeature::partitioned_geometry_property_seq_type &geometry_properties =
			partitioned_feature.partitioned_geometry_properties;

	// Return early if no geometry properties.
	if (geometry_properties.empty())
	{
		return boost::none;
	}

	// If there's only one geometry property.
	if (geometry_properties.size() == 1)
	{
		return find_partition_containing_most_geometry(geometry_properties.front());
	}

	// Keep track of the various partitions and their size metrics.
	typedef std::map<
			const ReconstructionGeometry *,
			GeometrySizeMetric> partition_size_metrics_type;
	partition_size_metrics_type partition_size_metrics;

	GeometrySizeMetric max_partition_size_metric;
	boost::optional<const ReconstructionGeometry *> max_partition;

	//
	// Iterate over the geometry properties.
	//
	PartitionedFeature::partitioned_geometry_property_seq_type::const_iterator geometry_property_iter =
			geometry_properties.begin();
	PartitionedFeature::partitioned_geometry_property_seq_type::const_iterator geometry_property_end =
			geometry_properties.end();
	for ( ; geometry_property_iter != geometry_property_end; ++geometry_property_iter)
	{
		const PartitionedFeature::GeometryProperty &geometry_property = *geometry_property_iter;

		//
		// Iterate over the partitioning polygons and accumulate size metrics.
		//
		GeometryCookieCutter::partition_seq_type::const_iterator partition_iter =
				geometry_property.partitioned_inside_geometries.begin();
		GeometryCookieCutter::partition_seq_type::const_iterator partition_end =
				geometry_property.partitioned_inside_geometries.end();
		for ( ; partition_iter != partition_end; ++partition_iter)
		{
			const GeometryCookieCutter::Partition &partition = *partition_iter;

			// Get the map entry keyed by the partition's partitioning reconstruction geometry.
			GeometrySizeMetric &partition_size_metric =
					partition_size_metrics[partition.reconstruction_geometry.get()];

			// Accumulate the geometry metric for partitioned inside geometries
			// of the current partition.
			partition_size_metric.accumulate(
					calculate_partition_size_metric(partition));

			// If the current partition contains more geometry then
			// set it as the maximum partition so far.
			if (partition_size_metric > max_partition_size_metric)
			{
				max_partition_size_metric = partition_size_metric;
				max_partition = partition.reconstruction_geometry.get();
			}
		}
	}

	return max_partition;
}


bool
GPlatesAppLogic::PartitionFeatureUtils::does_feature_exist_at_reconstruction_time(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
		const double &reconstruction_time)
{
	GPlatesAppLogic::ReconstructionFeatureProperties reconstruction_params(reconstruction_time);

	reconstruction_params.visit_feature(feature_ref);

	return reconstruction_params.is_feature_defined_at_recon_time();
}


boost::optional<GPlatesMaths::FiniteRotation>
GPlatesAppLogic::PartitionFeatureUtils::get_reverse_reconstruction(
		boost::optional<const ReconstructionGeometry *> partition,
		const ReconstructionTree &reconstruction_tree)
{
	const GPlatesMaths::real_t reconstruction_time = reconstruction_tree.get_reconstruction_time();
	if (partition && (reconstruction_time > 0))
	{
		// Get the reconstruction plate id from the partitioning polygon.
		const boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id =
				ReconstructionGeometryUtils::get_plate_id(partition.get());

		// If the current partitioning polygon has a plate id then find the
		// reverse rotation to rotate the partitioned geometries back to present day.
		if (reconstruction_plate_id)
		{
			// Get the composed absolute rotation needed to reverse reconstruct
			// geometries to present day.
			return GPlatesMaths::get_reverse(
					reconstruction_tree.get_composed_absolute_rotation(
							reconstruction_plate_id.get()).first);
		}
	}

	return boost::none;
}


boost::optional<GPlatesModel::integer_plate_id_type>
GPlatesAppLogic::PartitionFeatureUtils::get_reconstruction_plate_id_from_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	const GPlatesPropertyValues::GpmlPlateId *recon_plate_id = NULL;
	boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id;
	if (!GPlatesFeatureVisitors::get_property_value(
			feature_ref,
			get_reconstruction_plate_id_property_name(),
			recon_plate_id))
	{
		return boost::none;
	}

	return recon_plate_id->get_value();
}


void
GPlatesAppLogic::PartitionFeatureUtils::assign_reconstruction_plate_id_to_feature(
		boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	// Merge model events across this scope to avoid excessive number of model callbacks.
	GPlatesModel::NotificationGuard model_notification_guard(*feature_ref->model_ptr());

	// First remove any that might already exist.
	feature_ref->remove_properties_by_name(get_reconstruction_plate_id_property_name());

	// Only assign a new plate id if we've been given one.
	if (!reconstruction_plate_id)
	{
		return;
	}

	// Append a new property to the feature.
	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(reconstruction_plate_id.get());
	feature_ref->add(
			GPlatesModel::TopLevelPropertyInline::create(
				get_reconstruction_plate_id_property_name(),
				GPlatesModel::ModelUtils::create_gpml_constant_value(gpml_plate_id)));
}


boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type>
GPlatesAppLogic::PartitionFeatureUtils::get_valid_time_from_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	const GPlatesPropertyValues::GmlTimePeriod *time_period = NULL;
	if (!GPlatesFeatureVisitors::get_property_value(
			feature_ref,
			get_valid_time_property_name(),
			time_period))
	{
		return boost::none;
	}

	return GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type(time_period);
}


void
GPlatesAppLogic::PartitionFeatureUtils::assign_valid_time_to_feature(
		boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> valid_time,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	// First remove any that might already exist.
	feature_ref->remove_properties_by_name(get_valid_time_property_name());

	// Only assign a new time period if we've been given one.
	if (!valid_time)
	{
		return;
	}

	// Append a new property to the feature.
	feature_ref->add(
			GPlatesModel::TopLevelPropertyInline::create(
				get_valid_time_property_name(),
				valid_time.get()->clone()));
}


void
GPlatesAppLogic::PartitionFeatureUtils::append_geometry_to_feature(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const GPlatesModel::PropertyName &geometry_property_name,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> geometry_property =
			GeometryUtils::create_geometry_property_value(geometry);

	if (geometry_property)
	{
		feature_ref->add(
				GPlatesModel::TopLevelPropertyInline::create(
					geometry_property_name,
					*geometry_property));
	}
}


GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeatureManager::PartitionedFeatureManager(
		const GPlatesModel::FeatureHandle::weak_ref &original_feature,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		const boost::shared_ptr<PropertyValueAssigner> &property_value_assigner) :
	d_original_feature(original_feature),
	d_feature_collection(feature_collection),
	d_has_original_feature_been_claimed(false),
	d_property_value_assigner(property_value_assigner)
{
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeatureManager::get_feature_for_partition(
		boost::optional<const ReconstructionGeometry *> partition)
{
	// See if we've already mapped the partition to a feature.
	partition_to_feature_map_type::iterator feature_iter =
			d_partitioned_features.find(partition);
	if (feature_iter != d_partitioned_features.end())
	{
		return feature_iter->second;
	}

	// Create a new feature.
	const GPlatesModel::FeatureHandle::weak_ref new_feature = create_feature();

	// Assign property values (from 'partition's feature) to the new feature.
	assign_property_values(new_feature, partition);

	d_partitioned_features.insert(std::make_pair(partition, new_feature));
	
	return new_feature;
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeatureManager::create_feature()
{
	if (!d_has_original_feature_been_claimed)
	{
		d_has_original_feature_been_claimed = true;
		return d_original_feature;
	}

	return d_original_feature->clone(
			d_feature_collection,
			&GPlatesFeatureVisitors::is_not_geometry_property);
}


void
GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeatureManager::assign_property_values(
		const GPlatesModel::FeatureHandle::weak_ref &partitioned_feature,
		boost::optional<const ReconstructionGeometry *> partition)
{
	boost::optional<GPlatesModel::FeatureHandle::const_weak_ref> partitioning_feature_opt;

	// If there's a partitioning polygon then get its feature so we can copy
	// property values from it.
	if (partition)
	{
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> partitioning_feature =
				ReconstructionGeometryUtils::get_feature_ref(partition.get());
		if (partitioning_feature)
		{
			partitioning_feature_opt = partitioning_feature;
		}
	}

	d_property_value_assigner->assign_property_values(
			partitioned_feature,
			partitioning_feature_opt);
}


GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::GeometrySizeMetric() :
	d_num_points(0),
	d_arc_distance(0),
	d_using_arc_distance(false)
{
}


void
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::accumulate(
		const GPlatesMaths::GeometryOnSphere &geometry)
{
	GeometrySize geometry_size(d_num_points, d_arc_distance, d_using_arc_distance);
	geometry.accept_visitor(geometry_size);
}


void
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::accumulate(
		const GeometrySizeMetric &geometry_size_metric)
{
	d_num_points += geometry_size_metric.d_num_points;
	d_arc_distance += geometry_size_metric.d_arc_distance;

	if (geometry_size_metric.d_using_arc_distance)
	{
		d_using_arc_distance = true;
	}
}


bool
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::operator<(
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
