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
#include <map>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QDebug>
#include <QString>

#include "PartitionFeatureUtils.h"

#include "GeometrySetter.h"
#include "GeometryUtils.h"
#include "PartitionFeatureTask.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionTree.h"
#include "ReconstructMethodRegistry.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/FiniteRotation.h"

#include "model/Gpgim.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"
#include "model/PropertyName.h"
#include "model/PropertyValueFinder.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GpmlPlateId.h"
#include "property-values/GmlDataBlock.h"


namespace GPlatesAppLogic
{
	namespace PartitionFeatureUtils
	{
		namespace
		{
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
					distance += gca_iter->arc_length();
				}

				return distance;
			}


			/**
			 * Accumulates a geometry's size: a polygon's area, a polyline's arc length, a point's or
			 * multi-point's number of points (spherical-geometry.md section 5.4). A polygon's arc
			 * length is not used: its cuts would inflate it.
			 */
			class GeometrySize :
					public GPlatesMaths::ConstGeometryOnSphereVisitor
			{
			public:
				GeometrySize(
						unsigned int &num_points,
						GPlatesMaths::real_t &arc_distance,
						GPlatesMaths::real_t &area) :
					d_num_points(num_points),
					d_arc_distance(arc_distance),
					d_area(area)
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
						GPlatesMaths::PointGeometryOnSphere::non_null_ptr_to_const_type /*point_on_sphere*/)
				{
					++d_num_points;
				}

				virtual
				void
				visit_polygon_on_sphere(
						GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
				{
					d_area += polygon_on_sphere->get_area();
				}

				virtual
				void
				visit_polyline_on_sphere(
						GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
				{
					d_arc_distance += polyline_on_sphere->get_arc_length();
				}

				unsigned int &d_num_points;
				GPlatesMaths::real_t &d_arc_distance;
				GPlatesMaths::real_t &d_area;
			};


			/**
			 * Calculates the accumulated size metric for all partitioned inside geometries
			 * of @a partition.
			 */
			GeometrySizeMetric
			calculate_partition_size_metric(
					const PartitionedFeature::Partition &partition)
			{
				GeometrySizeMetric partition_size_metric;

				//
				// Iterate over the geometries inside the current partitioning polygon.
				//
				PartitionedFeature::partitioned_geometry_seq_type::const_iterator inside_geometries_iter =
						partition.partitioned_geometries.begin();
				PartitionedFeature::partitioned_geometry_seq_type::const_iterator inside_geometries_end =
						partition.partitioned_geometries.end();
				for ( ; inside_geometries_iter != inside_geometries_end; ++inside_geometries_iter)
				{
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &inside_geometry =
							inside_geometries_iter->geometry_domain;

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
			 * Returns "gpml:conjugatePlateId".
			 */
			const GPlatesModel::PropertyName &
			get_conjugate_plate_id_property_name()
			{
				static const GPlatesModel::PropertyName conjugate_plate_id_property_name =
						GPlatesModel::PropertyName::create_gpml("conjugatePlateId");

				return conjugate_plate_id_property_name;
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


			/**
			 * Adds partitioned geometries to the partitioned feature associated with @a partition.
			 *
			 * If @a partition is none then adds to the special feature associated with no partition.
			 *
			 * All partitioned geometries are reverse reconstructed using the plate id of their partitioning polygon
			 * (if has a plate id) and/or deformed if @a reconstruct_method_context contains deformation.
			 */
			void
			add_partitioned_geometries_to_feature(
					const PartitionedFeature::partitioned_geometry_seq_type &partitioned_geometries,
					const GPlatesModel::PropertyName &geometry_domain_property_name,
					const boost::optional<GPlatesModel::PropertyName> &geometry_range_property_name,
					PartitionedFeatureManager &partitioned_feature_manager,
					const ReconstructMethodInterface::Context &reconstruct_method_context,
					const double &reconstruction_time,
					boost::optional<const ReconstructionGeometry *> partition = boost::none)
			{
				//
				// Iterate over the partitioned geometries.
				//
				PartitionedFeature::partitioned_geometry_seq_type::const_iterator partitioned_geometries_iter =
						partitioned_geometries.begin();
				PartitionedFeature::partitioned_geometry_seq_type::const_iterator partitioned_geometries_end =
						partitioned_geometries.end();
				for ( ; partitioned_geometries_iter != partitioned_geometries_end; ++partitioned_geometries_iter)
				{
					const PartitionedFeature::PartitionedGeometry &partitioned_geometry = *partitioned_geometries_iter;

					const bool geometry_domain_has_associated_range =
							geometry_range_property_name && partitioned_geometry.geometry_range;

					// Note that we only get the partitioned feature when we know we
					// are going to append a geometry property to it.
					// If there are no partitioned geometries then it doesn't get called
					// which means a new feature won't get cloned. 
					const GPlatesModel::FeatureHandle::weak_ref partitioned_feature =
							partitioned_feature_manager.get_feature_for_partition(
									geometry_domain_property_name,
									geometry_domain_has_associated_range,
									partition);

					// Reverse reconstruct to get the present day geometry.
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_partitioned_geometry =
							reverse_reconstruct(
									partitioned_geometry.geometry_domain,
									partitioned_feature,
									reconstruct_method_context,
									reconstruction_time);

					// Add the geometry domain property.
					append_geometry_domain_to_feature(
							present_day_partitioned_geometry,
							geometry_domain_property_name,
							partitioned_feature);

					// If there's an associated geometry range then add it as a property.
					if (geometry_domain_has_associated_range)
					{
						append_geometry_range_to_feature(
								partitioned_geometry.geometry_range.get(),
								geometry_range_property_name.get(),
								partitioned_feature);
					}
				}
			}
		}
	}
}


GPlatesAppLogic::PartitionFeatureUtils::GenericFeaturePropertyAssigner::GenericFeaturePropertyAssigner(
		const GPlatesModel::FeatureHandle::const_weak_ref &original_feature,
		const AssignPlateIds::feature_property_flags_type &feature_property_types_to_assign,
		bool verify_information_model) :
	d_verify_information_model(verify_information_model),
	d_default_reconstruction_plate_id(
			get_reconstruction_plate_id_from_feature(original_feature)),
	d_default_conjugate_plate_id(
			get_conjugate_plate_id_from_feature(original_feature)),
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
	GPlatesModel::NotificationGuard model_notification_guard(partitioned_feature->model_ptr());

	// Get the reconstruction plate id.
	// Either from the partitioning feature or the default reconstruction plate id.
	// If we are not supposed to assign plate ids then use the default reconstruction plate id as
	// that has the effect of keeping the original reconstruction plate id.
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
			partitioned_feature,
			d_verify_information_model);

	// Get the conjugate plate id.
	// Either from the partitioning feature or the default conjugate plate id.
	// If we are not supposed to assign plate ids then use the default conjugate plate id as
	// that has the effect of keeping the original conjugate plate id.
	boost::optional<GPlatesModel::integer_plate_id_type> conjugate_plate_id;
	if (partitioning_feature &&
		d_feature_property_types_to_assign.test(AssignPlateIds::CONJUGATE_PLATE_ID))
	{
		conjugate_plate_id = get_conjugate_plate_id_from_feature(
				partitioning_feature.get());
	}
	else
	{
		conjugate_plate_id = d_default_conjugate_plate_id;
	}
	assign_conjugate_plate_id_to_feature(
			conjugate_plate_id,
			partitioned_feature,
			d_verify_information_model);

	// Get the time period.
	// Either from the partitioning feature or the default time period or a mixture of both.
	// If we are not supposed to assign time periods then use the default time period as
	// that has the effect of keeping the original time period.
	boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> valid_time;
	if (partitioning_feature && (
		d_feature_property_types_to_assign.test(AssignPlateIds::TIME_OF_APPEARANCE) ||
		d_feature_property_types_to_assign.test(AssignPlateIds::TIME_OF_DISAPPEARANCE)))
	{
		valid_time = get_valid_time_from_feature(partitioning_feature.get());

		if (valid_time)
		{
			// If only copying time of disappearance (not appearance) then replace the appearance time
			// with the default appearance time (or distant past if none).
			if (!d_feature_property_types_to_assign.test(AssignPlateIds::TIME_OF_APPEARANCE))
			{
				valid_time = GPlatesModel::ModelUtils::create_gml_time_period(
						d_default_valid_time
								? d_default_valid_time.get()->begin()->get_time_position()
								: GPlatesPropertyValues::GeoTimeInstant::create_distant_past(),
						valid_time.get()->end()->get_time_position());
			}

			// If only copying time of appearance (not disappearance) then replace the disappearance time
			// with the default disappearance time (or distant future if none).
			if (!d_feature_property_types_to_assign.test(AssignPlateIds::TIME_OF_DISAPPEARANCE))
			{
				valid_time = GPlatesModel::ModelUtils::create_gml_time_period(
						valid_time.get()->begin()->get_time_position(),
						d_default_valid_time
								? d_default_valid_time.get()->end()->get_time_position()
								: GPlatesPropertyValues::GeoTimeInstant::create_distant_future());
			}
		}
	}
	else
	{
		valid_time = d_default_valid_time;
	}
	assign_valid_time_to_feature(
			valid_time,
			partitioned_feature,
			d_verify_information_model);
}


void
GPlatesAppLogic::PartitionFeatureUtils::add_partitioned_geometry_to_feature(
		const PartitionedFeature::GeometryProperty &geometry_property,
		PartitionedFeatureManager &partitioned_feature_manager,
		const ReconstructMethodInterface::Context &reconstruct_method_context,
		const double &reconstruction_time)
{
	// Iterate over the partitioning polygons and add the *inside* geometries to features.
	PartitionedFeature::partition_seq_type::const_iterator partition_iter =
			geometry_property.partitioned_inside_geometries.begin();
	PartitionedFeature::partition_seq_type::const_iterator partition_end =
			geometry_property.partitioned_inside_geometries.end();
	for ( ; partition_iter != partition_end; ++partition_iter)
	{
		const PartitionedFeature::Partition &partition = *partition_iter;

		add_partitioned_geometries_to_feature(
				partition.partitioned_geometries,
				geometry_property.domain_property_name,
				geometry_property.range_property_name,
				partitioned_feature_manager,
				reconstruct_method_context,
				reconstruction_time,
				partition.reconstruction_geometry.get());
	}

	// Add partitioned *outside* geometries and to special feature associated with no partition.
	add_partitioned_geometries_to_feature(
			geometry_property.partitioned_outside_geometries,
			geometry_property.domain_property_name,
			geometry_property.range_property_name,
			partitioned_feature_manager,
			reconstruct_method_context,
			reconstruction_time);
}


void
GPlatesAppLogic::PartitionFeatureUtils::add_unpartitioned_geometry_to_feature(
		const PartitionedFeature::GeometryProperty &geometry_property,
		PartitionedFeatureManager &partitioned_feature_manager,
		const ReconstructMethodInterface::Context &reconstruct_method_context,
		const double &reconstruction_time,
		boost::optional<const ReconstructionGeometry *> partition)
{
	PartitionedFeature::geometry_property_clone_seq_type::const_iterator property_clone_iter =
			geometry_property.property_clones.begin();
	PartitionedFeature::geometry_property_clone_seq_type::const_iterator property_clone_end =
			geometry_property.property_clones.end();
	for ( ; property_clone_iter != property_clone_end; ++property_clone_iter)
	{
		const PartitionedFeature::GeometryPropertyClone &property_clone = *property_clone_iter;

		const GPlatesModel::FeatureHandle::weak_ref feature =
				partitioned_feature_manager.get_feature_for_partition(
						geometry_property.domain_property_name,
						static_cast<bool>(property_clone.range)/*geometry_domain_has_associated_range*/,
						partition);

		// Extract the geometry from the geometry property clone.
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> reconstructed_geometry =
				GeometryUtils::get_geometry_from_property(property_clone.domain);
		if (!reconstructed_geometry)
		{
			// Shouldn't get here since geometry property should contain a geometry.
			continue;
		}

		// Reverse reconstruct to get the present day geometry.
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_geometry =
				reverse_reconstruct(reconstructed_geometry.get(), feature, reconstruct_method_context, reconstruction_time);

		// Store the present day geometry back in the cloned property.
		GPlatesAppLogic::GeometrySetter geometry_setter(present_day_geometry);
		geometry_setter.set_geometry(property_clone.domain.get());

		// Set the cloned geometry domain property (and optional cloned range property) on the feature.
		const GPlatesModel::FeatureHandle::iterator geometry_domain_feature_iterator =
				feature->add(property_clone.domain);
		if (property_clone.range)
		{
			feature->add(property_clone.range.get());
		}
	}
}


boost::optional<const GPlatesAppLogic::ReconstructionGeometry *>
GPlatesAppLogic::PartitionFeatureUtils::find_partition_containing_most_geometry(
		const PartitionedFeature &partitioned_feature)
{
	const PartitionedFeature::partitioned_geometry_property_map_type &geometry_properties =
			partitioned_feature.partitioned_geometry_properties;

	// Return early if no geometry properties.
	if (geometry_properties.empty())
	{
		return boost::none;
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
	PartitionedFeature::partitioned_geometry_property_map_type::const_iterator geometry_property_iter =
			geometry_properties.begin();
	PartitionedFeature::partitioned_geometry_property_map_type::const_iterator geometry_property_end =
			geometry_properties.end();
	for ( ; geometry_property_iter != geometry_property_end; ++geometry_property_iter)
	{
		const PartitionedFeature::GeometryProperty &geometry_property = geometry_property_iter->second;

		//
		// Iterate over the partitioning polygons and accumulate size metrics.
		//
		PartitionedFeature::partition_seq_type::const_iterator partition_iter =
				geometry_property.partitioned_inside_geometries.begin();
		PartitionedFeature::partition_seq_type::const_iterator partition_end =
				geometry_property.partitioned_inside_geometries.end();
		for ( ; partition_iter != partition_end; ++partition_iter)
		{
			const PartitionedFeature::Partition &partition = *partition_iter;

			// Get the map entry keyed by the partition's partitioning reconstruction geometry.
			GeometrySizeMetric &partition_size_metric =
					partition_size_metrics[partition.reconstruction_geometry.get()];

			// Accumulate the geometry metric for partitioned inside geometries
			// of the current partition.
			partition_size_metric.accumulate(
					calculate_partition_size_metric(partition));

			// If the current partition contains more geometry then set it as the maximum
			// partition so far. The first partition with anything inside is taken whatever its
			// size, so that a geometry of no measure (a polygon whose edges are all too short for
			// an area, a polyline at one position) still gets a plate, as in pyGPlates.
			if (!max_partition || partition_size_metric > max_partition_size_metric)
			{
				max_partition_size_metric = partition_size_metric;
				max_partition = partition.reconstruction_geometry.get();
			}
		}
	}

	return max_partition;
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::PartitionFeatureUtils::reverse_reconstruct(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &reconstructed_geometry,
		const GPlatesModel::FeatureHandle::weak_ref &feature,
		const ReconstructMethodInterface::Context &reconstruct_method_context,
		const double &reconstruction_time)
{
	if (!feature.is_valid())
	{
		return reconstructed_geometry;
	}

	const ReconstructMethodRegistry reconstruct_method_registry;

	return ReconstructUtils::reconstruct_geometry(
					reconstructed_geometry,
					reconstruct_method_registry,
					feature,
					reconstruction_time,
					reconstruct_method_context,
					true/*reverse_reconstruct*/);
}


boost::optional<GPlatesModel::integer_plate_id_type>
GPlatesAppLogic::PartitionFeatureUtils::get_reconstruction_plate_id_from_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> recon_plate_id =
			GPlatesModel::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
					feature_ref,
					get_reconstruction_plate_id_property_name());
	if (!recon_plate_id)
	{
		return boost::none;
	}

	return recon_plate_id.get()->get_value();
}


void
GPlatesAppLogic::PartitionFeatureUtils::assign_reconstruction_plate_id_to_feature(
		boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		bool verify_information_model)
{
	// Merge model events across this scope to avoid excessive number of model callbacks.
	GPlatesModel::NotificationGuard model_notification_guard(feature_ref->model_ptr());

	// First remove any that might already exist.
	feature_ref->remove_properties_by_name(get_reconstruction_plate_id_property_name());

	// Only assign a new reconstruction plate id if we've been given one.
	if (!reconstruction_plate_id)
	{
		return;
	}

	// Append a new property to the feature.
	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_reconstruction_plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(reconstruction_plate_id.get());
	// If 'verify_information_model' is true then property is only added if it doesn't not violate the GPGIM.
	GPlatesModel::ModelUtils::add_property(
			feature_ref,
			get_reconstruction_plate_id_property_name(),
			gpml_reconstruction_plate_id,
			verify_information_model/*check_property_name_allowed_for_feature_type*/);
}


boost::optional<GPlatesModel::integer_plate_id_type>
GPlatesAppLogic::PartitionFeatureUtils::get_conjugate_plate_id_from_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> conjugate_plate_id =
			GPlatesModel::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
					feature_ref,
					get_conjugate_plate_id_property_name());
	if (!conjugate_plate_id)
	{
		return boost::none;
	}

	return conjugate_plate_id.get()->get_value();
}


void
GPlatesAppLogic::PartitionFeatureUtils::assign_conjugate_plate_id_to_feature(
		boost::optional<GPlatesModel::integer_plate_id_type> conjugate_plate_id,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		bool verify_information_model)
{
	// Merge model events across this scope to avoid excessive number of model callbacks.
	GPlatesModel::NotificationGuard model_notification_guard(feature_ref->model_ptr());

	// First remove any that might already exist.
	feature_ref->remove_properties_by_name(get_conjugate_plate_id_property_name());

	// Only assign a new conjugate plate id if we've been given one.
	if (!conjugate_plate_id)
	{
		return;
	}

	// Append a new property to the feature.
	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_conjugate_plate_id = 
			GPlatesPropertyValues::GpmlPlateId::create(conjugate_plate_id.get());
	// If 'verify_information_model' is true then property is only added if it doesn't not violate the GPGIM.
	GPlatesModel::ModelUtils::add_property(
			feature_ref,
			get_conjugate_plate_id_property_name(),
			gpml_conjugate_plate_id,
			verify_information_model/*check_property_name_allowed_for_feature_type*/);
}


boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type>
GPlatesAppLogic::PartitionFeatureUtils::get_valid_time_from_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> time_period =
			GPlatesModel::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
					feature_ref,
					get_valid_time_property_name());
	if (!time_period)
	{
		return boost::none;
	}

	return time_period.get();
}


void
GPlatesAppLogic::PartitionFeatureUtils::assign_valid_time_to_feature(
		boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> valid_time,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		bool verify_information_model)
{
	// First remove any that might already exist.
	feature_ref->remove_properties_by_name(get_valid_time_property_name());

	// Only assign a new time period if we've been given one.
	if (!valid_time)
	{
		return;
	}

	// Append a new property to the feature.
	// If 'verify_information_model' is true then property is only added if it doesn't not violate the GPGIM.
	GPlatesModel::ModelUtils::add_property(
			feature_ref,
			get_valid_time_property_name(),
			valid_time.get()->clone(),
			verify_information_model/*check_property_name_allowed_for_feature_type*/);
}


void
GPlatesAppLogic::PartitionFeatureUtils::append_geometry_domain_to_feature(
		const geometry_domain_type &geometry_domain,
		const GPlatesModel::PropertyName &geometry_domain_property_name,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	GPlatesModel::PropertyValue::non_null_ptr_type geometry_domain_property =
			GeometryUtils::create_geometry_property_value(geometry_domain);

	// Use 'ModelUtils::add_property()' instead of 'FeatureHandle::add()' to ensure any
	// necessary time-dependent wrapper is added.
	GPlatesModel::ModelUtils::add_property(
			feature_ref,
			geometry_domain_property_name,
			geometry_domain_property);
}


void
GPlatesAppLogic::PartitionFeatureUtils::append_geometry_range_to_feature(
		const geometry_range_type &geometry_range,
		const GPlatesModel::PropertyName &geometry_range_property_name,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	// Clone to get 'non-const' from 'const'.
	// Might also need cloning if cannot share child revisionable objects across parents?
	std::vector<GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_type> geometry_range_clone;

	geometry_range_type::const_iterator geometry_range_iter = geometry_range.begin();
	geometry_range_type::const_iterator geometry_range_end = geometry_range.end();
	for ( ; geometry_range_iter != geometry_range_end; ++geometry_range_iter)
	{
		geometry_range_clone.push_back((*geometry_range_iter)->clone());
	}

	// Use 'ModelUtils::add_property()' instead of 'FeatureHandle::add()' to ensure any
	// necessary time-dependent wrapper is added.
	GPlatesModel::ModelUtils::add_property(
			feature_ref,
			geometry_range_property_name,
			GPlatesPropertyValues::GmlDataBlock::create(
					geometry_range_clone.begin(),
					geometry_range_clone.end()));
}


GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeatureManager::PartitionedFeatureManager(
		const GPlatesModel::FeatureHandle::weak_ref &original_feature,
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
		const boost::shared_ptr<PropertyValueAssigner> &property_value_assigner) :
	d_original_feature(original_feature),
	d_has_original_feature_been_claimed(false),
	d_feature_to_clone_from(original_feature->clone()),
	d_feature_collection(feature_collection),
	d_property_value_assigner(property_value_assigner)
{
}


GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::PartitionFeatureUtils::PartitionedFeatureManager::get_feature_for_partition(
		const GPlatesModel::PropertyName &geometry_domain_property_name,
		bool geometry_domain_has_associated_range,
		boost::optional<const ReconstructionGeometry *> partition)
{
	//
	// Search for an existing feature we can re-use.
	//

	// First iterate over the existing features created for the current partition.
	feature_info_seq_type &feature_infos = d_partitioned_features[partition];
	feature_info_seq_type::iterator feature_infos_iter = feature_infos.begin();
	feature_info_seq_type::iterator feature_infos_end = feature_infos.end();
	for ( ; feature_infos_iter != feature_infos_end; ++feature_infos_iter)
	{
		FeatureInfo &feature_info = *feature_infos_iter;

		// See if the current feature has a geometry property with the same property name
		// that the caller will be adding to the feature.
		const std::pair<feature_contents_type::iterator, bool> feature_contents =
				feature_info.contents.insert(
						feature_contents_type::value_type(
								geometry_domain_property_name,
								geometry_domain_has_associated_range));

		// If the current feature does not have the geometry property name then the caller
		// can add the geometry property without any conflict (in the domain/range mapping).
		if (feature_contents.second/*inserted*/)
		{
			return feature_info.feature;
		}
		// ...else the current feature has a geometry domain property with the same property name.

		// If either:
		//   (1) the caller is adding a range (associated with domain), or
		//   (2) the current feature already has a range (associated with the domain)
		// ...then we should create a new feature to avoid the problem of two domains
		// (with the same property name) having the same number of points and hence there being an
		// ambiguous mapping to the range(s). Note that we could check to see if the number of points
		// are the same, but it's better to keep in separate features anyway since the user could
		// subsequently delete points in one domain leading to the same problem.
		if (!geometry_domain_has_associated_range &&
			!feature_contents.first->second)
		{
			// No prior or subsequent ranges (associated with domain) in the current feature.
			// So we can re-use it since can have any number of geometry domains with the same
			// property name in a single feature without any ambiguity.
			return feature_info.feature;
		}
	}

	// Create a new feature.
	const GPlatesModel::FeatureHandle::weak_ref new_feature = create_feature();

	// Assign property values (from 'partition's feature) to the new feature.
	assign_property_values(new_feature, partition);

	// Record new feature and its contents.
	feature_infos.push_back(FeatureInfo(new_feature));
	FeatureInfo &feature_info = feature_infos.back();
	feature_info.contents.insert(
			feature_contents_type::value_type(
					geometry_domain_property_name,
					geometry_domain_has_associated_range));
	
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

	return d_feature_to_clone_from->clone(d_feature_collection);
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
	d_area(0)
{
}


void
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::accumulate(
		const GPlatesMaths::GeometryOnSphere &geometry)
{
	GeometrySize geometry_size(d_num_points, d_arc_distance, d_area);
	geometry.accept_visitor(geometry_size);
}


void
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::accumulate(
		const GeometrySizeMetric &geometry_size_metric)
{
	d_num_points += geometry_size_metric.d_num_points;
	d_arc_distance += geometry_size_metric.d_arc_distance;
	d_area += geometry_size_metric.d_area;
}


bool
GPlatesAppLogic::PartitionFeatureUtils::GeometrySizeMetric::operator<(
		const GeometrySizeMetric &rhs) const
{
	// Area (polygons) first, then arc length (polylines), then the number of points: so a feature
	// with polygons is decided by them (section 5.4).
	//
	// The area and the arc length are real_t, whose '!=' is "differ by more than its epsilon", and
	// equality within a tolerance is not transitive: this is not a strict weak ordering. It serves
	// the running maximum of 'find_partition_containing_most_geometry()' and must not be given to
	// a sort.
	if (d_area != rhs.d_area)
	{
		return d_area < rhs.d_area;
	}
	if (d_arc_distance != rhs.d_arc_distance)
	{
		return d_arc_distance < rhs.d_arc_distance;
	}
	return d_num_points < rhs.d_num_points;
}
