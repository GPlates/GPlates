
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
#include <boost/optional.hpp>

#include "app-logic/TopologyUtils.h"

#include "global/types.h"

#include "gui/PlateIdColourPalettes.h"

#include "maths/types.h"
#include "maths/FiniteRotation.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/LatLonPointConversions.h"

#include "model/types.h"
#include "model/FeatureVisitor.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"
#include "model/Reconstruction.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryParameters.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesFeatureVisitors
{
	class ComputationalMeshSolver: public GPlatesModel::FeatureVisitor
	{
	public:

		//
		// FROM ReconstructedFeatureGeometryPopulator.h
		//
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


		typedef std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>
				reconstruction_geometries_type;

		//
		// the ComputationalMeshSolver class
		//

		// This is a mimic of ReconstructedFeatureGeometryPopulator()

		ComputationalMeshSolver(
			GPlatesModel::Reconstruction &reconstruction,
			const double &recon_time,
			const double &recon_time_2,
			unsigned long root_plate_id,
			//GPlatesModel::Reconstruction &recon,
			GPlatesModel::ReconstructionTree &recon_tree,
			GPlatesModel::ReconstructionTree &recon_tree_2,
			const GPlatesAppLogic::TopologyUtils::resolved_geometries_for_geometry_partitioning_query_type &
					resolved_geoms_for_partitioning_geometry_query,
			//reconstruction_geometries_type &reconstructed_geometries,
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type point_layer,
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type arrow_layer,
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
		visit_gml_multi_point(
				GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

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

#if 0
		virtual
		void
		visit_gml_data_block(
				GPlatesPropertyValues::GmlDataBlock &gml_data_block);
#endif

		void
		report();

		void
		process_point( const GPlatesMaths::PointOnSphere &point );

	private:
		
		//
		// from ReconstructedFeatureGeometryPopulator.h
		//
		const GPlatesPropertyValues::GeoTimeInstant d_recon_time;
		GPlatesModel::integer_plate_id_type d_root_plate_id;
		GPlatesModel::Reconstruction *d_recon_ptr;
		GPlatesModel::ReconstructionTree *d_recon_tree_ptr;
		GPlatesModel::ReconstructionTree *d_recon_tree_2_ptr;
		GPlatesAppLogic::TopologyUtils::resolved_geometries_for_geometry_partitioning_query_type
				d_resolved_geoms_for_partitioning_geometry_query;


		//reconstruction_geometries_type *d_reconstruction_geometries_to_populate;

		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_rendered_point_layer;
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_rendered_arrow_layer;

		boost::optional<ReconstructedFeatureGeometryAccumulator> d_accumulator;
		bool d_should_keep_features_without_recon_plate_id;

		// This constructor should never be defined, because we don't want to allow
		// copy-construction.
		ComputationalMeshSolver(
				const ComputationalMeshSolver &);

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		ComputationalMeshSolver &
		operator=(
			const ComputationalMeshSolver &);

		/** the number of features visited by this visitor */
		int d_num_features;
		int d_num_meshes;
		int d_num_points;

		/** used to set the color of the points found on a plate */
		GPlatesGui::DefaultPlateIdColourPalette d_colour_palette;

		/** vectors to hold computed values */
		std::vector<double> d_velocity_colat_values;
		std::vector<double> d_velocity_lon_values;
	};

}

#endif  // GPLATES_FEATUREVISITORS_COMPUTATIONAL_MESH_SOLVER_H
