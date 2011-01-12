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

#include <QObject>

#include "utils/ReferenceCount.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesPresentation
{
	/**
	 * This is the base class of classes that store parameters and options specific
	 * to particular types of visual layers. This allows us to keep the
	 * @a VisualLayers class clean from code specific to one type of visual layer.
	 *
	 * This is the visual layers analogue of @a GPlatesAppLogic::LayerTaskParams.
	 * If the parameters and options that you wish to store impact upon the
	 * operation of a @a LayerTask, they need to reside in a @a LayerTaskParams
	 * derivation, not in a @a VisualLayerParams derivation. (But of course, one
	 * may wish to have both a @a VisualLayerParams derivation, for visualisation-
	 * specific options and a @a LayerTaskParams derivation.)
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
		create()
		{
			return new VisualLayerParams();
		}

		virtual
		~VisualLayerParams()
		{  }

	signals:

		/**
		 * Emitted when any aspect of the parameters has been modified.
		 */
		void
		modified();

	protected:

		VisualLayerParams()
		{  }

		/**
		 * Subclasses should call this method to cause the @a modified() signal to be
		 * emitted.
		 */
		void
		emit_modified()
		{
			emit modified();
		}
	};
}

#endif // GPLATES_PRESENTATION_VISUALLAYERPARAMS_H
