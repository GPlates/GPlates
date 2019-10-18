/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_PRESENTATION_VISUALLAYERPARAMS_H
#define GPLATES_PRESENTATION_VISUALLAYERPARAMS_H

#include <boost/shared_ptr.hpp>
#include <QObject>

#include "ReconstructionGeometrySymboliser.h"
#include "VisualLayerParamsVisitor.h"

#include "app-logic/LayerParams.h"

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesAppLogic
{
	class Layer;
}

namespace GPlatesGui
{
	class StyleAdapter;
}

namespace GPlatesPresentation
{
	class ViewState;

	/**
	 * This is the base class of classes that store parameters and options specific
	 * to particular types of visual layers. This allows us to keep the
	 * @a VisualLayers class clean from code specific to one type of visual layer.
	 *
	 * This is the visual layers analogue of @a GPlatesAppLogic::LayerParams.
	 * If the parameters and options that you wish to store impact upon the
	 * operation of a @a LayerTask, they need to reside in a @a LayerParams
	 * derivation, not in a @a VisualLayerParams derivation. (But of course, one
	 * may wish to have both a @a VisualLayerParams derivation, for visualisation-
	 * specific options and a @a LayerParams derivation.)
	 */
	class VisualLayerParams :
			public QObject,
			public GPlatesUtils::ReferenceCount<VisualLayerParams>
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<VisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const VisualLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params,
				ViewState &view_state)
		{
			return new VisualLayerParams(layer_params, view_state);
		}

		virtual
		~VisualLayerParams()
		{  }

		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{  }

		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{  }

		/**
		 * Subclasses should override this to get notified when the app-logic
		 * layer corresponding to the parent visual layer has had an input
		 * connection added or removed.
		 *
		 * This function is also guaranteed to be called immediately after the
		 * instance of the subclass is constructed.
		 */
		virtual
		void
		handle_layer_modified(
				const GPlatesAppLogic::Layer &layer)
		{  }


		/**
		 * The reconstruction geometry symboliser for this layer.
		 *
		 * Note: When the symboliser is modified then our 'modified' signal is emitted.
		 */
		ReconstructionGeometrySymboliser &
		get_reconstruction_geometry_symboliser()
		{
			return *d_reconstruction_geometry_symboliser;
		}

		const ReconstructionGeometrySymboliser &
		get_reconstruction_geometry_symboliser() const
		{
			return *d_reconstruction_geometry_symboliser;
		}


		void
		set_style_adapter(
				const GPlatesGui::StyleAdapter* adapter)
		{
			d_style = adapter;
		}

		const GPlatesGui::StyleAdapter*
		style_adapter() const
		{
			return d_style;
		}

	Q_SIGNALS:

		/**
		 * Emitted when any aspect of the parameters has been modified (including the symboliser).
		 */
		void
		modified();

	protected:

		explicit
		VisualLayerParams(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params,
				ViewState &view_state,
				const GPlatesGui::StyleAdapter* style = NULL) :
			d_layer_params(layer_params),
			d_style(style),
			d_reconstruction_geometry_symboliser(ReconstructionGeometrySymboliser::create())
		{
			// When the symboliser is modified then we are modified.
			QObject::connect(
					d_reconstruction_geometry_symboliser.get(), SIGNAL(modified()),
					this, SIGNAL(modified()));
		}

		GPlatesAppLogic::LayerParams::non_null_ptr_type
		get_layer_params() const
		{
			return d_layer_params;
		}

		/**
		 * Subclasses should call this method to cause the @a modified() signal to be
		 * emitted.
		 */
		void
		emit_modified()
		{
			Q_EMIT modified();
		}

	private:

		GPlatesAppLogic::LayerParams::non_null_ptr_type d_layer_params;
		const GPlatesGui::StyleAdapter* d_style;
		ReconstructionGeometrySymboliser::non_null_ptr_type d_reconstruction_geometry_symboliser;
	};
}

#endif // GPLATES_PRESENTATION_VISUALLAYERPARAMS_H
