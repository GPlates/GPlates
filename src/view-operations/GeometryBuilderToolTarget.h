/* $Id$ */

/**
 * \file 
 * Determines which @a GeometryBuilder and main rendered layer each
 * geometry builder canvas tool should target.
 * $Revision$
 * $Date$
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

#ifndef GPLATES_VIEWOPERATIONS_GEOMETRYBUILDERTOOLTARGET_H
#define GPLATES_VIEWOPERATIONS_GEOMETRYBUILDERTOOLTARGET_H

#include <QObject>

#include "RenderedGeometryCollection.h"
#include "gui/ChooseCanvasTool.h"
#include "gui/FeatureFocus.h"


namespace GPlatesViewOperations
{
	class GeometryBuilder;
	class RenderedGeometryCollection;

	/**
	 * Manages which geometry builder tools target which geometry at which times.
	 * Two geometry types currently supported are temporary geometry used
	 * for digitising new geometry and chosen feature geometry selected by
	 * the @a ClickGeometry tool.
	 */
	class GeometryBuilderToolTarget :
			public QObject
	{
		Q_OBJECT

	public:
		/**
		 * Type of tool that builds/manipulates geometry.
		 */
		enum ToolType
		{
			DIGITISE_GEOMETRY,
			MOVE_VERTEX,

			NUM_TOOLS
		};

		GeometryBuilderToolTarget(
				GeometryBuilder &digitise_geom_builder,
				GeometryBuilder &focused_feature_geom_builder,
				const RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesGui::FeatureFocus &feature_focus);

		/**
		 * Activate a tool type.
		 * This will send signals to those slots listening to us.
		 */
		void
		activate(
				ToolType);

		/**
		 * Which GeometryBuilder should the currently activated tool operate on.
		 */
		GeometryBuilder *
		get_geometry_builder_for_active_tool() const;

		/**
		 * Which main rendered layer should the currently activated tool operate on.
		 */
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType
		get_main_rendered_layer_for_active_tool() const;

	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		switched_move_vertex_geometry_builder(
				GPlatesViewOperations::GeometryBuilderToolTarget &,
				GPlatesViewOperations::GeometryBuilder *);

		void
		switched_move_vertex_main_rendered_layer(
				GPlatesViewOperations::GeometryBuilderToolTarget &,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType);

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
		 * Every time the @a RenderedGeometryCollection is updated we'll see if the main layer active
		 * status has changed and update ourselves accordingly.
		 */
		void
		collection_was_updated(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::main_layers_update_type main_layers_updated);
	private:
		/**
		 * Used to build temporary geometry for digitising new geometry.
		 */
		GeometryBuilder *d_digitise_geom_builder;

		/**
		 * Used to manipulate existing geometry selected by choose feature tool.
		 */
		GeometryBuilder *d_focused_feature_geom_builder;

		/**
		 * Used to determine which main rendered layers are currently visible.
		 */
		const RenderedGeometryCollection *d_rendered_geom_collection;

		/**
		 * Used to determine if any feature is in focus.
		 */
		const GPlatesGui::FeatureFocus *d_feature_focus;

		/**
		 * Is true if there is currently geometry in focus.
		 */
		bool d_is_geometry_in_focus;

		/**
		 * The current geometry builder targets for each tool type.
		 */
		GeometryBuilder *d_current_geom_builder_targets[NUM_TOOLS];

		/**
		 * The current geometry builder targets for each tool type.
		 */
		RenderedGeometryCollection::MainLayerType d_current_main_layer_targets[NUM_TOOLS];

		/**
		 * We keep track of which main rendered layers are active as this
		 * helps determine which geometry builder is targeted.
		 */
		RenderedGeometryCollection::MainLayerActiveState
				d_main_rendered_layer_active_state;

		/**
		 * Current tool type that's been activated.
		 */
		ToolType d_current_tool_type;

		void
		initialise_targets();

		void
		connect_to_feature_focus(
				const GPlatesGui::FeatureFocus &);

		void
		connect_to_rendered_geom_collection(
				const RenderedGeometryCollection &);

		/**
		 * When one of the objects we listen to notifies us of a change we update
		 * our state and send signals if necessary.
		 */
		void
		update();

		void
		update_move_vertex();

		/**
		 * Return true if geometry builders should target the focus geometry.
		 */
		bool
		target_focus_geometry() const;
	};
}

#endif // GPLATES_VIEWOPERATIONS_GEOMETRYBUILDERTOOLTARGET_H
