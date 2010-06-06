/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_PRESENTATION_VISUALLAYERS_H
#define GPLATES_PRESENTATION_VISUALLAYERS_H

#include <list>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <QObject>

#include "VisualLayer.h"

#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructGraph.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;

	class VisualLayers :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:
		VisualLayers(
				GPlatesPresentation::ViewState &view_state);


	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Creates rendered geometries for each active visual layer.
		 *
		 * Each visual layer has its own rendered geometry layer created inside the
		 * @a RenderedGeometryCollection (contained in the @a ViewState passed into the
		 * constructor). There rendered geometry layers are created inside the
		 * 'RECONSTRUCTION_LAYER' main rendered layer.
		 *
		 * NOTE: This won't perform a new reconstruction, it'll just iterate over the
		 * visual layers and convert any reconstruction geometries (created by the most
		 * recent reconstruction in @a ApplicationState) into rendered geometries
		 * (and remove the old rendered geometries from the individual rendered geometry layers).
		 *
		 * This call will automatically get triggered, however, when the @a ApplicationState
		 * performs a new reconstruction.
		 *
		 * This method can be explicitly called when render settings/styles have changed to
		 * avoid performing a new reconstruction when it's not necessary.
		 */
		void
		create_rendered_geometries();


	private slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		handle_layer_added(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer);

		void
		handle_layer_about_to_be_removed(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer);

	private:
		/**
		 * Typedef for a shared pointer to a @a VisualLayer.
		 */
		typedef boost::shared_ptr<VisualLayer> visual_layer_ptr_type;

		/**
		 * Typedef for mapping a layer to a visual layer.
		 */
		typedef std::map<GPlatesAppLogic::Layer, visual_layer_ptr_type> visual_layer_map_type;


		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geometry_collection;

		/**
		 * Record of all visual layers associated with application state layers.
		 *
		 * Each layer has its own rendered geometry layer so that draw order
		 * of the layers can be controlled.
		 */
		visual_layer_map_type d_visual_layers;


		void
		connect_to_application_state_signals();

		boost::shared_ptr<VisualLayer>
		create_visual_layer(
				const GPlatesAppLogic::Layer &layer);
	};
}

#endif // GPLATES_PRESENTATION_VISUALLAYERS_H
