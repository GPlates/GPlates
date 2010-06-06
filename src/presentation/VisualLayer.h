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
 
#ifndef GPLATES_PRESENTATION_VISUALLAYER_H
#define GPLATES_PRESENTATION_VISUALLAYER_H

#include <boost/noncopyable.hpp>

#include "app-logic/Layer.h"

#include "model/FeatureCollectionHandle.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesPresentation
{
	/**
	 * Represents a layer that processes inputs (such as a feature collection)
	 * to an output (such as reconstruction geometries) and determines how
	 * to visualise the output.
	 */
	class VisualLayer :
			private boost::noncopyable
	{
	public:
		/**
		 * Constructor wraps a visual layer around @a layer created in @a ReconstructGraph.
		 */
		VisualLayer(
				const GPlatesAppLogic::Layer &layer,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection);


		/**
		 * Returns the layer used by @a ReconstructGraph.
		 */
		const GPlatesAppLogic::Layer &
		get_reconstruct_graph_layer() const
		{
			return d_layer;
		}


		/**
		 * Creates rendered geometries for this visual layer.
		 *
		 * Each visual layer has its own rendered geometry layer created inside the
		 * @a RenderedGeometryCollection passed into the constructor.
		 */
		void
		create_rendered_geometries();


	private:
		GPlatesAppLogic::Layer d_layer;

		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
				d_rendered_geometry_layer;
	};
}

#endif // GPLATES_PRESENTATION_VISUALLAYER_H
