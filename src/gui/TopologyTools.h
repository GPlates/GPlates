/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
 * Copyright (C) 2008 , 2009 California Institute of Technology 
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

#include <cstddef>
#include <utility>
#include <vector>

#include "global/types.h"

#include "TopologySectionsContainer.h"

#include "feature-visitors/TopologySectionsFinder.h"

#include "maths/GeometryOnSphere.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/ModelInterface.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"

#include "model/PropertyValue.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalIntersection.h"

#include "utils/GeometryCreationUtils.h"
#include "utils/UnicodeStringUtils.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesAppLogic
{
	class Reconstruct;
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
		 * Set the click point (called from canvas tool)
		 */
		void
		set_click_point( double lat, double lon );


		/**
		 * Get a weak_ref on the topology feature 
		 */
		GPlatesModel::FeatureHandle::weak_ref
		get_topology_feature_ref()
		{
			return d_topology_feature_ref;
		}


		int
		get_number_of_sections()
		{
			return d_topology_sections_container_ptr->size();
		}


	public slots:
		
		void
		handle_reconstruction();

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
		handle_shift_left_click(
			const GPlatesMaths::PointOnSphere &click_pos_on_globe,
			const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
			bool is_on_globe);

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
		handle_add_feature();

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
		// Slots for signals from TopologySectionsContainer
		//

		void
		react_cleared();

		void
		react_insertion_point_moved(
			GPlatesGui::TopologySectionsContainer::size_type new_index);
		
		void
		react_entry_removed(
			GPlatesGui::TopologySectionsContainer::size_type deleted_index);
	
		void
		react_entries_inserted(
			GPlatesGui::TopologySectionsContainer::size_type inserted_index,
			GPlatesGui::TopologySectionsContainer::size_type quantity,
			GPlatesGui::TopologySectionsContainer::const_iterator inserted_begin,
			GPlatesGui::TopologySectionsContainer::const_iterator inserted_end);

		void
		react_entry_modified(
			GPlatesGui::TopologySectionsContainer::size_type modified_index);

	private:
		/**
		 * Keeps track of topological section data.
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
			 * Resets all variables except the TableRow in @a d_table_row.
			 */
			void
			reset();

			/**
			 * Initialises those data members that deal with reconstructions.
			 */
			void
			reconstruct_section_info_from_table_row(
					GPlatesModel::Reconstruction &reconstruction);


			/**
			 * Keep a copy of the @a TopologySectionsContainer table row.
			 * We will update this each time @a TopologySectionsContainer signals
			 * us that the table row has been modified - so it will always be in sync.
			 */
			GPlatesGui::TopologySectionsContainer::TableRow d_table_row;

			/**
			 * The unclipped, un-intersected section geometry - this is the full geometry
			 * of the RFG - it is not the subsegment geometry.
			 *
			 * NOTE: This does not take into account the reverse flag for this section.
			 */
			boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
					d_section_geometry_unreversed;

			/**
			 * The potentially clipped section geometry - the part of geometry of the topological
			 * section that represents the resolved topological polygon boundary.
			 *
			 * NOTE: This does not take into account the reverse flag for this section.
			 * It is designed this way to make the polyline clipping faster (which accounts
			 * for most of the CPU for resolving topology polygons).
			 */
			boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
					d_subsegment_geometry_unreversed;

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

			/**
			 * This is the 'd_click_point' in 'd_table_row' at the current reconstruction time.
			 */
			boost::optional<GPlatesMaths::PointOnSphere> d_reconstructed_click_point;

			//! The optional intersection point with previous section.
			boost::optional<GPlatesMaths::PointOnSphere> d_intersection_point_with_prev;

			//! The optional intersection point with next section.
			boost::optional<GPlatesMaths::PointOnSphere> d_intersection_point_with_next;
		};

		//! Typedef for a sequence of @a SectionInfo objects.
		typedef std::vector<SectionInfo> section_info_seq_type;


		/**
		 * Keeps track of the user's click point, the feature that was clicked and
		 * reconstructions needed to rotate the click point with the feature.
		 */
		class ClickPoint
		{
		public:
			void
			set_focus(
					const GPlatesModel::FeatureHandle::weak_ref &focused_feature_ref,
					const GPlatesMaths::PointOnSphere &reconstructed_click_point,
					GPlatesModel::ReconstructionTree &reconstruction_tree)
			{
				d_clicked_feature_ref = focused_feature_ref;
				d_reconstructed_click_point = reconstructed_click_point;
				d_present_day_click_point = boost::none;
				// Calculate present day click point from reconstructed click point.
				calc_present_day_click_point(reconstruction_tree);
			}

			void
			unset_focus()
			{
				d_clicked_feature_ref = GPlatesModel::FeatureHandle::weak_ref();
				d_present_day_click_point = boost::none;
				d_reconstructed_click_point = boost::none;
			}

			/**
			 * Recalculates 'd_reconstructed_click_point' when the reconstruction time
			 * changes - uses the 'reconstructionPlateId' in the clicked feature.
			 */
			void
			update_reconstructed_click_point(
					GPlatesModel::ReconstructionTree &reconstruction_tree);

			GPlatesModel::FeatureHandle::weak_ref d_clicked_feature_ref;
			boost::optional<GPlatesMaths::PointOnSphere> d_present_day_click_point;
			boost::optional<GPlatesMaths::PointOnSphere> d_reconstructed_click_point;

		private:
			//! Calculates the present day click point from the reconstructed click point.
			void
			calc_present_day_click_point(
					GPlatesModel::ReconstructionTree &reconstruction_tree);
		};


		/**
		 * Used to draw rendered geometries.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection;

		/**
		 * Rendered geometry layers to draw into 
		 */
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_topology_geometry_layer_ptr,
			d_focused_feature_layer_ptr,
			d_insertion_neighbors_layer_ptr,
			d_segments_layer_ptr,
			d_end_points_layer_ptr,
			d_intersection_points_layer_ptr,
			d_click_point_layer_ptr,
			d_click_points_layer_ptr;

		/**
		 * This is our reference to the Feature Focus, which we use to let the rest of the
		 * application know what the user just clicked on.
		 */
		GPlatesGui::FeatureFocus *d_feature_focus_ptr;

		/**
		 * Used to query the reconstruction.
		 */
		GPlatesAppLogic::Reconstruct *d_reconstruct_ptr;

		/**
		 * The View State is used to access the digitisation layer in the globe in the
		 * globe canvas.
		 */
		GPlatesQtWidgets::ViewportWindow *d_viewport_window_ptr;

		/*
		* pointer to the TopologySectionsContainer in ViewportWindow.
		*/
		GPlatesGui::TopologySectionsContainer *d_topology_sections_container_ptr;

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

		//! Keeps track of the click point and the feature clicked on.
		ClickPoint d_click_point;

		/**
		 * Contains information about the sections in the topology.
		 *
		 * This sequence matches the table row sequence in @a TopologySectionsContainer
		 * and is updated whenever that is updated (by listening to its insert, etc signals).
		 */
		section_info_seq_type d_section_info_seq;

		/**
		 * An ordered collection of all the vertices in the topology.
		 */
		std::vector<GPlatesMaths::PointOnSphere> d_topology_vertices;

		/**
		 * The 'd_topology_vertices' get processed into this geometry.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> 
			d_topology_geometry_opt_ptr;

		/*
		 * The topology feature being edited (if using edit tool) or NULL (if using
		 * the build tool) until the topology feature is created in the create feature dialog.
		 */ 
		GPlatesModel::FeatureHandle::weak_ref d_topology_feature_ref;


		//
		// private functions 
		//

		void
		connect_to_focus_signals( bool state );

		void
		connect_to_topology_sections_container_signals( bool state );

		void
		activate_build_mode();

		void
		activate_edit_mode();

		void
		display_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
			const GPlatesModel::FeatureHandle::properties_iterator &properties_iter);

		/**
		 * Initialise the topology from the currently focused feature.
		 */
		void
		initialise_focused_topology();

		/**
		 * Updates the topology from modified sections and
		 * redraws the screen.
		 */
		void
		update_and_redraw_topology(
				const section_info_seq_type::size_type first_modified_section_index,
				const section_info_seq_type::size_type num_sections);

		/**
		 * Processes intersection between two adjacent sections.
		 *
		 * NOTE: The second section must follow the first section.
		 */
		void
		process_intersection(
				const section_info_seq_type::size_type first_section_index,
				const section_info_seq_type::size_type second_section_index,
				bool first_section_already_clipped,
				bool second_section_already_clipped);

		//! Returns the index of the previous section.
		section_info_seq_type::size_type
		get_prev_section_index(
				section_info_seq_type::size_type section_index) const;

		//! Returns the index of the next section.
		section_info_seq_type::size_type
		get_next_section_index(
				section_info_seq_type::size_type section_index) const;

		/**
		 * Find the matching topological section given a feature reference and
		 * a geometry feature property iterator.
		 */
		int
		find_topological_section_index(
				const GPlatesModel::FeatureHandle::weak_ref &feature,
				const GPlatesModel::FeatureHandle::properties_iterator &properties_iter);

		void
		create_child_rendered_layers();

		void
		show_numbers();

		/**
		 * Gathers the points of all topology subsegments to be used for rendering.
		 */
		void
		update_topology_vertices();

		/**
		 * Returns true if the topological section at index @a section_index should
		 * be reversed.
		 */
		bool
		should_reverse_section(
				const section_info_seq_type::size_type section_index);

		/**
		 * Creates the 'gpml:boundary' property value from the current topology state
		 * and sets in on @a feature.
		 */
		void
		convert_topology_to_boundary_feature_property(
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
		draw_focused_geometry();

		void 
		draw_focused_geometry_end_points(
				const GPlatesMaths::PointOnSphere &start_point,
				const GPlatesMaths::PointOnSphere &end_point);

		void
		draw_segments();

		void
		draw_end_points();

		void
		draw_intersection_points();

		void
		draw_insertion_neighbors();

		void
		draw_click_points();

		void
		draw_click_point();
	};
}

#endif  // GPLATES_GUI_TOPOLOGY_TOOLS_H
