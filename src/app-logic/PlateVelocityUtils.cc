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

#include <cmath>
#include <boost/foreach.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "AppLogicUtils.h"
#include "GeometryCookieCutter.h"
#include "GeometryUtils.h"
#include "MultiPointVectorField.h"
#include "PlateVelocityUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionTree.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"
#include "ResolvedTriangulationNetwork.h"
#include "ResolvedTriangulationUtils.h"
#include "TopologyUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/AngularExtent.h"
#include "maths/CalculateVelocity.h"
#include "maths/FiniteRotation.h"
#include "maths/Rotation.h"
#include "maths/SmallCircleBounds.h"

#include "model/FeatureType.h"
#include "model/FeatureVisitor.h"
#include "model/Model.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"
#include "model/types.h"

#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"

#include "utils/Profile.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Determines if any mesh node features that can be used by plate velocity calculations.
		 */
		class DetectVelocityMeshNodes:
				public GPlatesModel::ConstFeatureVisitor
		{
		public:
			DetectVelocityMeshNodes() :
				d_found_velocity_mesh_nodes(false)
			{  }


			bool
			has_velocity_mesh_node_features() const
			{
				return d_found_velocity_mesh_nodes;
			}


			virtual
			void
			visit_feature_handle(
					const GPlatesModel::FeatureHandle &feature_handle)
			{
				if (d_found_velocity_mesh_nodes)
				{
					// We've already found a mesh node feature so just return.
					// We're trying to see if any features in a feature collection have
					// a velocity mesh node.
					return;
				}

				static const GPlatesModel::FeatureType mesh_node_feature_type = 
						GPlatesModel::FeatureType::create_gpml("MeshNode");

				if (feature_handle.feature_type() == mesh_node_feature_type)
				{
					d_found_velocity_mesh_nodes = true;
				}

				// NOTE: We don't actually want to visit the feature's properties
				// so we're not calling 'visit_feature_properties()'.
			}

		private:
			bool d_found_velocity_mesh_nodes;
		};


		/**
		 * For each feature of type "gpml:MeshNode" creates a new feature
		 * of type "gpml:VelocityField" and adds to a new feature collection.
		 */
		class AddVelocityFieldFeatures:
				public GPlatesModel::ConstFeatureVisitor
		{
		public:
			AddVelocityFieldFeatures(
					const GPlatesModel::FeatureCollectionHandle::weak_ref &velocity_field_feature_collection) :
				d_velocity_field_feature_collection(velocity_field_feature_collection)
			{  }


			virtual
			bool
			initialise_pre_feature_properties(
					const GPlatesModel::FeatureHandle &feature_handle)
			{
				static const GPlatesModel::FeatureType mesh_node_feature_type = 
						GPlatesModel::FeatureType::create_gpml("MeshNode");

				if (feature_handle.feature_type() != mesh_node_feature_type)
				{
					// Don't visit this feature.
					return false;
				}

				d_velocity_field_feature = create_velocity_field_feature();

				return true;
			}


			virtual
			void
			visit_gml_multi_point(
					const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
			{
				static const GPlatesModel::PropertyName mesh_points_prop_name =
						GPlatesModel::PropertyName::create_gpml("meshPoints");

				// Note: we can't get here without a valid property name but check
				// anyway in case visiting a property value directly (ie, not via a feature).
				if (current_top_level_propname() &&
					*current_top_level_propname() == mesh_points_prop_name)
				{
					// We only expect one "meshPoints" property per mesh node feature.
					// If there are multiple then we'll create multiple "domainSet" properties
					// and velocity solver will aggregate them all into a single "rangeSet"
					// property. Which means we'll have one large "rangeSet" property mapping
					// into multiple smaller "domainSet" properties and the mapping order
					// will be implementation defined.
					create_and_append_domain_set_property_to_velocity_field_feature(
							gml_multi_point);
				}
			}

		private:
			GPlatesModel::FeatureCollectionHandle::weak_ref d_velocity_field_feature_collection;
			GPlatesModel::FeatureHandle::weak_ref d_velocity_field_feature;


			const GPlatesModel::FeatureHandle::weak_ref
			create_velocity_field_feature()
			{
				static const GPlatesModel::FeatureType velocity_field_feature_type = 
						GPlatesModel::FeatureType::create_gpml("VelocityField");

				// Create a new velocity field feature adding it to the new collection.
				return GPlatesModel::FeatureHandle::create(
						d_velocity_field_feature_collection,
						velocity_field_feature_type);
			}


			void
			create_and_append_domain_set_property_to_velocity_field_feature(
					const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
			{
				//
				// Create the "gml:domainSet" property of type GmlMultiPoint -
				// basically references "meshPoints" property in mesh node feature which
				// should be a GmlMultiPoint.
				//
				static const GPlatesModel::PropertyName domain_set_prop_name =
						GPlatesModel::PropertyName::create_gml("domainSet");

				GPlatesPropertyValues::GmlMultiPoint::non_null_ptr_type domain_set_gml_multi_point =
						GPlatesPropertyValues::GmlMultiPoint::create(
								gml_multi_point.get_multipoint());

				d_velocity_field_feature->add(
						GPlatesModel::TopLevelPropertyInline::create(
							domain_set_prop_name,
							domain_set_gml_multi_point));
			}
		};


		/**
		 * Test the domain point against the resolved topological network.
		 *
		 * Return false if point is not inside the network.
		 */
		bool
		solve_velocities_on_networks(
				const GPlatesMaths::PointOnSphere &domain_point,
				boost::optional<MultiPointVectorField::CodomainElement> &range_element,
				const PlateVelocityUtils::TopologicalNetworksVelocities &resolved_networks_query,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type)
		{
			boost::optional< std::pair<const ReconstructionGeometry *, GPlatesMaths::Vector3D > >
					network_velocity = resolved_networks_query.calculate_velocity(
							domain_point,
							velocity_delta_time,
							velocity_delta_time_type);
			if (!network_velocity)
			{
				return false;
			}

			// The network 'component' could be the network's deforming region or an interior
			// rigid block in the network.
			const ReconstructionGeometry *network_component = network_velocity->first;
			const GPlatesMaths::Vector3D &velocity_vector = network_velocity->second;

			// Determine if point was in the deforming region or interior rigid block of network.
			const MultiPointVectorField::CodomainElement::Reason codomain_element_reason =
					// It's either a resolved topological network or a reconstructed feature geometry...
					ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							const ResolvedTopologicalNetwork *>(network_component)
					? MultiPointVectorField::CodomainElement::InNetworkDeformingRegion
					: MultiPointVectorField::CodomainElement::InNetworkRigidBlock;

			// Note that we output the plate id of the deforming network (or one of its rigid blocks).
			// This keeps things consistent with the plate id assignment code which assigns both
			// static/dynamic polygon plate ids *and* network plate ids.
			// This also means the GMT velocity export (which also exports plate ids) will export non-zero
			// plate ids for all velocity domain points (assuming global coverage of plates/networks).
			range_element = MultiPointVectorField::CodomainElement(
					velocity_vector,
					codomain_element_reason,
					// The network component's plate id...
					ReconstructionGeometryUtils::get_plate_id(network_component),
					network_component);

			return true;
		}


		/**
		 * Test the domain point against rigid plates (resolved topological boundaries and static polygons).
		 *
		 * Return false if point is not inside any rigid plates.
		 */
		bool
		solve_velocities_on_rigid_plates(
				const GPlatesMaths::PointOnSphere &domain_point,
				boost::optional<MultiPointVectorField::CodomainElement> &range_element,
				const GeometryCookieCutter &rigid_plates_query,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type)
		{
			const boost::optional<const ReconstructionGeometry *> rigid_plate_containing_point =
					rigid_plates_query.partition_point(domain_point);
			if (!rigid_plate_containing_point)
			{
				return false;
			}


#ifdef DEBUG
GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(domain_point);
qDebug() << "solve_velocities_on_rigid_plates: " << llp;
#endif

			const boost::optional<GPlatesModel::integer_plate_id_type> recon_plate_id_opt =
					ReconstructionGeometryUtils::get_plate_id(
							rigid_plate_containing_point.get());
			if (!recon_plate_id_opt)
			{
				GPlatesMaths::Vector3D zero_velocity(0, 0, 0);
				range_element = MultiPointVectorField::CodomainElement(
						zero_velocity,
						MultiPointVectorField::CodomainElement::NotInAnyBoundaryOrNetwork);

				return true;
			}

			GPlatesModel::integer_plate_id_type recon_plate_id = recon_plate_id_opt.get();

			const double reconstruction_time = rigid_plate_containing_point.get()->get_reconstruction_time();

			// Get the reconstruction tree creator to calculate velocity with.
			boost::optional<ReconstructionTreeCreator> recon_tree_creator =
				ReconstructionGeometryUtils::get_reconstruction_tree_creator(
						rigid_plate_containing_point.get());
			// This should succeed since resolved topological boundaries and RFGs (static polygons)
			// support reconstruction trees.
			if (!recon_tree_creator)
			{
				GPlatesMaths::Vector3D zero_velocity(0, 0, 0);
				range_element = MultiPointVectorField::CodomainElement(
						zero_velocity,
						MultiPointVectorField::CodomainElement::NotInAnyBoundaryOrNetwork);

				return true;
			}

			// Compute the velocity for this domain point.
			const GPlatesMaths::Vector3D vector_xyz =
					PlateVelocityUtils::calculate_velocity_vector(
							domain_point,
							recon_plate_id,
							recon_tree_creator.get(),
							reconstruction_time,
							velocity_delta_time,
							velocity_delta_time_type);

			// Determine if point was in a resolved topological boundary or RFG (static polygon).
			const MultiPointVectorField::CodomainElement::Reason codomain_element_reason =
					ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							const ResolvedTopologicalBoundary *>(rigid_plate_containing_point.get())
					? MultiPointVectorField::CodomainElement::InPlateBoundary
					: MultiPointVectorField::CodomainElement::InStaticPolygon;

			range_element = MultiPointVectorField::CodomainElement(
					vector_xyz,
					codomain_element_reason,
					recon_plate_id,
					rigid_plate_containing_point.get());

			return true;
		}


		/**
		 * Test the domain point against the all surface types.
		 *
		 * Return false if point is not inside any surfaces.
		 */
		bool
		solve_velocity_on_surfaces(
				const GPlatesMaths::PointOnSphere &domain_point,
				boost::optional<MultiPointVectorField::CodomainElement> &range_element,
				const GeometryCookieCutter &rigid_plates_query,
				const PlateVelocityUtils::TopologicalNetworksVelocities &resolved_networks_query,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type)
		{
			//
			// First check whether domain point is inside any topological networks.
			// This includes points inside interior rigid blocks on the networks.
			//
			if (solve_velocities_on_networks(
					domain_point,
					range_element,
					resolved_networks_query,
					velocity_delta_time,
					velocity_delta_time_type))
			{
				return true;
			}

			//
			// Next see if point is inside any topological boundaries or static (reconstructed) polygons.
			//

			if (solve_velocities_on_rigid_plates(
					domain_point,
					range_element,
					rigid_plates_query,
					velocity_delta_time,
					velocity_delta_time_type))
			{
				return true;
			}

			// else, point not found in any topology or static polygons so set the velocity values to 0 

			const GPlatesMaths::Vector3D zero_velocity(0, 0, 0);
			const MultiPointVectorField::CodomainElement::Reason reason =
					MultiPointVectorField::CodomainElement::NotInAnyBoundaryOrNetwork;
			range_element = MultiPointVectorField::CodomainElement(zero_velocity, reason);

#ifdef DEBUG
			// Report a warning.
			// This is useful for global models that should span the entire globes with no gaps.
			const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(domain_point);
			qDebug() << "WARNING: domain point not in any Topology! point = " << llp;
#endif

			return false;
		}


		/**
		 * Calculate the boundary velocity at the specified point sample (which is very close
		 * to the boundary but not on it).
		 *
		 * It will be either inside the polygon or outside (in an adjacent polygon).
		 */
		void
		solve_velocity_at_boundary(
				const GPlatesMaths::PointOnSphere &point_sample,
				boost::optional<GPlatesMaths::Vector3D> &velocity_inside_polygon_boundary,
				boost::optional<GPlatesMaths::Vector3D> &velocity_outside_polygon_boundary,
				const ReconstructionGeometry *polygon_recon_geom_containing_domain_point,
				const GeometryCookieCutter &rigid_plates_query,
				const PlateVelocityUtils::TopologicalNetworksVelocities &resolved_networks_query,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type)
		{
			// Sample the velocity at the point sample.
			boost::optional<MultiPointVectorField::CodomainElement> velocity_sample;
			if (!solve_velocity_on_surfaces(
					point_sample,
					velocity_sample,
					rigid_plates_query,
					resolved_networks_query,
					velocity_delta_time,
					velocity_delta_time_type))
			{
				// Didn't sample a surface.
				return;
			}

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					velocity_sample,
					GPLATES_ASSERTION_SOURCE);

			// If the surface was found (ie, not a zero velocity vector) then assign the
			// velocity to the inside or outside polygon depending on the surface.
			if (velocity_sample->d_plate_id_reconstruction_geometry)
			{
				if (velocity_sample->d_plate_id_reconstruction_geometry.get() ==
					polygon_recon_geom_containing_domain_point)
				{
					// If not already found a velocity inside polygon...
					if (!velocity_inside_polygon_boundary)
					{
						velocity_inside_polygon_boundary = velocity_sample->d_vector;
					}
				}
				else // velocity sample is from the adjacent polygon(s)...
				{
					// If not already found a velocity outside polygon...
					if (!velocity_outside_polygon_boundary)
					{
						velocity_outside_polygon_boundary = velocity_sample->d_vector;
					}
				}
			}
		}


		/**
		 * Calculate the average velocity, at the specified point on the polygon boundary,
		 * by averaging the velocity on each side of the boundary.
		 */
		boost::optional<GPlatesMaths::Vector3D>
		solve_average_velocity_at_boundary(
				const GPlatesMaths::PointOnSphere &polygon_boundary_point,
				const GPlatesMaths::PointOnSphere &domain_point,
				const ReconstructionGeometry *polygon_recon_geom_containing_domain_point,
				const GeometryCookieCutter &rigid_plates_query,
				const PlateVelocityUtils::TopologicalNetworksVelocities &resolved_networks_query,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type)
		{
			// We need both a velocity just inside and just outside the polygon boundary before
			// we can calculate the average velocity at boundary.
			boost::optional<GPlatesMaths::Vector3D> velocity_inside_polygon_boundary;
			boost::optional<GPlatesMaths::Vector3D> velocity_outside_polygon_boundary;

			// Rotation axis to rotate domain point towards polygon boundary point.
			GPlatesMaths::Vector3D domain_to_boundary_rotation_axis_cross_product = cross(
					GPlatesMaths::Vector3D(domain_point.position_vector()),
					GPlatesMaths::Vector3D(polygon_boundary_point.position_vector()));
			const GPlatesMaths::UnitVector3D domain_to_boundary_rotation_axis =
					(domain_to_boundary_rotation_axis_cross_product.magSqrd() > 0.0)
					? domain_to_boundary_rotation_axis_cross_product.get_normalisation()
					// The domain point and the polygon boundary point happen to coincide.
					// Pick an arbitrary rotation axis orthogonal to the polygon boundary point...
					: GPlatesMaths::generate_perpendicular(polygon_boundary_point.position_vector());

			// This should be small but not too small that we start getting epsilon issues when
			// testing the epsilon-rotated points for inclusion in polygons sharing boundary.
			const double epsilon_rotation_angle = 1e-3/*radians, ~0.1 degrees or 10Kms*/;

			//
			// Try sampling directly outside and inside (separated by 180 degrees).
			//

			// Start with the point just inside the adjacent polygon (it might not be though).
			// In most cases this will give us the best chance of finding the velocity in the adjacent polygon.
			const GPlatesMaths::PointOnSphere original_point_outside_polygon_boundary =
					GPlatesMaths::Rotation::create(domain_to_boundary_rotation_axis, epsilon_rotation_angle) *
							polygon_boundary_point;

			// Sample directly outside the polygon.
			solve_velocity_at_boundary(
					original_point_outside_polygon_boundary,
					velocity_inside_polygon_boundary,
					velocity_outside_polygon_boundary,
					polygon_recon_geom_containing_domain_point,
					rigid_plates_query,
					resolved_networks_query,
					velocity_delta_time,
					velocity_delta_time_type);

			// To get point sample inside polygon boundary we rotate the point outside by 180 degrees.
			GPlatesMaths::Rotation rotation_180_about_polygon_boundary_point =
					GPlatesMaths::Rotation::create(polygon_boundary_point.position_vector(), GPlatesMaths::PI);

			// Sample directly inside the polygon.
			solve_velocity_at_boundary(
					rotation_180_about_polygon_boundary_point * original_point_outside_polygon_boundary,
					velocity_inside_polygon_boundary,
					velocity_outside_polygon_boundary,
					polygon_recon_geom_containing_domain_point,
					rigid_plates_query,
					resolved_networks_query,
					velocity_delta_time,
					velocity_delta_time_type);

			if (velocity_inside_polygon_boundary && velocity_outside_polygon_boundary)
			{
				// Return the average velocity at boundary.
				return 0.5 * (velocity_inside_polygon_boundary.get() + velocity_outside_polygon_boundary.get());
			}

			//
			// Try sampling directly left and right along boundary (separated by 180 degrees).
			//

			GPlatesMaths::Rotation rotation_90_about_polygon_boundary_point =
					GPlatesMaths::Rotation::create(polygon_boundary_point.position_vector(), GPlatesMaths::HALF_PI);

			// Start with the point just inside the adjacent polygon (it might not be though).
			// In most cases this will give us the best chance of finding the velocity in the adjacent polygon.
			const GPlatesMaths::PointOnSphere point_left_along_polygon_boundary =
					rotation_90_about_polygon_boundary_point * original_point_outside_polygon_boundary;

			// Sample left along polygon boundary.
			solve_velocity_at_boundary(
					point_left_along_polygon_boundary,
					velocity_inside_polygon_boundary,
					velocity_outside_polygon_boundary,
					polygon_recon_geom_containing_domain_point,
					rigid_plates_query,
					resolved_networks_query,
					velocity_delta_time,
					velocity_delta_time_type);

			if (velocity_inside_polygon_boundary && velocity_outside_polygon_boundary)
			{
				// Return the average velocity at boundary.
				return 0.5 * (velocity_inside_polygon_boundary.get() + velocity_outside_polygon_boundary.get());
			}

			// Sample right along polygon boundary.
			solve_velocity_at_boundary(
					rotation_180_about_polygon_boundary_point * point_left_along_polygon_boundary,
					velocity_inside_polygon_boundary,
					velocity_outside_polygon_boundary,
					polygon_recon_geom_containing_domain_point,
					rigid_plates_query,
					resolved_networks_query,
					velocity_delta_time,
					velocity_delta_time_type);

			if (velocity_inside_polygon_boundary && velocity_outside_polygon_boundary)
			{
				// Return the average velocity at boundary.
				return 0.5 * (velocity_inside_polygon_boundary.get() + velocity_outside_polygon_boundary.get());
			}

			//
			// Try sampling at regular intervals around a circle (around the polygon boundary point).
			// With each depth level halving the previous level's interval.
			//

			for (unsigned int level = 2; level < 5; ++level)
			{
				const unsigned int num_intervals = (1 << level);
				const double interval_rotation_angle = 2 * GPlatesMaths::PI / num_intervals;

				// The starting point sample is offset by 'half' an interval in order to avoid
				// sampling the same points as the previous level.
				GPlatesMaths::PointOnSphere point_sample =
						GPlatesMaths::Rotation::create(
								polygon_boundary_point.position_vector(),
								0.5 * interval_rotation_angle) *
										original_point_outside_polygon_boundary;

				GPlatesMaths::Rotation interval_rotation_about_polygon_boundary_point =
						GPlatesMaths::Rotation::create(
								polygon_boundary_point.position_vector(),
								interval_rotation_angle);

				for (unsigned int interval = 0; interval < num_intervals; ++interval)
				{
					solve_velocity_at_boundary(
							point_sample,
							velocity_inside_polygon_boundary,
							velocity_outside_polygon_boundary,
							polygon_recon_geom_containing_domain_point,
							rigid_plates_query,
							resolved_networks_query,
							velocity_delta_time,
							velocity_delta_time_type);

					if (velocity_inside_polygon_boundary && velocity_outside_polygon_boundary)
					{
						// Return the average velocity at boundary.
						return 0.5 * (velocity_inside_polygon_boundary.get() + velocity_outside_polygon_boundary.get());
					}

					// Move to the next sample point.
					point_sample = interval_rotation_about_polygon_boundary_point * point_sample;
				}
			}

			// We were unable to find a velocity both inside and outside the polygon boundary.

			// If unable to find a velocity inside the polygon boundary then just resort to
			// using the velocity at the domain point even though it could be further from
			// the boundary point (although still within the smoothing distance).
			// The velocities within the same polygon (and relatively near each other) should
			// be relatively the same.
			// This scenario happens when there are overlapping polygons and the domain point
			// is outside the overlapping region (then the boundary point lies in the overlapping
			// region and the plate polygon of the domain point cannot be found at the boundary point).
			if (!velocity_inside_polygon_boundary)
			{
				solve_velocity_at_boundary(
						domain_point,
						velocity_inside_polygon_boundary,
						velocity_outside_polygon_boundary,
						polygon_recon_geom_containing_domain_point,
						rigid_plates_query,
						resolved_networks_query,
						velocity_delta_time,
						velocity_delta_time_type);

				if (velocity_inside_polygon_boundary && velocity_outside_polygon_boundary)
				{
					// Return the average velocity at boundary.
					return 0.5 * (velocity_inside_polygon_boundary.get() + velocity_outside_polygon_boundary.get());
				}
			}

			// We were unable to find a velocity both inside and outside the polygon boundary.
			qWarning() << "Unable to find average velocity at plate boundary for smoothing:";
			qWarning() << "  Most likely cause is a gap between plates/surfaces.";
			GPlatesMaths::LatLonPoint domain_point_lat_lon =
					GPlatesMaths::make_lat_lon_point(domain_point);
			qWarning() << "  Domain point location lat/lon: "
					<< domain_point_lat_lon.latitude() << ", "
					<< domain_point_lat_lon.longitude();
			GPlatesMaths::LatLonPoint polygon_boundary_point_lat_lon =
					GPlatesMaths::make_lat_lon_point(polygon_boundary_point);
			qWarning() << "  Nearest plate boundary location lat/lon: "
					<< polygon_boundary_point_lat_lon.latitude() << ", "
					<< polygon_boundary_point_lat_lon.longitude() << "\n";

			return boost::none;
		}


		/**
		 * Test the domain point against the all surface types and smooth velocities near boundaries.
		 *
		 * Return false if point is not inside any surfaces.
		 */
		bool
		solve_velocity_on_surfaces_with_boundary_smoothing(
				const GPlatesMaths::PointOnSphere &domain_point,
				boost::optional<MultiPointVectorField::CodomainElement> &range_element,
				const GeometryCookieCutter &rigid_plates_query,
				const PlateVelocityUtils::TopologicalNetworksVelocities &resolved_networks_query,
				const double &velocity_delta_time,
				VelocityDeltaTime::Type velocity_delta_time_type,
				const double &boundary_smoothing_half_angle_radians,
				const GPlatesMaths::AngularExtent &boundary_smoothing_angular_half_extent,
				bool exclude_deforming_regions_from_smoothing)
		{
			// First solve the velocity at the domain point.
			if (!solve_velocity_on_surfaces(
					domain_point,
					range_element,
					rigid_plates_query,
					resolved_networks_query,
					velocity_delta_time,
					velocity_delta_time_type))
			{
				// Domain point is not inside any surfaces.
				return false;
			}

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					range_element,
					GPLATES_ASSERTION_SOURCE);

			if (exclude_deforming_regions_from_smoothing)
			{
				// If the domain point is inside a deforming region (or micro-block within a deforming region)
				// then it does not need smoothing so just use the already calculated velocity at the domain point.
				// One reason smoothing might not be needed is because a deforming region linearly interpolates
				// velocities and hence domain points near the deforming region boundary will have velocities
				// similar to those at the boundary, hence smoothing has essentially already been done.
				if (range_element->d_reason == MultiPointVectorField::CodomainElement::InNetworkDeformingRegion ||
					range_element->d_reason == MultiPointVectorField::CodomainElement::InNetworkRigidBlock)
				{
					return true;
				}
			}

			// If we don't have a reconstruction geometry then the point was inside a boundary but
			// we couldn't find the plate id of the boundary (and hence reverted to zero velocity).
			if (!range_element->d_plate_id_reconstruction_geometry)
			{
				return true;
			}

			const ReconstructionGeometry *boundary_recon_geom =
					range_element->d_plate_id_reconstruction_geometry.get();

			// Get the boundary polygon geometry.
			boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> polygon_boundary_opt =
					ReconstructionGeometryUtils::get_boundary_polygon(boundary_recon_geom);
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					polygon_boundary_opt,
					GPLATES_ASSERTION_SOURCE);
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_boundary =
					polygon_boundary_opt.get();

			// An optimisation that avoids testing all domain points for closeness to the polygon
			// boundary is to use a small circle test. Any domain point inside this small circle
			// cannot be in the smoothing region.
			const GPlatesMaths::BoundingSmallCircle polygon_boundary_inner_bounding_small_circle =
					polygon_boundary->get_inner_outer_bounding_small_circle()
							.get_inner_bounding_small_circle();

			// Interior of small circle is outside smoothing region.
			// The small circle is the inner small circle boundary of the polygon shrunk by
			// the smoothing distance. 
			const GPlatesMaths::BoundingSmallCircle outside_smoothing_region_small_circle =
					polygon_boundary_inner_bounding_small_circle.contract(
							boundary_smoothing_angular_half_extent);

			// Here OUTSIDE_BOUNDS means outside the small circle.
			if (outside_smoothing_region_small_circle.test(domain_point) !=
				GPlatesMaths::BoundingSmallCircle::OUTSIDE_BOUNDS)
			{
				// The domain point is not in the smoothing region so just use the
				// already calculated velocity at the domain point.
				return true;
			}

			// Find the closest point on the boundary polygon (to the domain point) that is
			// within the smoothing extents.
			GPlatesMaths::real_t closeness_to_polygon_boundary = -1/*least close*/;
			boost::optional<GPlatesMaths::PointOnSphere> closest_point_on_polygon_boundary =
					polygon_boundary->is_close_to(
							domain_point,
							boundary_smoothing_angular_half_extent,
							closeness_to_polygon_boundary);
			if (!closest_point_on_polygon_boundary)
			{
				// Domain point is not within the smoothing extent region-of-interest surrounding
				// the polygon so just use the already calculated velocity at the domain point.
				return true;
			}

			boost::optional<GPlatesMaths::Vector3D> average_boundary_velocity =
					solve_average_velocity_at_boundary(
							closest_point_on_polygon_boundary.get(),
							domain_point,
							boundary_recon_geom,
							rigid_plates_query,
							resolved_networks_query,
							velocity_delta_time,
							velocity_delta_time_type);
			if (!average_boundary_velocity)
			{
				// Unable to calculate average since unable to sample adjacent polygon -
				// most likely due to complicated local concavities in polygon boundary.
				// For now just return the un-smoothed velocity already calculated.
				return true;
			}

			// The smoothing (interpolation) factor.
			const GPlatesMaths::real_t smoothing_factor =
					acos(closeness_to_polygon_boundary) / boundary_smoothing_half_angle_radians;

			// Smooth the already calculated velocity vector.
			range_element->d_vector =
					smoothing_factor * range_element->d_vector +
					(1 - smoothing_factor) * average_boundary_velocity.get();

			return true;
		}
	}
}


bool
GPlatesAppLogic::PlateVelocityUtils::detect_velocity_mesh_nodes(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	if (!feature_collection.is_valid())
	{
		return false;
	}

	// Visitor to detect mesh node features in the feature collection.
	DetectVelocityMeshNodes detect_velocity_mesh_nodes;

	AppLogicUtils::visit_feature_collection(feature_collection, detect_velocity_mesh_nodes);

	return detect_velocity_mesh_nodes.has_velocity_mesh_node_features();
}


bool
GPlatesAppLogic::PlateVelocityUtils::detect_velocity_mesh_node(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	if (!feature_ref.is_valid())
	{
		return false;
	}

	// Visitor to detect a mesh node feature.
	DetectVelocityMeshNodes detect_velocity_mesh_nodes;

	detect_velocity_mesh_nodes.visit_feature(feature_ref);

	return detect_velocity_mesh_nodes.has_velocity_mesh_node_features();
}


GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
GPlatesAppLogic::PlateVelocityUtils::create_velocity_field_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_with_mesh_nodes)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			feature_collection_with_mesh_nodes.is_valid(),
			GPLATES_ASSERTION_SOURCE);

	// Create a new feature collection to store our velocity field features.
	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type velocity_field_feature_collection =
			GPlatesModel::FeatureCollectionHandle::create();

	// Create a reference to the new feature collection as that's needed to add features to it.
	GPlatesModel::FeatureCollectionHandle::weak_ref velocity_field_feature_collection_ref =
			velocity_field_feature_collection->reference();

	// A visitor to look for mesh node features in the original feature collection
	// and create corresponding velocity field features in the new feature collection.
	AddVelocityFieldFeatures add_velocity_field_features(velocity_field_feature_collection_ref);

	AppLogicUtils::visit_feature_collection(feature_collection_with_mesh_nodes, add_velocity_field_features);

	// Return the newly created feature collection.
	return velocity_field_feature_collection;
}


void
GPlatesAppLogic::PlateVelocityUtils::solve_velocities_on_surfaces(
		std::vector<MultiPointVectorField::non_null_ptr_type> &multi_point_velocity_fields,
		const double &reconstruction_time,
		const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &velocity_domains,
		const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &velocity_surface_reconstructed_static_polygons,
		const std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &velocity_surface_resolved_topological_boundaries,
		const std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &velocity_surface_resolved_topological_networks,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type,
		const boost::optional<VelocitySmoothingOptions> &velocity_smoothing_options)
{
	PROFILE_FUNC();

	// Return early if there are no velocity domains on which to solve velocities.
	if (velocity_domains.empty())
	{
		return;
	}

	// Get the rigid plate features (resolved topological boundaries and static polygons) and wrap
	// them in a structure that can do point-in-polygon tests so we can query them at domain points.
	GeometryCookieCutter rigid_plates_query(
			reconstruction_time,
			velocity_surface_reconstructed_static_polygons,
			velocity_surface_resolved_topological_boundaries,
			boost::none/*velocity_surface_resolved_topological_networks*/,
			GeometryCookieCutter::SORT_BY_PLATE_ID,
			// Use high speed point-in-poly testing since very dense velocity meshes containing
			// lots of points can go through this path...
			GPlatesMaths::PolygonOnSphere::HIGH_SPEED_HIGH_SETUP_HIGH_MEMORY_USAGE);

	// Get the resolved topological networks so we can query them for interpolated velocity at domain points.
	const TopologicalNetworksVelocities resolved_networks_query(
			velocity_surface_resolved_topological_networks);

	GPlatesMaths::AngularExtent boundary_smoothing_angular_half_extent = GPlatesMaths::AngularExtent::ZERO;
	bool exclude_deforming_regions_from_smoothing = true;
	if (velocity_smoothing_options)
	{
		boundary_smoothing_angular_half_extent =
				GPlatesMaths::AngularExtent::create_from_angle(
						velocity_smoothing_options->angular_half_extent_radians);
		exclude_deforming_regions_from_smoothing = velocity_smoothing_options->exclude_deforming_regions;
	}

	// Iterate over the velocity domain RFGs.
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>::const_iterator velocity_domains_iter =
			velocity_domains.begin();
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>::const_iterator velocity_domains_end =
			velocity_domains.end();
	for ( ; velocity_domains_iter != velocity_domains_end; ++velocity_domains_iter)
	{
		const ReconstructedFeatureGeometry::non_null_ptr_type velocity_domain_rfg =
				*velocity_domains_iter;

		// NOTE: This is slightly dodgy because we will end up creating a MultiPointVectorField
		// that stores a multi-point domain and a corresponding velocity field but the
		// geometry property iterator (referenced by the MultiPointVectorField) could be a
		// non-multi-point geometry.
		//
		// Note that we used the *reconstructed* geometry for the velocity domain.
		// Previously we used the *present-day* geometry - essentially hard-wiring the plate id to zero
		// (regardless of the actual plate id) in order to fix it to the spin axis before surface testing.
		// Although even this had problems for non-zero anchor plate ids since zero plate id rotations
		// then became non-identity rotations and we just assumed an identity rotation always.
		// Also now that we use the *reconstructed* geometry this is no longer a problem for
		// zero plate ids or non-zero plate ids (with zero or non-zero anchor plate ids).
		// However by not forcing a zero plate id this allows the user to move the positions of
		// the points before surface testing. Not sure how useful this is - but it shouldn't
		// interfere with CitcomS mesh nodes (cap files) because they always have plate ids of zero
		// and hence don't move (unless the anchor plate id is non-zero as expected).
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain_multi_point =
				GeometryUtils::convert_geometry_to_multi_point(
						*velocity_domain_rfg->reconstructed_geometry());

		GPlatesMaths::MultiPointOnSphere::const_iterator domain_iter = velocity_domain_multi_point->begin();
		GPlatesMaths::MultiPointOnSphere::const_iterator domain_end = velocity_domain_multi_point->end();

		MultiPointVectorField::non_null_ptr_type vector_field =
				MultiPointVectorField::create_empty(
						reconstruction_time,
						velocity_domain_multi_point,
						// FIXME: Should this be the feature handle of the domain or surface ?
						// For now using the domain...
						*velocity_domain_rfg->property().handle_weak_ref(),
						velocity_domain_rfg->property());
		MultiPointVectorField::codomain_type::iterator field_iter = vector_field->begin();

		// Iterate over the domain points and calculate their velocities.
		for ( ; domain_iter != domain_end; ++domain_iter, ++field_iter)
		{
			const GPlatesMaths::PointOnSphere &domain_point = *domain_iter;
			boost::optional<MultiPointVectorField::CodomainElement> &range_element = *field_iter;

			if (velocity_smoothing_options)
			{
				solve_velocity_on_surfaces_with_boundary_smoothing(
						domain_point,
						range_element,
						rigid_plates_query,
						resolved_networks_query,
						velocity_delta_time,
						velocity_delta_time_type,
						velocity_smoothing_options->angular_half_extent_radians,
						boundary_smoothing_angular_half_extent,
						exclude_deforming_regions_from_smoothing);
			}
			else
			{
				solve_velocity_on_surfaces(
						domain_point,
						range_element,
						rigid_plates_query,
						resolved_networks_query,
						velocity_delta_time,
						velocity_delta_time_type);
			}
		}

		multi_point_velocity_fields.push_back(vector_field);
	}
}


GPlatesMaths::Vector3D
GPlatesAppLogic::PlateVelocityUtils::calculate_velocity_vector(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesModel::integer_plate_id_type &reconstruction_plate_id,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type)
{
	const GPlatesMaths::FiniteRotation stage_rotation = calculate_stage_rotation(
			reconstruction_plate_id,
			reconstruction_tree_creator,
			reconstruction_time,
			velocity_delta_time,
			velocity_delta_time_type);

	return GPlatesMaths::calculate_velocity_vector(point, stage_rotation, velocity_delta_time);
}


GPlatesMaths::FiniteRotation
GPlatesAppLogic::PlateVelocityUtils::calculate_stage_rotation(
		const GPlatesModel::integer_plate_id_type &reconstruction_plate_id,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type)
{
	const std::pair<double, double> time_range = VelocityDeltaTime::get_time_range(
			velocity_delta_time_type, reconstruction_time, velocity_delta_time);

	// Get the finite rotation results for the plate id.
	boost::optional<GPlatesMaths::FiniteRotation> fr_young =
			reconstruction_tree_creator.get_reconstruction_tree(time_range.second/*young*/)
					->get_composed_absolute_rotation(reconstruction_plate_id);
	boost::optional<GPlatesMaths::FiniteRotation> fr_old =
			reconstruction_tree_creator.get_reconstruction_tree(time_range.first/*old*/)
					->get_composed_absolute_rotation(reconstruction_plate_id);

	// If both times found then calculate velocity as normal.
	if (fr_young && fr_old)
	{
		// Calculate the stage rotation.
		return GPlatesMaths::calculate_stage_rotation(fr_young.get(), fr_old.get());
	}

	// If the youngest time in the delta time interval is negative *and* the oldest time
	// is non-negative *and* the oldest time found a plate ID match.
	// This happens when the reconstruction time is non-negative but happens samples a negative time
	// when calculating the velocity - if only the negative time matches no plate ID then we will
	// shift the delta time interval to (velocity_delta_time, 0) and try again.
	// This enables rare users to support negative (future) times in rotation files if they wish
	// but also supports most users having only non-negative rotations yet still supplying a valid
	// velocity at/near present day when using a delta time interval such as (T-dt, T) instead of (T+dt, T).
	if (!fr_young &&
		fr_old &&
		time_range.second/*young*/ < 0 &&
		time_range.first/*old*/ >= 0)
	{
		// Shift velocity calculation such that the time interval [velocity_delta_time, 0] is non-negative.
		boost::optional<GPlatesMaths::FiniteRotation> fr_zero =
				reconstruction_tree_creator.get_reconstruction_tree(0)
						->get_composed_absolute_rotation(reconstruction_plate_id);
		boost::optional<GPlatesMaths::FiniteRotation> fr_delta =
				reconstruction_tree_creator.get_reconstruction_tree(velocity_delta_time)
						->get_composed_absolute_rotation(reconstruction_plate_id);

		// If both times found then calculate velocity.
		if (fr_zero && fr_delta)
		{
			// Calculate the stage rotation.
			return GPlatesMaths::calculate_stage_rotation(fr_zero.get(), fr_delta.get());
		}
	}

	// A valid finite rotation might not be defined for times older than 'reconstruction_time' since
	// a feature might not exist at that time and hence the rotation file may not include that time
	// in its rotation sequence (for the plate ID).
	//
	// If not then we will try a time range of [reconstruction_time, reconstruction_time - velocity_delta_time].
	if (velocity_delta_time_type != VelocityDeltaTime::T_TO_T_MINUS_DELTA_T &&
		fr_young &&
		!fr_old)
	{
		// Next try shifting the velocity calculation such that the time interval is now
		// [reconstruction_time, reconstruction_time - velocity_delta_time].
		const std::pair<double, double> new_time_range = VelocityDeltaTime::get_time_range(
				VelocityDeltaTime::T_TO_T_MINUS_DELTA_T, reconstruction_time, velocity_delta_time);

		boost::optional<GPlatesMaths::FiniteRotation> fr_new_young =
				reconstruction_tree_creator.get_reconstruction_tree(new_time_range.second/*young*/)
						->get_composed_absolute_rotation(reconstruction_plate_id);
		boost::optional<GPlatesMaths::FiniteRotation> fr_new_old =
				reconstruction_tree_creator.get_reconstruction_tree(new_time_range.first/*old*/)
						->get_composed_absolute_rotation(reconstruction_plate_id);

		// If both times found then calculate velocity.
		if (fr_new_young && fr_new_old)
		{
			// Calculate the stage rotation.
			return GPlatesMaths::calculate_stage_rotation(fr_new_young.get(), fr_new_old.get());
		}
	}

	// Unable to calculate stage rotation - return identity rotation.
	return GPlatesMaths::FiniteRotation::create_identity_rotation();
}


GPlatesMaths::VectorColatitudeLongitude
GPlatesAppLogic::PlateVelocityUtils::calculate_velocity_colat_lon(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesMaths::FiniteRotation &finite_rotation1,
		const GPlatesMaths::FiniteRotation &finite_rotation2,
		const double &delta_time)
{
	const GPlatesMaths::Vector3D vector_xyz =
			GPlatesMaths::calculate_velocity_vector(point, finite_rotation1, finite_rotation2, delta_time);

	return GPlatesMaths::convert_vector_from_xyz_to_colat_lon(point, vector_xyz);
}


GPlatesMaths::VectorColatitudeLongitude
GPlatesAppLogic::PlateVelocityUtils::calculate_velocity_colat_lon(
		const GPlatesMaths::PointOnSphere &point,
		const GPlatesModel::integer_plate_id_type &reconstruction_plate_id,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type)
{
	const GPlatesMaths::Vector3D vector_xyz = calculate_velocity_vector(
			point,
			reconstruction_plate_id,
			reconstruction_tree_creator,
			reconstruction_time,
			velocity_delta_time,
			velocity_delta_time_type);

	return GPlatesMaths::convert_vector_from_xyz_to_colat_lon(point, vector_xyz);
}


GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworksVelocities::TopologicalNetworksVelocities(
		const std::vector<ResolvedTopologicalNetwork::non_null_ptr_type> &networks) :
	d_networks(networks)
{
}


boost::optional<
		std::pair<
				const GPlatesAppLogic::ReconstructionGeometry *,
				GPlatesMaths::Vector3D> >
GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworksVelocities::calculate_velocity(
		const GPlatesMaths::PointOnSphere &point,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type) const
{
	BOOST_FOREACH(const ResolvedTopologicalNetwork::non_null_ptr_type &network, d_networks)
	{
		boost::optional<
				std::pair<
						GPlatesMaths::Vector3D,
						boost::optional<const ResolvedTriangulation::Network::RigidBlock &> > >
				velocity = network->get_triangulation_network().calculate_velocity(
						point,
						velocity_delta_time,
						velocity_delta_time_type);
		if (velocity)
		{
			const GPlatesMaths::Vector3D &velocity_vector = velocity->first;

			// If the point was in one of the network's rigid blocks.
			if (velocity->second)
			{
				const ResolvedTriangulation::Network::RigidBlock &rigid_block = velocity->second.get();
				const ReconstructionGeometry *velocity_recon_geom =
						rigid_block.get_reconstructed_feature_geometry().get();

				return std::make_pair(velocity_recon_geom, velocity_vector);
			}

			const ReconstructionGeometry *velocity_recon_geom = network.get();
			return std::make_pair(velocity_recon_geom, velocity_vector);
		}
	}

	// Point is not inside any networks.
	return boost::none;
}
