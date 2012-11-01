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

#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "AppLogicUtils.h"
#include "GeometryCookieCutter.h"
#include "MultiPointVectorField.h"
#include "PlateVelocityUtils.h"
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionTree.h"
#include "ResolvedTopologicalNetwork.h"
#include "TopologyUtils.h"

#include "feature-visitors/ComputationalMeshSolver.h"
#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/CalculateVelocity.h"
#include "maths/FiniteRotation.h"

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
								gml_multi_point.multipoint());

				d_velocity_field_feature->add(
						GPlatesModel::TopLevelPropertyInline::create(
							domain_set_prop_name,
							domain_set_gml_multi_point));
			}
		};


		/**
		 * Information for calculating velocities - information that is not
		 * on a per-feature basis.
		 */
		class InfoForVelocityCalculations
		{
		public:
			InfoForVelocityCalculations(
					const ReconstructionTree &reconstruction_tree_1_,
					const ReconstructionTree &reconstruction_tree_2_) :
				reconstruction_tree_1(&reconstruction_tree_1_),
				reconstruction_tree_2(&reconstruction_tree_2_)
			{  }

			const ReconstructionTree *reconstruction_tree_1;
			const ReconstructionTree *reconstruction_tree_2;
		};


		/**
		 * A function to calculate velocity colat/lon - the signature of this function
		 * matches that required by TopologyUtils::interpolate_resolved_topology_networks().
		 *
		 * FIXME: This needs to be optimised - currently there's alot of common calculations
		 * within a feature that are repeated for each point - these calculations can be cached
		 * and looked up.
		 */
		std::vector<double>
		calc_velocity_vector_for_interpolation(
				const GPlatesMaths::PointOnSphere &point,
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
				const boost::any &user_data)
		{
			const InfoForVelocityCalculations &velocity_calc_info =
					boost::any_cast<const InfoForVelocityCalculations &>(user_data);

			// Get the reconstructionPlateId property value for the feature.
			static const GPlatesModel::PropertyName recon_plate_id_property_name =
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

			GPlatesModel::integer_plate_id_type recon_plate_id_value = 0;
			const GPlatesPropertyValues::GpmlPlateId *recon_plate_id = NULL;
			if ( GPlatesFeatureVisitors::get_property_value(
					feature_ref, recon_plate_id_property_name, recon_plate_id ) )
			{
				// The feature has a reconstruction plate ID.
				recon_plate_id_value = recon_plate_id->value();
			}

			const GPlatesMaths::VectorColatitudeLongitude velocity_colat_lon = 
					PlateVelocityUtils::calc_velocity_colat_lon(
							point,
							*velocity_calc_info.reconstruction_tree_1,
							*velocity_calc_info.reconstruction_tree_2,
							recon_plate_id_value);

			return PlateVelocityUtils::convert_velocity_colatitude_longitude_to_scalars(
					velocity_colat_lon);
		}


		/**
		 * Accumulates the interior polygons of the specified topological networks.
		 */
		void
		get_resolved_networks_interior_polygons(
				std::vector<reconstructed_feature_geometry_non_null_ptr_type> &resolved_networks_interior_polygons,
				const std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks)
		{
			std::vector<resolved_topological_network_non_null_ptr_type>::const_iterator networks_iter =
					resolved_topological_networks.begin();
			std::vector<resolved_topological_network_non_null_ptr_type>::const_iterator networks_end =
					resolved_topological_networks.end();
			for ( ; networks_iter != networks_end; ++networks_iter)
			{
				const ResolvedTopologicalNetwork::non_null_ptr_type &network = *networks_iter;

				ResolvedTopologicalNetwork::interior_polygon_const_iterator interior_polys_iter =
						network->interior_polygons_begin();
				ResolvedTopologicalNetwork::interior_polygon_const_iterator interior_polys_end =
						network->interior_polygons_end();
				for ( ; interior_polys_iter != interior_polys_end; ++interior_polys_iter)
				{
					const ResolvedTopologicalNetwork::InteriorPolygon &interior_poly = *interior_polys_iter;

					resolved_networks_interior_polygons.push_back(
							interior_poly.get_reconstructed_feature_geometry());
				}
			}
		}


		/**
		 * Visits velocity domain features and calculates velocities using plate IDs.
		 */
		class VelocitySolverByPlateID :
				public GPlatesModel::FeatureVisitor,
				public boost::noncopyable
		{
		public:
			VelocitySolverByPlateID(
					std::vector<multi_point_vector_field_non_null_ptr_type> &velocity_fields_to_populate,
					const double &recon_time,
					const ReconstructionTree::non_null_ptr_to_const_type &recon_tree,
					const ReconstructionTree::non_null_ptr_to_const_type &recon_tree_2,
					bool should_keep_features_without_recon_plate_id = true) :
				d_velocity_fields_to_populate(velocity_fields_to_populate),
				d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time)),
				d_recon_tree_ptr(recon_tree),
				d_recon_tree_2_ptr(recon_tree_2),
				d_should_keep_features_without_recon_plate_id(should_keep_features_without_recon_plate_id)
			{
			}

			bool
			initialise_pre_feature_properties(
					GPlatesModel::FeatureHandle &feature_handle)
			{
				// Clears parameters before initialising them for the current feature.
				d_reconstruction_plate_id = boost::none;
				d_finite_rotation = boost::none;
				d_finite_rotation_2 = boost::none;

				const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature_handle.reference();

				// Firstly find the reconstruction plate ID.
				ReconstructionFeatureProperties reconstruction_params(d_recon_time.value());
				reconstruction_params.visit_feature(feature_ref);

				// Secondly the feature must be defined at the reconstruction time.
				if (!reconstruction_params.is_feature_defined_at_recon_time())
				{
					// Don't visit.
					return false;
				}

				if (!reconstruction_params.get_recon_plate_id())
				{
					if (!d_should_keep_features_without_recon_plate_id)
					{
						// If there's no reconstruction plate ID *and* client only wants velocities
						// calculated for features with valid plate IDs then don't visit the current feature.
						return false;
					}

					// Process current feature but calculated velocity will be zero.
					return true;
				}
				d_reconstruction_plate_id = reconstruction_params.get_recon_plate_id().get();

				// The finite rotation to reconstruction the geometry of the current feature.
				// The reconstructed position will be where the velocity gets calculated.
				d_finite_rotation =
						d_recon_tree_ptr->get_composed_absolute_rotation(d_reconstruction_plate_id.get()).first;

				// The second finite rotation so velocity can be calculated using difference.
				d_finite_rotation_2 =
						d_recon_tree_2_ptr->get_composed_absolute_rotation(d_reconstruction_plate_id.get()).first;


				// Now visit the feature.
				return true;
			}

			virtual
			void
			visit_gml_line_string(
					GPlatesPropertyValues::GmlLineString &gml_line_string)
			{
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr = gml_line_string.polyline();

				// Since the domain is always stored as a multipoint create a new multipoint using
				// the vertices of the polyline.
				//
				// NOTE: This is slightly dodgy because we will end up creating a MultiPointVectorField
				// that stores a multi-point domain and a corresponding velocity field but the
				// geometry property iterator (referenced by the MultiPointVectorField) will be
				// a polyline geometry and not a multi-point geometry.
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain =
						GPlatesMaths::MultiPointOnSphere::create_on_heap(
								polyline_ptr->vertex_begin(), polyline_ptr->vertex_end());

				generate_velocities_in_multipoint_domain(velocity_domain);
			}

			virtual
			void
			visit_gml_multi_point(
					GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
			{
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain =
						gml_multi_point.multipoint();

				generate_velocities_in_multipoint_domain(velocity_domain);
			}

			virtual
			void
			visit_gml_orientable_curve(
					GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
			{
				gml_orientable_curve.base_curve()->accept_visitor(*this);
			}

			virtual
			void
			visit_gml_point(
					GPlatesPropertyValues::GmlPoint &gml_point)
			{
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_ptr = gml_point.point();

				// Since the domain is always stored as a multipoint create a new multipoint using the point.
				//
				// NOTE: This is slightly dodgy because we will end up creating a MultiPointVectorField
				// that stores a multi-point domain and a corresponding velocity field but the
				// geometry property iterator (referenced by the MultiPointVectorField) will be
				// a point geometry and not a multi-point geometry.
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain =
						GPlatesMaths::MultiPointOnSphere::create_on_heap(
								point_ptr.get(), point_ptr.get() + 1);

				generate_velocities_in_multipoint_domain(velocity_domain);
			}

			virtual
			void
			visit_gml_polygon(
					GPlatesPropertyValues::GmlPolygon &gml_polygon)
			{
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_ptr = gml_polygon.exterior();

				// Since the domain is always stored as a multipoint create a new multipoint using
				// the vertices of the polygon.
				//
				// NOTE: This is slightly dodgy because we will end up creating a MultiPointVectorField
				// that stores a multi-point domain and a corresponding velocity field but the
				// geometry property iterator (referenced by the MultiPointVectorField) will be
				// a polygon geometry and not a multi-point geometry.
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain =
						GPlatesMaths::MultiPointOnSphere::create_on_heap(
								polygon_ptr->vertex_begin(), polygon_ptr->vertex_end());

				generate_velocities_in_multipoint_domain(velocity_domain);
			}

			virtual
			void
			visit_gpml_constant_value(
					GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}


		private:

			/**
			 * The multi-point velocity fields that we are calculating velocities for a populating.
			 */
			std::vector<multi_point_vector_field_non_null_ptr_type> &d_velocity_fields_to_populate;

			/**
			 * The reconstruction time at which the mesh is being solved.
			 */
			const GPlatesPropertyValues::GeoTimeInstant d_recon_time;

			/**
			 * The reconstruction tree at the reconstruction time.
			 */
			ReconstructionTree::non_null_ptr_to_const_type d_recon_tree_ptr;

			/**
			 * The reconstruction tree at a small time delta from the reconstruction time.
			 */
			ReconstructionTree::non_null_ptr_to_const_type d_recon_tree_2_ptr;

			bool d_should_keep_features_without_recon_plate_id;

			/**
			 * The current feature's reconstruction plate id (if none).
			 */
			boost::optional<GPlatesModel::integer_plate_id_type> d_reconstruction_plate_id;

			/**
			 * The finite rotation to reconstruction the geometry of the current feature.
			 */
			boost::optional<GPlatesMaths::FiniteRotation> d_finite_rotation;

			/**
			 * The second finite rotation so that the velocity can be calculated using the difference.
			 */
			boost::optional<GPlatesMaths::FiniteRotation> d_finite_rotation_2;


			void
			generate_velocities_in_multipoint_domain(
					const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type &velocity_domain)
			{
				GPlatesModel::FeatureHandle::iterator property = *current_top_level_propiter();

				// Rotate the velocity domain if we have a reconstruction plate ID.
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type rotated_velocity_domain =
						velocity_domain;
				if (d_reconstruction_plate_id)
				{
					GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
							d_finite_rotation && d_finite_rotation_2,
							GPLATES_ASSERTION_SOURCE);

					rotated_velocity_domain = d_finite_rotation.get() * velocity_domain;
				}

				// Create an RFG purely for the purpose of representing the feature that generated
				// the plate ID - normally this is another feature (eg, static or topological polygon,
				// but here it is the original velocity domain feature (that we're visiting).
				// This is required in order for the velocity arrows to be coloured correctly -
				// because the colouring code requires a reconstruction geometry (it will then
				// lookup the plate ID or other feature property(s) depending on the colour scheme).
				const ReconstructedFeatureGeometry::non_null_ptr_type plate_id_rfg_ptr =
						ReconstructedFeatureGeometry::create(
								d_recon_tree_ptr,
								*property.handle_weak_ref(),
								property,
								rotated_velocity_domain,
								d_reconstruction_plate_id,
								d_recon_time);

				GPlatesMaths::MultiPointOnSphere::const_iterator iter = rotated_velocity_domain->begin();
				GPlatesMaths::MultiPointOnSphere::const_iterator end = rotated_velocity_domain->end();

				MultiPointVectorField::non_null_ptr_type vector_field_ptr =
						MultiPointVectorField::create_empty(
								d_recon_tree_ptr,
								rotated_velocity_domain,
								*property.handle_weak_ref(),
								property);
				MultiPointVectorField::codomain_type::iterator field_iter =
						vector_field_ptr->begin();

				for ( ; iter != end; ++iter, ++field_iter)
				{
					// Calculate velocity for the current point.
					generate_velocity(*iter, *field_iter, plate_id_rfg_ptr.get());
				}

				// Having created and populated the MultiPointVectorField, let's store it in the collection
				// of velocity fields.
				d_velocity_fields_to_populate.push_back(vector_field_ptr);
			}

			void
			generate_velocity(
					const GPlatesMaths::PointOnSphere &rotated_domain_point,
					boost::optional<MultiPointVectorField::CodomainElement> &range_element,
					const ReconstructedFeatureGeometry *plate_id_rfg_ptr)
			{
				if (!d_reconstruction_plate_id)
				{
					// The geometry has no plate ID and hence does not reconstructed and hence has no velocity.
					GPlatesMaths::Vector3D zero_velocity(0, 0, 0);
					range_element = MultiPointVectorField::CodomainElement(
							zero_velocity,
							MultiPointVectorField::CodomainElement::ReconstructedDomainPoint);

					return;
				}
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						d_finite_rotation && d_finite_rotation_2,
						GPLATES_ASSERTION_SOURCE);

				// Calculate the velocity.
				const GPlatesMaths::Vector3D vector_xyz =
						GPlatesMaths::calculate_velocity_vector(
								rotated_domain_point,
								d_finite_rotation.get(),
								d_finite_rotation_2.get());

				range_element = MultiPointVectorField::CodomainElement(
						vector_xyz,
						MultiPointVectorField::CodomainElement::ReconstructedDomainPoint,
						d_reconstruction_plate_id.get(),
						plate_id_rfg_ptr);
			}
		};
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

	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			feature_collection, detect_velocity_mesh_nodes);

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

	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
			feature_collection_with_mesh_nodes, add_velocity_field_features);

	// Return the newly created feature collection.
	return velocity_field_feature_collection;
}


void
GPlatesAppLogic::PlateVelocityUtils::solve_velocities_by_plate_id(
		std::vector<multi_point_vector_field_non_null_ptr_type> &multi_point_velocity_fields,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &velocity_domain_feature_collections)
{
	// Return early if there are no velocity domain feature collections on which to solve velocities.
	if (velocity_domain_feature_collections.empty())
	{
		return;
	}

	// FIXME:  Should this '1' should be user controllable?
	const double reconstruction_time_1 = reconstruction_time;
	const double reconstruction_time_2 = reconstruction_time_1 + 1;

	// Our two reconstruction trees for velocity calculations.
	const reconstruction_tree_non_null_ptr_to_const_type reconstruction_tree_1 =
			reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time_1);
	const reconstruction_tree_non_null_ptr_to_const_type reconstruction_tree_2 =
			reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time_2);

	// Visit the velocity domain feature collections and calculate velocities by plate ID.
	VelocitySolverByPlateID velocity_solver(
			multi_point_velocity_fields,
			reconstruction_time_1,  // "the" reconstruction time
			reconstruction_tree_1,  // "the" reconstruction tree
			reconstruction_tree_2,
			true); // keep features without recon plate id

	GPlatesAppLogic::AppLogicUtils::visit_feature_collections(
			velocity_domain_feature_collections.begin(),
			velocity_domain_feature_collections.end(),
			static_cast<GPlatesModel::FeatureVisitor &>(velocity_solver));
}


void
GPlatesAppLogic::PlateVelocityUtils::solve_velocities_on_surfaces(
		std::vector<multi_point_vector_field_non_null_ptr_type> &multi_point_velocity_fields,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const double &reconstruction_time,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &velocity_domain_feature_collections,
		const std::vector<reconstructed_feature_geometry_non_null_ptr_type> &velocity_surface_reconstructed_static_polygons,
		const std::vector<resolved_topological_geometry_non_null_ptr_type> &velocity_surface_resolved_topological_boundaries,
		const std::vector<resolved_topological_network_non_null_ptr_type> &velocity_surface_resolved_topological_networks)
{
	// Return early if there are no velocity domain feature collections on which to solve velocities.
	if (velocity_domain_feature_collections.empty())
	{
		return;
	}

	// FIXME:  Should this '1' should be user controllable?
	const double reconstruction_time_1 = reconstruction_time;
	const double reconstruction_time_2 = reconstruction_time_1 + 1;

	// Our two reconstruction trees for velocity calculations.
	const reconstruction_tree_non_null_ptr_to_const_type reconstruction_tree_1 =
			reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time_1);
	const reconstruction_tree_non_null_ptr_to_const_type reconstruction_tree_2 =
			reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time_2);


	// Get the reconstructed feature geometries and wrap them in a structure that can do
	// point-in-polygon tests.
	GeometryCookieCutter reconstructed_static_polygons_query(
			reconstruction_time,
			velocity_surface_reconstructed_static_polygons,
			boost::none/*velocity_surface_resolved_topological_boundaries*/,
			boost::none/*velocity_surface_resolved_topological_networks*/);


	// Get the resolved topological boundaries and optimise them for point-in-polygon tests.
	// Later the velocity solver will query the boundaries at various points and
	// then calculate velocities using the best matching boundary.
	const TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type resolved_boundaries_query =
			TopologyUtils::query_resolved_topologies_for_geometry_partitioning(velocity_surface_resolved_topological_boundaries);


	// Get the resolved topological networks.
	// This effectively goes through all the node points in all topological networks
	// and generates velocity data for them.
	// Later the velocity solver will query the networks for the interpolated velocity
	// at various points.

	const InfoForVelocityCalculations velocity_calc_info(*reconstruction_tree_1, *reconstruction_tree_2);

	/*
	 * The following callback function will take a ResolvedTopologicalNetworkImpl pointer
	 * as its first argument and a TopologyUtils::network_interpolation_query_callback_type
	 * as its second argument. It will first construct a TopologicalNetworkVelocities object
	 * using the second argument and then pass that as an argument to
	 * ResolvedTopologicalNetworkImpl::set_network_velocities().
	 */
	const TopologyUtils::network_interpolation_query_callback_type callback_function =
			boost::lambda::bind(
					// The member function that this callback will delegate to
					&ResolvedTopologicalNetwork::set_network_velocities,
					boost::lambda::_1/*the ResolvedTopologicalNetworkImpl object to set it on*/,
					boost::lambda::bind(
							// Construct a TopologicalNetworkVelocities object from the query
							boost::lambda::constructor<TopologicalNetworkVelocities>(),
							boost::lambda::_2/*the query to store in the network*/));

	// Fill the topological network with velocities data at all the network points.
	const TopologyUtils::resolved_networks_for_interpolation_query_type resolved_networks_query =
			TopologyUtils::query_resolved_topology_networks_for_interpolation(
					velocity_surface_resolved_topological_networks,
					&calc_velocity_vector_for_interpolation,
					// Two scalars per velocity...
					NUM_TOPOLOGICAL_NETWORK_VELOCITY_COMPONENTS,
					// Info used by "calc_velocity_vector_for_interpolation()"...
					boost::any(velocity_calc_info),
					callback_function);

	// Get the topological network interior polygons (if any) and wrap them in a structure
	// that can do point-in-polygon tests.
	// These are static (shape) interior regions of topological networks whose velocities
	// are treated like regular static polygons (except these are given preference since they
	// belong to a topological network).
	std::vector<reconstructed_feature_geometry_non_null_ptr_type> resolved_networks_interior_polygons;
	get_resolved_networks_interior_polygons(
			resolved_networks_interior_polygons,
			velocity_surface_resolved_topological_networks);
	GeometryCookieCutter resolved_networks_interior_polygons_query(
			reconstruction_time,
			resolved_networks_interior_polygons,
			boost::none/*velocity_surface_resolved_topological_boundaries*/,
			boost::none/*velocity_surface_resolved_topological_networks*/);

	// Visit the velocity domain feature collections and calculate velocities.
	GPlatesFeatureVisitors::ComputationalMeshSolver velocity_solver(
			multi_point_velocity_fields,
			reconstruction_time_1,  // "the" reconstruction time
			reconstruction_tree_1,  // "the" reconstruction tree
			reconstruction_tree_2,
			reconstructed_static_polygons_query,
			resolved_boundaries_query,
			resolved_networks_query,
			resolved_networks_interior_polygons_query,
			true); // keep features without recon plate id

	GPlatesAppLogic::AppLogicUtils::visit_feature_collections(
			velocity_domain_feature_collections.begin(),
			velocity_domain_feature_collections.end(),
			static_cast<GPlatesModel::FeatureVisitor &>(velocity_solver));
}


GPlatesMaths::Vector3D
GPlatesAppLogic::PlateVelocityUtils::calc_velocity_vector(
		const GPlatesMaths::PointOnSphere &point,
		const ReconstructionTree &reconstruction_tree1,
		const ReconstructionTree &reconstruction_tree2,
		const GPlatesModel::integer_plate_id_type &reconstruction_plate_id)
{
	// Get the finite rotation for this plate id.
	const GPlatesMaths::FiniteRotation &fr_t1 =
			reconstruction_tree1.get_composed_absolute_rotation(reconstruction_plate_id).first;

	const GPlatesMaths::FiniteRotation &fr_t2 =
			reconstruction_tree2.get_composed_absolute_rotation(reconstruction_plate_id).first;

	return GPlatesMaths::calculate_velocity_vector(point, fr_t1, fr_t2);
}


GPlatesMaths::VectorColatitudeLongitude
GPlatesAppLogic::PlateVelocityUtils::calc_velocity_colat_lon(
		const GPlatesMaths::PointOnSphere &point,
		const ReconstructionTree &reconstruction_tree1,
		const ReconstructionTree &reconstruction_tree2,
		const GPlatesModel::integer_plate_id_type &reconstruction_plate_id)
{
	const GPlatesMaths::Vector3D vector_xyz = calc_velocity_vector(
			point, reconstruction_tree1, reconstruction_tree2, reconstruction_plate_id);

	return GPlatesMaths::convert_vector_from_xyz_to_colat_lon(point, vector_xyz);
}


boost::optional<GPlatesMaths::VectorColatitudeLongitude>
GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworkVelocities::interpolate_velocity(
		const GPlatesMaths::PointOnSphere &point)
{
	const boost::optional< std::vector<double> > interpolated_velocity =
			TopologyUtils::interpolate_resolved_topology_network(
					d_network_velocities_query, point);

	if (!interpolated_velocity)
	{
		return boost::none;
	}

	return convert_velocity_scalars_to_colatitude_longitude(*interpolated_velocity);
}


void
GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworkVelocities::get_network_velocities(
		std::vector<GPlatesMaths::PointOnSphere> &network_points,
		std::vector<GPlatesMaths::VectorColatitudeLongitude> &network_velocities) const
{
	// Get the network velocities as scalar tuples.
	std::vector< std::vector<double> > network_scalar_tuple_sequence;
	TopologyUtils::get_resolved_topology_network_scalars(
			d_network_velocities_query,
			network_points,
			network_scalar_tuple_sequence);

	// Assemble the scalar tuples as velocity vectors.

	network_velocities.reserve(network_scalar_tuple_sequence.size());

	// Iterate through the scalar tuple sequence and convert to velocity colat/lon.
	typedef std::vector< std::vector<double> > scalar_tuple_seq_type;
	scalar_tuple_seq_type::const_iterator scalar_tuple_iter = network_scalar_tuple_sequence.begin();
	scalar_tuple_seq_type::const_iterator scalar_tuple_end = network_scalar_tuple_sequence.end();
	for ( ; scalar_tuple_iter != scalar_tuple_end; ++scalar_tuple_iter)
	{
		const std::vector<double> &scalar_tuple = *scalar_tuple_iter;

		network_velocities.push_back(
				convert_velocity_scalars_to_colatitude_longitude(scalar_tuple));
	}
}
