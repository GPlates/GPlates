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
 
#ifndef GPLATES_APP_LOGIC_LAYERPARAMS_H
#define GPLATES_APP_LOGIC_LAYERPARAMS_H

#include <QObject>

#include "LayerParamsVisitor.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * This is the base class of classes that store parameters and options specific
	 * to particular types of layers (layer task types).
	 */
	class LayerParams :
			public QObject,
			public GPlatesUtils::ReferenceCount<LayerParams>
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<LayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const LayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new LayerParams());
		}

		virtual
		~LayerParams()
		{  }

		virtual
		void
		accept_visitor(
				ConstLayerParamsVisitor &visitor) const
		{  }

		virtual
		void
		accept_visitor(
				LayerParamsVisitor &visitor)
		{  }

	Q_SIGNALS:

		/**
		 * Emitted when any aspect of the parameters has been modified.
		 */
		void
		modified(
				GPlatesAppLogic::LayerParams &layer_params);

	protected:

		/**
		 * Subclasses should call this method to cause the @a modified() signal to be emitted.
		 */
		void
		emit_modified()
		{
			Q_EMIT modified(*this);
		}
	};
}

#endif // GPLATES_APP_LOGIC_LAYERPARAMS_H
