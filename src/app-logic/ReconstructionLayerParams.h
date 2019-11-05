/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONLAYERPARAMS_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONLAYERPARAMS_H

#include <QObject>

#include "LayerParams.h"
#include "ReconstructionParams.h"


namespace GPlatesAppLogic
{
	/**
	 * App-logic parameters for a reconstruction layer.
	 */
	class ReconstructionLayerParams :
			public LayerParams
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ReconstructionLayerParams());
		}


		/**
		 * Returns the 'const' reconstruction parameters.
		 */
		const ReconstructionParams &
		get_reconstruction_params() const
		{
			return d_reconstruction_params;
		}

		/**
		 * Sets the reconstruction parameters.
		 *
		 * Emits signals 'modified_reconstruction_params' and 'modified' if a change detected.
		 */
		void
		set_reconstruction_params(
				const ReconstructionParams &reconstruction_params)
		{
			if (d_reconstruction_params == reconstruction_params)
			{
				return;
			}

			d_reconstruction_params = reconstruction_params;

			Q_EMIT modified_reconstruction_params(*this);
			emit_modified();
		}


		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerParamsVisitor &visitor) const
		{
			visitor.visit_reconstruction_layer_params(*this);
		}

		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				LayerParamsVisitor &visitor)
		{
			visitor.visit_reconstruction_layer_params(*this);
		}

	Q_SIGNALS:

		/**
		 * Emitted when @a set_reconstruction_params has been called (if a change detected).
		 */
		void
		modified_reconstruction_params(
				GPlatesAppLogic::ReconstructionLayerParams &layer_params);

	private:
		ReconstructionParams d_reconstruction_params;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONLAYERPARAMS_H
