
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-13 09:31:07 -0700 (Wed, 13 Aug 2008) $
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FEATUREVISITORS_RECONSTRUCTEDFEATUREGEOMETRYFINDER_H
#define GPLATES_FEATUREVISITORS_RECONSTRUCTEDFEATUREGEOMETRYFINDER_H

#include <list>
#include <boost/optional.hpp>

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"
#include "model/Reconstruction.h"

#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonOnSphere.h"


namespace GPlatesFeatureVisitors
{
	class ReconstructedFeatureGeometryFinder: 
		public GPlatesModel::FeatureVisitor,
		public GPlatesMaths::ConstGeometryOnSphereVisitor
	{

	public:

		/**
		 * Records details about the top-level items (properties) that we are building.
		 * This allows us to add all top-level items in a single pass, after we have figured
		 * out whether the property contains geometry or not.
		 */
		struct PropertyInfo
		{
			bool is_geometric_property;
		};

		typedef std::vector<PropertyInfo> property_info_vector_type;
		typedef property_info_vector_type::const_iterator property_info_vector_const_iterator;

		/**
		 * Stores the reconstructed geometry and the property it belongs to.
		 *
		 * This allows us to display the reconstructed coordinates at the same time as the
		 * present-day coordinates.
		 */
		struct ReconstructedGeometryInfo
		{
			const GPlatesModel::FeatureHandle::properties_iterator d_property;
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_geometry;
			
			ReconstructedGeometryInfo(
					const GPlatesModel::FeatureHandle::properties_iterator property,
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry):
				d_property(property),
				d_geometry(geometry)
			{  }
		};

		// FIXME:  Since this container contains ReconstructedGeometryInfo instances, which
		// are effectively just a few pointers (inexpensive to copy, and no conceptual
		// problems with copying), would this container be better as a 'std::vector'?
		typedef std::list<ReconstructedGeometryInfo> geometries_for_property_type;
		typedef geometries_for_property_type::const_iterator geometries_for_property_const_iterator;


		// map of old feature id string to feature's RFG 
		typedef std::map<std::string, const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type > id_to_geometry_map_type;

#if 0
// FIXME : save this code for example on using separate maps 
//
		// map of old feature id string to feature's vertex list
		typedef std::map<std::string, const std::list<GPlatesMaths::PointOnSphere> > id_to_vertex_list_map_type;

		// map of old feature id string to point on sphere pointers
		typedef std::map<std::string, const GPlatesMaths::PointOnSphere *> id_to_point_map_type;

		// map of old feature id string to polyline on sphere pointers
		typedef std::map<std::string, const GPlatesMaths::PolylineOnSphere *> id_to_polyline_map_type;
#endif


		explicit
		ReconstructedFeatureGeometryFinder(
				GPlatesModel::Reconstruction &reconstruction):
			d_reconstruction_ptr(&reconstruction)
		{  
			d_num_features = 0;
		}

		virtual
		~ReconstructedFeatureGeometryFinder() {  }

		virtual
		void
		visit_feature_handle(
				GPlatesModel::FeatureHandle &feature_handle);
		
		virtual
		void
		visit_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_inline_property_container(
				GPlatesModel::InlinePropertyContainer &inline_property_container);

		virtual
		void
		visit_gml_line_string(
				GPlatesPropertyValues::GmlLineString &gml_line_string);

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
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_vertex_list.clear();
			d_vertex_list.push_back( *point_on_sphere );
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_vertex_list.clear();

			GPlatesMaths::PolylineOnSphere::vertex_const_iterator beg = polyline_on_sphere->vertex_begin();
			GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline_on_sphere->vertex_end();

			copy(beg, end, back_inserter( d_vertex_list ) );
		}


		/**
		* Report on the information found from this visitor 
		*/
		void
		report();

		/**
		* Access the RFG map
		*/
		boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
		get_geometry_from_feature_id( 
			std::string old_id ); 

		/**
		* Access the Vertex List map, populate the passed in list
		*/
		void
		get_vertex_list_from_feature_id( 
			std::list<GPlatesMaths::PointOnSphere> &vertex_list,
			std::string old_id );

#if 0
// FIXME: save this code for example on using individual maps
		/**
		* Access the Point On Sphere pointer map
		*/
		const
		GPlatesMaths::PointOnSphere*
		get_point_from_feature_id( 
			std::string old_id );

		/**
		* Access the Point On Sphere pointer map
		*/
		const
		GPlatesMaths::PolylineOnSphere*
		get_polyline_from_feature_id( 
			std::string old_id );
#endif


	private:

		/**
		 * The Reconstruction which we will scan for RFGs from.
		 */
		GPlatesModel::Reconstruction *d_reconstruction_ptr;

		GPlatesModel::FeatureHandle *d_feature_handle_ptr;
		
		/**
		 * When visiting a FeatureHandle, this member will record the properties_iterator
		 * of the last property visited.
		 */
		boost::optional<GPlatesModel::FeatureHandle::properties_iterator> d_last_property_visited;

		/**
		 * Records details about the top-level items (properties) that we are building.
		 * This allows us to add all top-level items in a single pass, after we have figured
		 * out whether the property contains geometry or not.
		 */
		property_info_vector_type d_property_info_vector;

		/**
		 * Stores the reconstructed geometries and the properties they belong to.
		 *
		 * This allows us to add the reconstructed coordinates at the same time as the
		 * present-day coordinates.
		 */
		geometries_for_property_type d_rfg_geometries;

		/**
		 * Iterates over d_reconstruction_ptr's RFGs, fills in the d_rfg_geometries table
		 * with geometry found from RFGs which belong to the given feature.
		 */
		void
		populate_rfg_geometries_for_feature(
				const GPlatesModel::FeatureHandle &feature_handle);

		/**
		 * Searches the d_rfg_geometries table for geometry matching the given property.
		 */
		boost::optional<const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
		get_reconstructed_geometry_for_property(
				const GPlatesModel::FeatureHandle::properties_iterator property);

		
		/**
		* the number of features visited by this visitor 
		*/
		int d_num_features;

		/**
		* Temporary list of vertices
		*/
		std::list< GPlatesMaths::PointOnSphere > d_vertex_list;

		/**
		* Stores the reconstructed feature geoms
		*/
		id_to_geometry_map_type d_id_to_geometry_map;

#if 0
// FIXME: save this code for examples of using individual maps
//
		/** 
		* Stores the vertex list map 
		*/
		id_to_vertex_list_map_type d_id_to_vertex_list_map;

		/** 
		* Stores the points map 
		*/
		id_to_point_map_type d_id_to_point_map;

		/** 
		* Stores the polylines map 
		*/
		id_to_polyline_map_type d_id_to_polyline_map;


		/**
		 * Iterates over d_reconstruction_ptr's RFGs, fills in the d_id_to_geometry_map
		 * with geometry found from RFGs which belong to the given feature.
		 */
		void 
		populate_feature_maps( const GPlatesModel::FeatureHandle &feature_handle );
#endif 
		
	};

}

#endif  // GPLATES_FEATUREVISITORS_RECONSTRUCTEDFEATUREGEOMETRYFINDER_H
