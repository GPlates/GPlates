/**
 * \file 
 * $Revision$
 * $Date$ 
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
 
#ifndef GPLATES_GUI_TOPOLOGY_TOOLS_H
#define GPLATES_GUI_TOPOLOGY_TOOLS_H

#include <QDebug>
#include <QObject>

#include <boost/optional.hpp>
#include <boost/none.hpp>

#include <cstddef> // For std::size_t
#include <utility>
#include <vector>

#include "global/types.h"

#include "TopologySectionsContainer.h"

#include "app-logic/Layer.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/TopologyBoundaryIntersections.h"

#include "feature-visitors/TopologySectionsFinder.h"

#include "maths/GeometryOnSphere.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/ModelInterface.h"

#include "model/PropertyValue.h"
#include "property-values/GpmlTopologicalInterior.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalLineSection.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/UnicodeStringUtils.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class CreateFeatureDialog;
	class ViewportWindow;
	class TopologyToolsWidget;
}

namespace GPlatesGui
{
	class ChooseCanvasTool;
	class FeatureFocus;


	class TopologyTools:
			public QObject, 
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
		Q_OBJECT

	public:

		/** What mode the tools were started in ; NOTE this can change during tool use */
		enum CanvasToolMode
		{
			BUILD, EDIT
		};

		/** Constructor */
		TopologyTools(
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ViewportWindow &viewport_window,
				GPlatesGui::ChooseCanvasTool &choose_canvas_tool);

		void
		activate(
				CanvasToolMode);

		void
		deactivate();


		/**
		 * Get a weak_ref on the topology feature 
		 */
		GPlatesModel::FeatureHandle::weak_ref
		get_topology_feature_ref()
		{
			return d_topology_feature_ref;
		}


		//! get the number of boundary sections
		int
		get_number_of_sections_boundary();

		//! get the number of interior sections
		int
		get_number_of_sections_interior();

		//! synchronize tools and widget
		// NOTE : will call handle_sections_combobox_index_changed to change container pointer
		void
		synchronize_seq_num(int i);

		//! called from the topology tools widget when the user or program changes the combo box
		void
		handle_sections_combobox_index_changed( int i );

	public slots:
		
		void
		handle_reconstruction();

		void
		unset_focus();
		
		void
		set_focus(
			GPlatesGui::FeatureFocus &feature_focus);

		void
		display_feature_focus_modified(
			GPlatesGui::FeatureFocus &feature_focus);

		//
		// Slots for signals from TopologyToolsWidget or its FeatureCreationDialog
		//
		void
		handle_create_new_feature(
			GPlatesModel::FeatureHandle::weak_ref);


		/**
		 * The slot that gets called when the user clicks "Add Focused Feature".
		 *
		 * This will insert the focused feature at the location displayed in the
		 * topological sections tab of the feature table.
		 *
		 * The topology feature being edited is not modified yet though - to do that
		 * call @a handle_apply.
		 */
		void
		handle_add_feature_boundary();

		//! same as above, but for the <gpml:interior> list of features
		void
		handle_add_feature_interior();

		/**
		 * The slot that gets called when the user clicks "Remove Focused Feature".
		 *
		 * This will remove the focused feature 
		 *
		 * The topology feature being edited is not modified yet though - to do that
		 * call @a handle_apply.
		 */
		void
		handle_remove_feature();

		/**
		 * The slot that gets called when the user clicks "Remove All Features".
		 *
		 * The topology feature being edited is not modified yet though - to do that
		 * call @a handle_apply.
		 */
		void
		handle_remove_all_sections();

		/**
		 * The slot that gets called when the user clicks "Apply".
		 *
		 * This stores any edits made so far to the topology feature (whether
		 * that's a newly created feature from the create feature dialog when using
		 * build tool or an existing topology feature when using the edit tool).
		 */
		void
		handle_apply();

		//
		// Slots for signals from either TopologySectionsContainer
		//
		void
		react_insertion_point_moved(
			GPlatesGui::TopologySectionsContainer::size_type new_index);

		//
		// Slots for signals from boundary TopologySectionsContainer
		//

		//! boundary react functions
		void
		react_cleared_boundary();

		void
		react_entry_removed_boundary(
			GPlatesGui::TopologySectionsContainer::size_type deleted_index);
	
		void
		react_entries_inserted_boundary(
			GPlatesGui::TopologySectionsContainer::size_type inserted_index,
			GPlatesGui::TopologySectionsContainer::size_type quantity,
			GPlatesGui::TopologySectionsContainer::const_iterator inserted_begin,
			GPlatesGui::TopologySectionsContainer::const_iterator inserted_end);

		void
		react_entry_modified_boundary(
			GPlatesGui::TopologySectionsContainer::size_type modified_index);

		//! Interior react functions
		void
		react_cleared_interior();

		void
		react_entry_removed_interior(
			GPlatesGui::TopologySectionsContainer::size_type deleted_index);
	
		void
		react_entries_inserted_interior(
			GPlatesGui::TopologySectionsContainer::size_type inserted_index,
			GPlatesGui::TopologySectionsContainer::size_type quantity,
			GPlatesGui::TopologySectionsContainer::const_iterator inserted_begin,
			GPlatesGui::TopologySectionsContainer::const_iterator inserted_end);

		void
		react_entry_modified_interior(
			GPlatesGui::TopologySectionsContainer::size_type modified_index);

	private:
		/**
		 * Keeps track of geometry/intersection/rendering information for all currently visible sections.
		 */
		class VisibleSection
		{
		public:
			VisibleSection(
					const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &
							section_geometry_unreversed,
					const std::size_t section_info_index);

			/**
			 * Index of the @a SectionInfo that 'this' object was created from.
			 */
			std::size_t d_section_info_index;

			/**
			 * The unclipped, un-intersected section geometry - this is the full geometry
			 * of the RFG - it is not the subsegment geometry.
			 *
			 * NOTE: This does not take into account the reverse flag for this section.
			 */
			boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
					d_section_geometry_unreversed;

			/**
			 * The final possibly clipped boundary segment geometry - the part of geometry of
			 * the topological section that represents the resolved topological polygon boundary.
			 *
			 * This is empty until it this section been tested against both its
			 * neighbours and the appropriate possibly clipped subsegment is chosen
			 * to be part of the plate polygon boundary.
			 *
			 * NOTE: This does not take into account the reverse flag for this section.
			 * It is designed this way to make the polyline clipping faster (which accounts
			 * for most of the CPU for resolving topology polygons).
			 */
			boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
					d_final_boundary_segment_unreversed_geom;

			/**
			 * The start of the section (takes into account whether section is reversed).
			 * It's optional in case we can't find the section's geometry.
			 *
			 * NOTE: This is not the start of the (clipped) subsegment - it is the
			 * start of the unclipped section geometry.
			 */
			boost::optional<GPlatesMaths::PointOnSphere> d_section_start_point;

			/**
			 * The end of the section (takes into account whether section is reversed).
			 * It's optional in case we can't find the section's geometry.
			 *
			 * NOTE: This is not the end of the (clipped) subsegment - it is the
			 * end of the unclipped section geometry.
			 */
			boost::optional<GPlatesMaths::PointOnSphere> d_section_end_point;

			//! The optional intersection point with previous section.
			boost::optional<GPlatesMaths::PointOnSphere> d_intersection_point_with_prev;

			//! The optional intersection point with next section.
			boost::optional<GPlatesMaths::PointOnSphere> d_intersection_point_with_next;

			/**
			 * Keeps track of temporary results from intersections of this section
			 * with its neighbours.
			 */
			GPlatesAppLogic::TopologicalBoundaryIntersections d_intersection_results;
		};

		//! Typedef for a sequence of @a VisibleSection objects.
		typedef std::vector<VisibleSection> visible_section_seq_type;


		/**
		 * Keeps track of topological section data for all current sections even if they
		 * are not currently visible because their age range does not contain the current
		 * reconstruction time.
		 *
		 * The information stored here is extra information not contained in the table rows
		 * of @a TopologySectionsContainer and is information specific to us.
		 * It's information that other clients of @a TopologySectionsContainer don't need
		 * to know about and hence don't need to be notified
		 * (via @a TopologySectionsContainer signals) whenever it's modified.
		 */
		class SectionInfo
		{
		public:
			explicit
			SectionInfo(
					const GPlatesGui::TopologySectionsContainer::TableRow &table_row) :
				d_table_row(table_row)
			{  }


			/**
			 * Reconstruct this section and return a data structure to be used
			 * for intersection processing and rendering.
			 *
			 * Returns false if no RFG could be found that was reconstructed using
			 * @a reconstruction_tree meaning the current section's age range does
			 * not contain the current reconstruction time or that there is no
			 * reconstruction tree to match.
			 */
			boost::optional<VisibleSection>
			reconstruct_section_info_from_table_row(
					std::size_t section_index,
					const GPlatesAppLogic::ReconstructionTree &reconstruction_tree) const;

			/**
			 * Keep a copy of the @a TopologySectionsContainer table row.
			 * We will update this each time @a TopologySectionsContainer signals
			 * us that the table row has been modified - so it will always be in sync.
			 */
			GPlatesGui::TopologySectionsContainer::TableRow d_table_row;
		};

		//! Typedef for a sequence of @a SectionInfo objects.
		typedef std::vector<SectionInfo> section_info_seq_type;


		/**
		 * Used to draw rendered geometries.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection;

		/**
		 * Rendered geometry layers to draw into 
		 */
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_topology_geometry_layer_ptr,
			d_boundary_geometry_layer_ptr,
			d_interior_geometry_layer_ptr,
			d_focused_feature_layer_ptr,
			d_insertion_neighbors_layer_ptr,
			d_segments_layer_ptr,
			d_end_points_layer_ptr,
			d_intersection_points_layer_ptr;

		/**
		 * This is our reference to the Feature Focus, which we use to let the rest of the
		 * application know what the user just clicked on.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;

		/**
		 * Used to query the reconstruction.
		 */
		GPlatesAppLogic::ApplicationState *d_application_state_ptr;

		/**
		 * The View State is used to access the digitisation layer in the globe in the
		 * globe canvas.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

		/*
		* pointer to the TopologySectionsContainer in ViewportWindow.
		*/
		GPlatesGui::TopologySectionsContainer *d_boundary_sections_container_ptr;
		GPlatesGui::TopologySectionsContainer *d_interior_sections_container_ptr;

		/**
		* pointer to the TopologyToolsWidget in Task Panel
		*/
		GPlatesQtWidgets::TopologyToolsWidget *d_topology_tools_widget_ptr;

		/**
		 * To change the canvas tool when we are finished editing/building topology.
		 */
		GPlatesGui::ChooseCanvasTool *d_choose_canvas_tool;


		/** Keeps track of what canvas tool mode activated this object */
		CanvasToolMode d_mode;	


		QString d_warning;

		// These control the behavior of the geom. visitors
		bool d_is_active;
		bool d_in_edit;

		// This gets set upon activation 
		GPlatesGlobal::TopologyTypes d_topology_type;

		/**
		 * Contains information about the sections in the topology.
		 *
		 * This sequence matches the table row sequence in @a TopologySectionsContainer
		 * and is updated whenever that is updated (by listening to its insert, etc signals).
		 */
		// section_info_seq_type d_section_info_seq;
		section_info_seq_type d_boundary_section_info_seq;
		section_info_seq_type d_interior_section_info_seq;

		/**
		 * Contains the intersection/rendering information of the current visible sections.
		 */
		visible_section_seq_type d_visible_boundary_section_seq;
		visible_section_seq_type d_visible_interior_section_seq;

		/**
		 * An ordered collection of ALL the vertices in the topology.
		 */
		std::vector<GPlatesMaths::PointOnSphere> d_topology_vertices;

		/**
		 * The 'd_topology_vertices' get processed into this geometry.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> 
			d_topology_geometry_opt_ptr;

		/**
		 * The boundary polygon gets processed into this geometry.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> 
			d_boundary_geometry_opt_ptr;

		/**
		 * The interior geometires processed into this geometry.
		 */
		std::vector<boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> >
			d_interior_geometry_opt_ptrs;

		/*
		 * The topology feature being edited (if using edit tool) or NULL (if using
		 * the build tool) until the topology feature is created in the create feature dialog.
		 */ 
		GPlatesModel::FeatureHandle::weak_ref d_topology_feature_ref;

		// shape bound : 0.125 is the default shape bound. It corresponds to about 20.6 degree.
		double d_shape_factor;
		// max edge is the longest edge length
		double d_max_edge;

		// which sequence of sections is under going edit and display 
		int d_seq_num;

		//
		// private functions 
		//

		void
		connect_to_focus_signals( bool state );

		void
		connect_to_boundary_sections_container_signals( bool state );

		void
		connect_to_interior_sections_container_signals( bool state );

		void
		activate_build_mode();

		void
		activate_edit_mode();

		void
		display_focused_feature();

		/**
		 * Initialise the topology from the currently focused feature.
		 */
		void
		initialise_focused_topology();

		/**
		 * Clears all sections from the topology if any sections are unloaded
		 * (ie, if any features referenced by the topology are deleted).
		 */
		void
		handle_unloaded_sections();

		/**
		 * Updates the topology and redraws the screen.
		 */
		void
		update_and_redraw_topology();

		/**
		 * Returns true if in build mode (ie, topology not yet created so assumed
		 * exists for all time) or if in edit mode and topology feature exists at
		 * the current reconstruction time.
		 */
		bool
		does_topology_exist_at_current_recon_time() const;

		//! Reconstruct all sections.
		void
		reconstruct_boundary_sections();

		void
		reconstruct_interior_sections();

		//! Process all intersections.
		void
		process_intersections();

		//! Special case intersection processing when there are exactly two sections.
		void
		process_two_section_intersections();

		//! Find section reversal flags to minimise any rubber banding distance.
		void
		determine_boundary_segment_reversals();

		/**
		 * Find any contiguous sequences of sections that require rubber-banding
		 * (ie, don't intersect each other) and that are anchored at the start and end
		 * of each sequence by an intersection.
		 */
		const std::vector< std::vector<section_info_seq_type::size_type> > 
		find_reverse_section_subsets();

		/**
		 * Find section reversal flags to minimise any rubber banding distance for a
		 * subset of contiguous sections that are anchored either end by an intersection.
		 */
		std::vector<bool>
		find_flip_reverse_order_flags(
				const std::vector<visible_section_seq_type::size_type> &reverse_section_subset);

		/**
		 * Recursive function to find section reversal flags that minimise rubber banding distance.
		 */
		void
		find_reverse_order_subset_to_minimize_rubber_banding(
				double &length_with_current_flipped,
				double &length_with_current_not_flipped,
				std::vector<bool> &reverse_order_seq_with_current_flipped,
				std::vector<bool> &reverse_order_seq_with_current_not_flipped,
				const std::vector<visible_section_seq_type::size_type> &reverse_section_subset,
				const GPlatesMaths::PointOnSphere &start_section_head,
				const GPlatesMaths::PointOnSphere &current_section_flipped_geometry_tail,
				const GPlatesMaths::PointOnSphere &current_section_not_flipped_geometry_tail,
				visible_section_seq_type::size_type current_depth,
				visible_section_seq_type::size_type total_depth);

		void
		choose_child_reverse_order_subset(
				double &length,
				std::vector<bool> &reverse_order_seq,
				const double &length_with_next_flipped,
				const double &length_with_next_not_flipped,
				const std::vector<bool> &reverse_order_seq_with_next_flipped,
				const std::vector<bool> &reverse_order_seq_with_next_not_flipped,
				const GPlatesMaths::PointOnSphere &current_section_geometry_tail,
				const GPlatesMaths::PointOnSphere &next_section_flipped_geometry_head,
				const GPlatesMaths::PointOnSphere &next_section_not_flipped_geometry_head);

		//! Assign the boundary subsegments.
		void
		assign_boundary_segments();

		//! Assign the interior subsegments.
		void
		assign_interior_segments();

		/**
		 * Processes intersection between two adjacent visible sections.
		 *
		 * NOTE: The second section must follow the first section.
		 */
		void
		process_intersection(
				const visible_section_seq_type::size_type first_visible_section_index,
				const visible_section_seq_type::size_type second_visible_section_index);

		void
		assign_boundary_segment(
				const visible_section_seq_type::size_type visible_section_index);
		void
		assign_interior_segment(
				const visible_section_seq_type::size_type visible_section_index);

		const SectionInfo &
		get_boundary_section_info(
				const VisibleSection &visible_section) const;

		SectionInfo &
		get_boundary_section_info(
				const VisibleSection &visible_section);

		const SectionInfo &
		get_interior_section_info(
				const VisibleSection &visible_section) const;

		SectionInfo &
		get_interior_section_info(
				const VisibleSection &visible_section);

		boost::optional<visible_section_seq_type::const_iterator>
		is_section_visible_boundary(
				const section_info_seq_type::size_type section_index) const;

		boost::optional<visible_section_seq_type::const_iterator>
		is_section_visible_interior(
				const section_info_seq_type::size_type section_index) const;


		void
		set_boundary_section_reverse_flag(
				const section_info_seq_type::size_type section_index,
				const bool new_reverse_flag);

		void
		flip_reverse_flag(
				const section_info_seq_type::size_type section_index);

		void
		flip_reverse_flag(
				VisibleSection &visible_section_info);

		std::pair<
				GPlatesMaths::PointOnSphere/*start point*/,
				GPlatesMaths::PointOnSphere/*end point*/>
		get_boundary_geometry_end_points(
				const visible_section_seq_type::size_type visible_section_index,
				const bool flip_reversal_flag);

		/**
		 * Find the matching topological sections given a feature reference and
		 * a geometry feature property iterator.
		 */
		void
		find_topological_section_indices(
				int &seq_num,
				std::vector<int> &indices,
				const GPlatesModel::FeatureHandle::weak_ref &feature,
				const GPlatesModel::FeatureHandle::iterator &properties_iter);
		
		//! Returns true if the focused feature can be added to the topology.
		bool
		can_insert_focused_feature_into_topology();

		void
		create_child_rendered_layers();

		void
		show_numbers();

		/**
		 * Gathers the points of all topology subsegments to be used for rendering.
		 */
		void
		update_topology_vertices();

		void
		update_boundary_vertices();

		void
		update_interior_vertices();

		/**
		 * Creates the 'gpml:boundary' property value from the current topology state
		 * and sets in on @a feature.
		 */
		void
		convert_topology_to_feature_property(
				const GPlatesModel::FeatureHandle::weak_ref &feature);

		/**
		 * Creates a sequence of @a GpmlTopologicalSection reflecting the
		 * current topology state.
		 */
		void
		create_topological_sections(
			std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &
				topological_sections);

		void
		create_boundary_sections(
			std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &
				topological_sections);

		void
		create_interior_sections(
			std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &
				topological_sections);

		void
		clear_widgets_and_data();

		//
		// Rendering methods
		//
		void
		draw_all_layers_clear();

		void
		draw_all_layers();

		void 
		draw_topology_geometry();

		void 
		draw_boundary_geometry();

		void 
		draw_interior_geometry();

		void
		draw_segments();

		void
		draw_intersection_points();

		void
		draw_insertion_neighbors();

#if 0
		void
		draw_end_points();
#endif

		void 
		draw_focused_geometry( bool draw);

		void 
		draw_focused_geometry_end_points(
				const GPlatesMaths::PointOnSphere &start_point,
				const GPlatesMaths::PointOnSphere &end_point);

	};
}

#endif  // GPLATES_GUI_TOPOLOGY_TOOLS_H
