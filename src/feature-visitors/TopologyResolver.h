
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
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

#ifndef GPLATES_FEATUREVISITORS_TOPOLOGY_RESOLVER_H
#define GPLATES_FEATUREVISITORS_TOPOLOGY_RESOLVER_H

#include <list>
#include <boost/optional.hpp>
#include <QTreeWidget>

#include "global/types.h"

#include "maths/FiniteRotation.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PolygonOnSphere.h"

#include "model/types.h"
#include "model/FeatureVisitor.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/PropertyValue.h"
#include "model/PropertyName.h"
#include "model/Reconstruction.h"

#include "property-values/GeoTimeInstant.h"

#include "feature-visitors/ReconstructedFeatureGeometryFinder.h"


#define POINT_OUTSIDE_POLYGON 0
#define POINT_ON_POLYGON  1
#define POINT_INSIDE_POLYGON  2

namespace GPlatesPropertyValues
{
	class GpmlKeyValueDictionaryElement;
	class GpmlTimeSample;
	class GpmlTimeWindow;
}


namespace GPlatesFeatureVisitors
{
	class TopologyResolver: public GPlatesModel::FeatureVisitor
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

			boost::optional<GPlatesModel::FeatureHandle::properties_iterator> d_current_property;
			boost::optional<GPlatesModel::integer_plate_id_type> d_recon_plate_id;
			boost::optional<GPlatesMaths::FiniteRotation> d_recon_rotation;
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_appearance;

			ReconstructedFeatureGeometryAccumulator():
				d_perform_reconstructions(false),
				d_feature_is_defined_at_recon_time(true)
			{  }

			/**
			 * Return the name of the current property.
			 *
			 * Note that this function assumes we're actually in a property!
			 */
			const GPlatesModel::PropertyName &
			current_property_name() const
			{
				return (**d_current_property)->property_name();
			}
		};


		typedef std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>
				reconstruction_geometries_type;

		// typedef std::vector<GPlatesModel::ReconstructedFeatureGeometry> 
			// reconstructed_geometries_type;


		//
		// From GPlates 8
		//
		/** 
		* A BoundaryFeature struct represents a reconstructed 
		* geologic component of a plates's boundary.
		*
		* A BoundaryFeature is formed from:
		* a GPlatesGeo feature with its properties, 
		* the reconstructed vertices of the feature, and,
		* the various intersection and overlap relations between 
		* the feature and its neighbors in a boundary list. 
		*
		* Some Member data will change when:
		* the feature undergoes reconstructions, and when,
		* the oder of the nodes changes (and hence the relations 
		* between neighbors changes).
		* 
		*/
		struct BoundaryFeature 
		{

			// The next few data members will most likely
			// come from a user click-on-screen
			// or a simulated click
			// with transfer via Layout::CloseDatum
			//

			/** Pointer to the original GeoData feature */
			// GPlatesGeo::DrawableData *m_feature;
			GPlatesModel::FeatureHandle &m_feature;

			/** The feature id */
			std::string m_feature_id;
			
			/** The feature type */
			GPlatesGlobal::FeatureTypes m_feature_type;

			/** The feature's reconstructed vertices */
			std::list<GPlatesMaths::PointOnSphere> m_vertex_list;

			/** 
			* The PointOnSphere that the user clicked on to select 
			* this feature; stored in present day coordinates ; 
			* will need to be rotated with feature->GetRotationId() 
			* if used to compare to other reconstructed features
			*/
			GPlatesMaths::PointOnSphere m_click_point;

			/** Value of how close to the feature the user clicked on */
			GPlatesMaths::real_t m_closeness;

			/** 
			* Use vertices from this feature in the reverse order
			* of how they are stored in the feature itself.
			* Only applicable to LINE features.
			*/
			bool m_use_reverse;

			//
			// The following node data will change:
			// upon reconstrction
			// upon boundary list edits
			//

			//
			// Intersection relations flags
			//

			/** 
			* The number of intersection points between this 
			* boundary_feature node and its previous neighbor 
			*/
			int m_num_intersections_with_prev;

			/** 
			* The number of intersection points between this
			* boundary_feature node and its next neighbor
			*/
			int m_num_intersections_with_next;

			//
			// relations with Prev
			//

			/** 
			* use the head segment of this feature resulting 
			* from an intersection with the previous feature 
			*/
			bool m_use_head_from_intersect_prev;

			/** 
			* use the tail segment of this feature resulting 
			* from an intersection with the previous feature 
			*/
			bool m_use_tail_from_intersect_prev;

			/** 
			* use the head segment of this feature resulting 
			* from an intersection with the next feature 
			*/
			bool m_use_head_from_intersect_next;

			/** 
			* use the tail segment of this feature resulting 
			* from an intersection with the next feature 
			*/
			bool m_use_tail_from_intersect_next;


			/** 
			* Constructor 
			*/
			BoundaryFeature( 
				GPlatesModel::FeatureHandle &feature_handle,
				std::string feature_id,
				GPlatesGlobal::FeatureTypes feature_type,
				std::list<GPlatesMaths::PointOnSphere> vertex_list,
				GPlatesMaths::PointOnSphere click_point,
				GPlatesMaths::real_t closeness,
				bool use_reverse = false,
				int num_i_prev = 0,
				int num_i_next = 0,
				bool use_head_prev = false,
				bool use_tail_prev = false,
				bool use_head_next = false,
				bool use_tail_next = false) 
				:
				m_feature( feature_handle ),
				m_feature_id( feature_id ),
				m_feature_type ( feature_type ),
				m_vertex_list ( vertex_list ),
				m_click_point ( click_point ),
				m_closeness ( closeness ),
				m_use_reverse ( use_reverse ),
				m_num_intersections_with_prev ( num_i_prev ),
				m_num_intersections_with_next ( num_i_next ),
				m_use_head_from_intersect_prev ( use_head_prev ),
				m_use_tail_from_intersect_prev ( use_tail_prev ),
				m_use_head_from_intersect_next ( use_head_next ),
				m_use_tail_from_intersect_next ( use_tail_next )
			{ }

			// FIX : this is just a mimic from Layout::CloseDatum
			// FIX : not sure how BoundaryFeature should be ordered?
			/** 
			* Less than operator for boundary feature.
			*/
			bool operator<( const BoundaryFeature &other) const
			{
				return ( m_closeness.isPreciselyLessThan( other.m_closeness.dval() ) );
			}

		}; // end of struct BoundaryFeature


		/** typedef for list of boundary features */
		typedef std::list< BoundaryFeature > BoundaryFeatureList_type;


		/** simple struct to hold partial boundary data */
		struct SubductionBoundaryFeature
		{
			std::string m_feature_id;
			std::string m_feature_tag;
			std::list<GPlatesMaths::PointOnSphere> m_vertex_list;

			SubductionBoundaryFeature(
				std::string id,
				std::string tag,
				std::list<GPlatesMaths::PointOnSphere> list)
				:
				m_feature_id( id ),
				m_feature_tag( tag ),
				m_vertex_list ( list )
			{ }
		};

		/** typedef for list of boundary features */
		typedef std::list< SubductionBoundaryFeature > 	SubductionBoundaryFeatureList_t;


		/** simple enum to identify neighbor relation */
		enum NeighborRelation
		{
			NONE, INTERSECT_PREV, INTERSECT_NEXT, OVERLAP_PREV, OVERLAP_NEXT, OTHER
		};


		//
		// the TopologyResolver class
		//

		// This is a mimic of ReconstructedFeatureGeometryPopulator()

		explicit
		TopologyResolver(
				const double &recon_time,
				unsigned long root_plate_id,
				GPlatesModel::Reconstruction &recon,
				GPlatesModel::ReconstructionTree &recon_tree,
				GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder &finder,
				reconstruction_geometries_type &reconstructed_geometries,
				bool should_keep_features_without_recon_plate_id = true);

		virtual
		~TopologyResolver() 
		{  }

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



#if 0
		virtual
		void
		visit_enumeration(
				GPlatesPropertyValues::Enumeration &enumeration);

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
		visit_gml_time_instant(
				GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

#endif

		virtual
		void
		visit_gml_time_period(
				GPlatesPropertyValues::GmlTimePeriod &gml_time_period);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

#if 0

		virtual
		void
		visit_gpml_feature_reference(
				GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference);

		virtual
		void
		visit_gpml_feature_snapshot_reference(
				GPlatesPropertyValues::GpmlFeatureSnapshotReference &gpml_feature_snapshot_reference);

		virtual
		void
		visit_gpml_finite_rotation(
				GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation);

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp);

		virtual
		void
		visit_hot_spot_trail_mark(
				GPlatesPropertyValues::GpmlHotSpotTrailMark &gpml_hot_spot_trail_mark);

		virtual
		void
		visit_gpml_irregular_sampling(
				GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling);

		virtual
		void
		visit_gpml_key_value_dictionary(
				GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary);

		virtual
		void
		visit_gpml_measure(
				GPlatesPropertyValues::GpmlMeasure &gpml_measure);

		virtual
		void
		visit_gpml_old_plates_header(
				GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header);
#endif

		virtual
		void
		visit_gpml_piecewise_aggregation(
				GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation);

		virtual
		void
		visit_gpml_plate_id(
				GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

#if 0
		virtual
		void
		visit_gpml_polarity_chron_id(
				GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id);
#endif

		virtual
		void
		visit_gpml_property_delegate(
				GPlatesPropertyValues::GpmlPropertyDelegate &gpml_property_delegate);

#if 0
		virtual
		void
		visit_gpml_revision_id(
				GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id);

		void
		write_gpml_time_sample(
				GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample);

		void
		write_gpml_time_window(
				GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window);
#endif

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
		visit_gpml_topological_intersection(
				GPlatesPropertyValues::GpmlTopologicalIntersection &gpml_toplogical_intersection);


#if 0

		virtual
		void
		visit_uninterpreted_property_value(
				GPlatesPropertyValues::UninterpretedPropertyValue &uninterpreted_prop_val);

		virtual
		void
		visit_xs_boolean(
				GPlatesPropertyValues::XsBoolean &xs_boolean);

		virtual
		void
		visit_xs_double(
				GPlatesPropertyValues::XsDouble &xs_double);
		
		virtual
		void
		visit_xs_integer(
				GPlatesPropertyValues::XsInteger &xs_integer);

		virtual
		void
		visit_xs_string(
				GPlatesPropertyValues::XsString &xs_string);
#endif

		//
		// from GPlates 8
		//
		/** Test state of boundary vertex list */
		bool
		vertex_list_empty() const
		{
			return m_vertex_list.empty();
		}

		/** Test state of boundary list */
		bool
		boundary_empty()
		{
			return m_boundary_list.empty();
		}

		/** Get the number of boundary features */
		BoundaryFeatureList_type::size_type 
		boundary_list_size()
		{
			return m_boundary_list.size();
		}

			/** Get the total number of boundary vertices */
			std::list< GPlatesMaths::PointOnSphere >::size_type 
			boundary_vertex_size()
			{
				return m_vertex_list.size();
			}

			/** Get the list */
			BoundaryFeatureList_type
			get_boundary_list()
			{
				return m_boundary_list;
			}

			/** Test if a feature is on the list */
			bool is_feature_id_in_boundary( std::string fid);

			//
			// Iterator creation operations
			//

			/** Iterator to the boundary list */
			BoundaryFeatureList_type::iterator
			boundary_begin()
			{
				return m_boundary_list.begin();
			}

			/** Iterator to the boundary list */
			BoundaryFeatureList_type::iterator
			boundary_end()
			{
				return m_boundary_list.end();
			}


			//
			// Insert and remove operations
			//

			/**
			* Insert BoundaryFeature feature into list at position pos
			*/
			void
			insert( 
				BoundaryFeatureList_type::iterator pos,
				BoundaryFeature feature );

			/**
			* Set the neighbor relations for node1 
			* based upon relation ships with node2
			*/
			void
			set_node_relation(
				NeighborRelation relation,
				BoundaryFeature &node1,
				BoundaryFeature &node2 );

			/**
			* Insert BoundaryFeature feature into list at end 
			*/
			void
			push_back( BoundaryFeature feature )
			{
				m_boundary_list.push_back( feature );
			}

			/** 
			* Erase a feature from the boundary list
			*/
			BoundaryFeatureList_type::iterator
			erase( BoundaryFeatureList_type::iterator pos )
			{
				return m_boundary_list.erase( pos );
			}

 			/** 
			* Erase a boundary feature from the list
			*/
			void
			erase( std::string fid );

			//
			// Modifying operations
			//

			/** update the specified feature */
			bool update_feature( std::string fid );

			/** reselect the specified feature */
			bool reselect_feature(BoundaryFeature feature);

			/** insert the specified feature before the feature fid */
			bool insert_feature(
				BoundaryFeature feature, 
				std::string fid);

			/** remove the specified feature */
			bool remove_feature(BoundaryFeature feature);

			//
			// Traversal operations
			//

			/** 
			* Return a vertex list (list of PointOnSphere 's) 
			* generated by iterating over the boundary features, 
			* from posistion pos1 to pos2,
			* applying each node's neighbor relations.
			*/
			std::list<GPlatesMaths::PointOnSphere>
			get_vertex_list(
				BoundaryFeatureList_type::iterator pos1,
				BoundaryFeatureList_type::iterator pos2
			);

			/**
			* get_vertex_list_from_node_relation 
			* relation is the specific relation to test
			* node 1 is the node to change, and get vertex list from
			* node 2 is a neighbor node on list (will not change)
			* vertex_list is the list to adjust
			*
			* this is a general purpose function
			* that tests a boundary feature node's flags
			* and adjusts it's vertex list as needed
			*
			*/
			void
			get_vertex_list_from_node_relation(
				NeighborRelation relation,
				BoundaryFeature &node1,
				BoundaryFeature &node2,
				std::list<GPlatesMaths::PointOnSphere> &vertex_list );

		
			/**
			* This function is used to see if some point P is located
			* inside, outside, or on the boundary of the
			* plate polygon.
			* Returns the following values:
			*  0:  P is outside
			*  1:  P is inside 
			*  2:  P is on the boundary 
			*/
			int
			is_point_in_on_out ( 
				const GPlatesMaths::PointOnSphere &test_point ) const;

			int
			is_point_in_on_out_counter (
				const GPlatesMaths::PointOnSphere &test_point,
				int &count_north,
				int &count_south) const;

			/**
			 * Restricted Enumerative access to the vertices
			 */
			std::list<GPlatesMaths::PointOnSphere>::const_iterator
			VertexBegin() const { return m_vertex_list.begin(); }

			std::list<GPlatesMaths::PointOnSphere>::const_iterator
			VertexEnd() const { return m_vertex_list.end(); }

		// from GPlates 8
		void
		parse_boundary_string(
				GPlatesModel::FeatureHandle &feature_handle);

		void
		resolve_boundary();

		void 
		compute_bounds();

		void
		report();

	private:
		

		//
		// from ReconstructedFeatureGeometryPopulator.h
		//
		const GPlatesPropertyValues::GeoTimeInstant d_recon_time;
		GPlatesModel::integer_plate_id_type d_root_plate_id;
		GPlatesModel::Reconstruction *d_recon_ptr;
		GPlatesModel::ReconstructionTree *d_recon_tree_ptr;
		GPlatesFeatureVisitors::ReconstructedFeatureGeometryFinder *d_recon_finder_ptr;

		reconstruction_geometries_type *d_reconstruction_geometries_to_populate;
		boost::optional<ReconstructedFeatureGeometryAccumulator> d_accumulator;
		bool d_should_keep_features_without_recon_plate_id;

		// This constructor should never be defined, because we don't want to allow
		// copy-construction.
		TopologyResolver(
				const TopologyResolver &);

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		TopologyResolver &
		operator=(
			const TopologyResolver &);

		//
		// FROM GPlates 8
		//

		/**
		* list of boundary feature nodes
		*/
		BoundaryFeatureList_type m_boundary_list;

		/** List of subduction features */
		SubductionBoundaryFeatureList_t m_subduction_list;
		SubductionBoundaryFeatureList_t m_subduction_sR_list;
		SubductionBoundaryFeatureList_t m_subduction_sL_list;
		SubductionBoundaryFeatureList_t m_ridge_transform_list;

		/** List of mesh points on this plate */
		std::list<GPlatesMaths::PointOnSphere> m_mesh_on_plate_list;

		/** list of coordinate data for mesh on plate frame */
		std::list<std::string> m_mesh_on_plate_frame;

		/* List of vertices */
		std::list<GPlatesMaths::PointOnSphere> m_vertex_list;


		/**
		 * Polar polygon status:
		 * 0 = no pole in polygon; 1 = North Pole; -1 = South Pole in polygon
		*/
		int m_pole;

		/** geographic bounds */
		double m_max_lat;
		double m_max_lon;
		double m_min_lat;
		double m_min_lon;

		/** the number of features visited by this visitor */
		int d_num_features;
		int d_num_topologies;
	};

}

#endif  // GPLATES_FEATUREVISITORS_TOPOLOGY_RESOLVER_H
