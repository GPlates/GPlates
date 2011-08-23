/* $Id$ */

/**
 * @file 
 * Contains the definition of class ComputationalMeshSolver, which is a FeatureVisitor.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
 * Copyright (C) 2008, 2009 California Institute of Technology 
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

#ifndef GPLATES_FEATUREVISITORS_COMPUTATIONAL_MESH_SOLVER_H
#define GPLATES_FEATUREVISITORS_COMPUTATIONAL_MESH_SOLVER_H

#include <list>
#include <vector>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>

#include "app-logic/AppLogicFwd.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/MultiPointVectorField.h"
#include "app-logic/TopologyUtils.h"

#include "global/types.h"

#include "gui/PlateIdColourPalettes.h"

#include "maths/FiniteRotation.h"

#include "model/types.h"
#include "model/FeatureVisitor.h"
#include "model/FeatureCollectionHandle.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryParameters.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesAppLogic
{
	class GeometryCookieCutter;
}

namespace GPlatesGui
{
	class Colour;
}

namespace GPlatesMaths
{
	class PointOnSphere;
	class VectorColatitudeLongitude;
}

namespace GPlatesFeatureVisitors
{
	class ComputationalMeshSolver :
			public GPlatesModel::FeatureVisitor,
			public boost::noncopyable
	{
	public:

		struct ReconstructedFeatureGeometryAccumulator
		{
			/**
			 * Whether or not we're performing reconstructions, 
			 * or just gathering information.
			 */
			bool d_perform_reconstructions;

			/**
			 * Whether or not the current feature is defined at this reconstruction
			 * time.
			 *
			 * The value of this member defaults to true; it's only set to false if
			 * both: (i) a "gml:validTime" property is encountered which contains a
			 * "gml:TimePeriod" structural type; and (ii) the reconstruction time lies
			 * outside the range of the valid time.
			 */
			bool d_feature_is_defined_at_recon_time;

			boost::optional<GPlatesModel::integer_plate_id_type> d_recon_plate_id;
			boost::optional<GPlatesMaths::FiniteRotation> d_recon_rotation;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_appearance;

			ReconstructedFeatureGeometryAccumulator():
				d_perform_reconstructions(false),
				d_feature_is_defined_at_recon_time(true)
			{  }
		};


		//
		// the ComputationalMeshSolver class
		//

		/**
		 * Construct a computational mesh solver for reconstruction time @a recon_time.
		 *
		 * The reconstruction tree at this time is @a recon_tree.  @a recon_tree_2 is the
		 * reconstruction tree at a small time delta.
		 */
		ComputationalMeshSolver(
				std::vector<GPlatesAppLogic::multi_point_vector_field_non_null_ptr_type> &velocity_fields_to_populate,
				const double &recon_time,
				const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &recon_tree,
				const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &recon_tree_2,
				const GPlatesAppLogic::GeometryCookieCutter &reconstructed_static_polygons_query,
				const GPlatesAppLogic::TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type &
						resolved_boundaries_for_partitioning_geometry_query,
				const GPlatesAppLogic::TopologyUtils::resolved_networks_for_interpolation_query_type &
						resolved_networks_for_velocity_interpolation,
				const GPlatesAppLogic::GeometryCookieCutter &resolved_networks_interior_polygons_query,
				bool should_keep_features_without_recon_plate_id = true);

		virtual
		~ComputationalMeshSolver() 
		{  }

		virtual
		void
		visit_feature_handle(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_gml_line_string(
				GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
		void
		visit_gml_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		virtual
		void
		visit_gml_orientable_curve(
				GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
		void
		visit_gml_point(
				GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
		void
		visit_gml_polygon(
				GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
		void
		visit_gml_time_period(
				GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

		void
		report();

	private:
		/**
		 * The multi-point velocity fields that we are calculating velocities for a populating.
		 */
		std::vector<GPlatesAppLogic::multi_point_vector_field_non_null_ptr_type> &d_velocity_fields_to_populate;

		/**
		 * The reconstruction time at which the mesh is being solved.
		 */
		const GPlatesPropertyValues::GeoTimeInstant d_recon_time;

		/**
		 * The reconstruction tree at the reconstruction time.
		 */
		GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type d_recon_tree_ptr;

		/**
		 * The reconstruction tree at a small time delta from the reconstruction time.
		 */
		GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type d_recon_tree_2_ptr;

		/**
		 * Used to determine if points are inside reconstructed static polygons.
		 */
		const GPlatesAppLogic::GeometryCookieCutter &d_reconstructed_static_polygons_query;

		/**
		 * Used to determine if points are inside resolved topological plate polygons.
		 */
		GPlatesAppLogic::TopologyUtils::resolved_boundaries_for_geometry_partitioning_query_type
				d_resolved_boundaries_for_partitioning_geometry_query;

		/**
		 * Used to determine if points are inside resolved topological networks.
		 */
		GPlatesAppLogic::TopologyUtils::resolved_networks_for_interpolation_query_type
				d_resolved_networks_for_velocity_interpolation;

		/**
		 * Used to determine if points are inside the interior polygons of topological networks.
		 */
		const GPlatesAppLogic::GeometryCookieCutter &d_resolved_networks_interior_polygons_query;

		boost::optional<ReconstructedFeatureGeometryAccumulator> d_accumulator;
		bool d_should_keep_features_without_recon_plate_id;


		/** the number of features visited by this visitor */
		int d_num_features;
		int d_num_meshes;
		int d_num_points;

		/**
		 * A pointer to the current feature handle.
		 *
		 * We expect this to be non-NULL while we're visiting a feature handle.
		 */
		GPlatesModel::FeatureHandle *d_feature_handle_ptr;

		void
		generate_velocities_in_multipoint_domain(
				const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type &velocity_domain);

		// 2D 
		void
		process_point_in_base_triangulation(
				const GPlatesMaths::PointOnSphere &point,
				boost::optional<GPlatesAppLogic::MultiPointVectorField::CodomainElement> &range_element);

		void
		set_velocity_from_base_triangulation(
				const GPlatesMaths::PointOnSphere &point,
				boost::optional<GPlatesAppLogic::MultiPointVectorField::CodomainElement> &range_element,
				const GPlatesMaths::VectorColatitudeLongitude &velocity_colat_lon);

#if 0
		// 2D+C 
		void
		process_point_in_constrained_triangulation(
				const GPlatesMaths::PointOnSphere &point,
				boost::optional<GPlatesAppLogic::MultiPointVectorField::CodomainElement> &range_element);

		void
		set_velocity_from_constrained_triangulation(
				const GPlatesMaths::PointOnSphere &point,
				boost::optional<GPlatesAppLogic::MultiPointVectorField::CodomainElement> &range_element,
				const GPlatesMaths::VectorColatitudeLongitude &velocity_colat_lon);
#endif

		// Plate Polygon
		void
		process_point_in_plate_polygon(
				const GPlatesMaths::PointOnSphere &point,
				boost::optional<GPlatesAppLogic::MultiPointVectorField::CodomainElement> &range_element,
				const GPlatesAppLogic::TopologyUtils::resolved_topological_boundary_seq_type &
						resolved_topological_boundaries_containing_point);

		void
		process_point_in_static_polygon(
				const GPlatesMaths::PointOnSphere &point,
				boost::optional<GPlatesAppLogic::MultiPointVectorField::CodomainElement> &range_element,
				const GPlatesAppLogic::ReconstructionGeometry *reconstructed_static_polygon_containing_point,
				const GPlatesAppLogic::MultiPointVectorField::CodomainElement::Reason reason);
	};
}

#endif  // GPLATES_FEATUREVISITORS_COMPUTATIONAL_MESH_SOLVER_H
