
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
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

#ifndef GPLATES_APP_LOGIC_TOPOLOGY_NETWORK_RESOLVER_H
#define GPLATES_APP_LOGIC_TOPOLOGY_NETWORK_RESOLVER_H

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionFeatureProperties.h"

#include "maths/GeometryOnSphere.h"

#include "model/FeatureId.h"
#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/Model.h"
#include "model/types.h"

#include "property-values/GpmlTopologicalSection.h"


namespace GPlatesPropertyValues
{
	class GpmlPropertyDelegate;
	class GpmlTimeWindow;
}

namespace GPlatesAppLogic
{
	class ReconstructionGeometryCollection;

	/**
	 * Finds all topological network features (in the features visited)
	 * that exist at a particular reconstruction time and creates
	 * @a ResolvedTopologicalNetwork objects for each one and stores them
	 * in a @a ReconstructionGeometryCollection object.
	 *
	 * @pre the features referenced by any of these topological network features
	 *      must have already been reconstructed and reference the same @a ReconstructionTree
	 *      object referenced by the @a ReconstructionGeometryCollection object passed into
	 *      the constructor of @a TopologyNetworkResolver.
	 */
	class TopologyNetworkResolver: 
		public GPlatesModel::FeatureVisitor,
		private boost::noncopyable
	{

	public:
		TopologyNetworkResolver(
				ReconstructionGeometryCollection &reconstruction_geometry_collection);

		virtual
		~TopologyNetworkResolver() 
		{  }

		virtual
		bool
		initialise_pre_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_piecewise_aggregation(
				GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation);

		void
		visit_gpml_time_window(
				GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window);

		virtual
		void
		visit_gpml_topological_polygon(
			 	GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon);

		virtual
		void
		visit_gpml_topological_line_section(
				GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section);

		virtual
		void
		visit_gpml_topological_point(
				GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point);

	private:
		/**
		 * Stores/builds information from iterating over @a GpmlTopologicalSection objects.
		 */
		class ResolvedNetwork
		{
		public:
			//! Reset in preparation for a new sequence of topological sections.
			void
			reset()
			{
				d_sections.clear();
			}

			//! Keeps track of topological section information when visiting topological sections.
			class Section
			{
			public:
				Section(
						const GPlatesModel::FeatureId &source_feature_id) :
					d_source_feature_id(source_feature_id)
				{  }

				//! The feature id of the feature referenced by this topological section.
				GPlatesModel::FeatureId d_source_feature_id;
				//! The source @a ReconstructedFeatureGeometry.
				boost::optional<ReconstructedFeatureGeometry::non_null_ptr_type> d_source_rfg;
				//! The segment geometry.
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_geometry;
			};

			//! Typedef for a sequence of sections.
			typedef std::vector<Section> section_seq_type;

			//! Sequence of sections of the currently visited topological polygon.
			section_seq_type d_sections;
		};


		//! Destination for resolved networks.
		ReconstructionGeometryCollection &d_reconstruction_geometry_collection;

		//! The current feature being visited.
		GPlatesModel::FeatureHandle::weak_ref d_currently_visited_feature;

		//! Gathers some useful reconstruction parameters.
		ReconstructionFeatureProperties d_reconstruction_params;

		//! Used to help build the resolved network of the current topological polygon.
		ResolvedNetwork d_resolved_network;

		//! The number of topologies visited.
		int d_num_topologies;


		/**
		 * Create a @a ResolvedTopologicalNetwork from information gathered
		 * from the most recently visited topological polygon
		 * (stored in @a d_resolved_network) and add it to the @a Reconstruction.
		 */
		void
		create_resolved_topology_network();

		void
		record_topological_sections(
				std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &
						sections);

		void
		record_topological_section_reconstructed_geometry(
				ResolvedNetwork::Section &section,
				const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate);

		void
		debug_output_topological_section_feature_id(
				const GPlatesModel::FeatureId &section_feature_id);
	};
}

#endif  // GPLATES_APP_LOGIC_TOPOLOGY_NETWORK_RESOLVER_H
