/* $Id$ */

/**
 * \file 
 * Determines which @a GeometryBuilder each geometry operation canvas tool should target.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_GEOMETRYBUILDERTOOLTARGET_H
#define GPLATES_VIEWOPERATIONS_GEOMETRYBUILDERTOOLTARGET_H

#include <boost/any.hpp>
#include <QObject>
#include <QUndoCommand>

#include "gui/ChooseCanvasTool.h"
#include "gui/FeatureFocus.h"


namespace GPlatesViewOperations
{
	class GeometryBuilder;

	/**
	 * Manages which geometry builder tools target which geometry at which times.
	 * Two geometry types currently supported are temporary geometry used
	 * for digitising new geometry and chosen feature geometry selected by
	 * the @a ClickGeometry tool.
	 */
	class GeometryOperationTarget :
			public QObject
	{
		Q_OBJECT

	public:
		/**
		 * Opaque mememto used to get and set internal state.
		 */
		typedef boost::any internal_state_type;

		GeometryOperationTarget(
				GeometryBuilder &digitise_geom_builder,
				GeometryBuilder &focused_feature_geom_builder,
				const GPlatesGui::FeatureFocus &feature_focus,
				const GPlatesGui::ChooseCanvasTool &choose_canvas_tool);

		/**
		 * Which GeometryBuilder should @a current_canvas_tool use.
		 * If caller is a canvas tool then might need to call
		 * 'get_and_set_current_geometry_builder_for_newly_activated_tool()' below.
		 *
		 * NOTE: will return NULL if there's no focused feature geometry or
		 * not enough vertices for the current geometry operation tool depending
		 * on which is being targeted by the current canvas tool.
		 * This shouldn't happen if the tools are disabled appropriately. 
		 */
		GeometryBuilder *
		get_current_geometry_builder() const;

		/**
		 * Which @a GeometryBuilder should @a current_canvas_tool use.
		 *
		 * Note: if caller is a canvas tool then need to call this method instead
		 * of one above. This is because the 'chose_canvas_tool' slot is used to
		 * get the current canvas tool - however the canvas tool that is calling
		 * this method does so while the canvas tool is being switched and the so
		 * 'chose_canvas_tool' slot will get called too late resulting in the
		 * wrong canvas tool used internally to select the geometry builder.
		 *
		 * NOTE: will return NULL if there's no focused feature geometry or
		 * not enough vertices for the current geometry operation tool depending
		 * on which is being targeted by the current canvas tool.
		 * This shouldn't happen if the tools are disabled appropriately. 
		 */
		GeometryBuilder *
		get_and_set_current_geometry_builder_for_newly_activated_tool(
				GPlatesCanvasTools::CanvasToolType::Value current_canvas_tool);

		/**
		 * Returns the @a GeometryBuilder that would be used if we switched to the
		 * canvas tool specified.
		 *
		 * Returns NULL if the canvas tool should not be switched to @a next_canvas_tool
		 * because there are not enough vertices in the geometry.
		 *
		 * This is similar to the above method except the caller is not telling
		 * this object what the current canvas tool is (or what the next one is either
		 * for that matter).
		 */
		GeometryBuilder *
		get_geometry_builder_if_canvas_tool_is_chosen_next(
				GPlatesCanvasTools::CanvasToolType::Value next_canvas_tool) const;

		/**
		 * Returns the @a GeometryBuilder used to digitise/modify temporary new geometry
		 * used for creating a new feature.
		 */
		GeometryBuilder *
		get_digitise_new_geometry_builder() const
		{
			return d_digitise_new_geom_builder;
		}

		/**
		 * Returns the @a GeometryBuilder used to modify geometry of the focused feature.
		 */
		GeometryBuilder *
		get_focused_feature_geometry_builder() const
		{
			return d_focused_feature_geom_builder;
		}

		/**
		 * Returns the current internal state.
		 */
		internal_state_type
		get_internal_state() const;

		/**
		 * Sets the current internal state to be that of @a internal_state.
		 */
		void
		set_internal_state(
				internal_state_type internal_state);

	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		switched_geometry_builder(
				GPlatesViewOperations::GeometryOperationTarget &,
				GPlatesViewOperations::GeometryBuilder *);

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Changed which reconstruction geometry is currently focused.
		 */
		void
		set_focus(
				GPlatesModel::FeatureHandle::weak_ref feature_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry);

		/**
		 * A canvas tool has been chosen.
		 */
		void
		chose_canvas_tool(
				GPlatesGui::ChooseCanvasTool &,
				GPlatesCanvasTools::CanvasToolType::Value);

	private:
		/**
		 * Used to decide which @a GeometryBuilder to target.
		 * This is mainly here because the 'get_geometry_builder_if_canvas_tool_is_chosen_next()'
		 * method shouldn't update the state of @a GeometryOperationTarget so we wrap up
		 * the decision logic in a class so it can get reused.
		 */
		class TargetChooser
		{
		public:
			TargetChooser();

			/**
			 * Type of geometry to target.
			 */
			enum TargetType
			{
				TARGET_NONE,
				TARGET_DIGITISE_NEW_GEOMETRY,
				TARGET_FOCUS_GEOMETRY
			};

			//! Set to true/false if a feature is/is not in focus.
			void
			set_focused_geometry(
					bool is_geometry_in_focus);

			//! Change the canvas tool type.
			void
			change_canvas_tool(
					GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type);

			//! Returns true if should target focused feature geometry.
			TargetType
			get_target_type() const;

		private:
			/**
			 * Is true if there is currently geometry in focus.
			 */
			bool d_is_geometry_in_focus;

			/**
			 * Is the user digitising new geometry.
			 */
			bool d_user_is_digitising_new_geometry;
		};

		/**
		 * Used to build temporary geometry for digitising new geometry.
		 */
		GeometryBuilder *d_digitise_new_geom_builder;

		/**
		 * Used to manipulate existing geometry selected by choose feature tool.
		 */
		GeometryBuilder *d_focused_feature_geom_builder;

		/**
		 * Used to determine if any feature is in focus.
		 */
		const GPlatesGui::FeatureFocus *d_feature_focus;

		/**
		 * Does the target decision making.
		 */
		TargetChooser d_target_chooser;

		/**
		 * The current geometry builder.
		 */
		GeometryBuilder *d_current_geometry_builder;

		void
		connect_to_feature_focus(
				const GPlatesGui::FeatureFocus &);

		void
		connect_to_choose_canvas_tool(
				const GPlatesGui::ChooseCanvasTool &);

		/**
		 * Returns the current geometry builder target or NULL if
		 * no target is available for geometry operations.
		 */
		GeometryBuilder *
		get_target(
				const TargetChooser &target_chooser) const;

		/**
		 * When one of the objects we listen to notifies us of a change we update_current_geometry_builder
		 * our state and send signals if necessary.
		 */
		void
		update_current_geometry_builder();

		/**
		 * Return true if geometry builders should target the focus geometry.
		 */
		bool
		target_focus_geometry() const;

		/**
		 * Handles notification of changed canvas tool.
		 */
		void
		changed_canvas_tool(
				GPlatesCanvasTools::CanvasToolType::Value canvas_tool_type);
	};


	/**
	 * Undo/redo command for restoring @a GeoemetryOperationTarget state.
	 */
	class GeometryOperationTargetUndoCommand :
		public QUndoCommand
	{
	public:
		/**
		 * @param geometry_operation_target @a ChooseCanvasTool object on which to call method.
		 * @param choose_canvas_tool_method method to call on @a choose_canvas_tool.
		 */
		GeometryOperationTargetUndoCommand(
				GeometryOperationTarget *geometry_operation_target,
				QUndoCommand *parent = 0);

		virtual
		void
		redo();

		virtual
		void
		undo();

	private:
		GeometryOperationTarget *d_geometry_operation_target;
		GeometryOperationTarget::internal_state_type d_internal_state;
		bool d_first_redo;
	};

}

#endif // GPLATES_VIEWOPERATIONS_GEOMETRYBUILDERTOOLTARGET_H
