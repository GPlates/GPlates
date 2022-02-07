/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTLAYERPARAMS_H
#define GPLATES_APP_LOGIC_RECONSTRUCTLAYERPARAMS_H

#include <QObject>

#include "LayerParams.h"
#include "ReconstructParams.h"


namespace GPlatesAppLogic
{
	/**
	 * App-logic parameters for a reconstruct layer.
	 */
	class ReconstructLayerParams :
			public LayerParams
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ReconstructLayerParams());
		}


		/**
		 * Returns the 'const' reconstruct parameters.
		 */
		const ReconstructParams &
		get_reconstruct_params() const
		{
			return d_reconstruct_params;
		}

		/**
		 * Sets the reconstruct parameters.
		 *
		 * Emits signals 'modified_reconstruct_params' and 'modified' if a change detected.
		 */
		void
		set_reconstruct_params(
				const ReconstructParams &reconstruct_params);


		/**
		 * Whether to bring up the Set Topology Reconstruction Parameters dialog when selecting
		 * to reconstruct with topologies.
		 *
		 * Since it can take a long time to initialise topology reconstruction, this gives the user
		 * an opportunity to change the parameters before initialisation so they don't get hit with
		 * a long initialisation twice (once when selecting topology reconstruction and again when
		 * changing parameters).
		 */
		bool
		get_prompt_to_change_topology_reconstruction_parameters() const
		{
			return d_prompt_to_change_topology_reconstruction_parameters;
		}

		void
		set_prompt_to_change_topology_reconstruction_parameters(
				bool prompt_to_change_parameters)
		{
			if (d_prompt_to_change_topology_reconstruction_parameters == prompt_to_change_parameters)
			{
				return;
			}

			d_prompt_to_change_topology_reconstruction_parameters = prompt_to_change_parameters;
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
			visitor.visit_reconstruct_layer_params(*this);
		}

		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				LayerParamsVisitor &visitor)
		{
			visitor.visit_reconstruct_layer_params(*this);
		}

	Q_SIGNALS:

		/**
		 * Emitted when @a set_reconstruct_params has been called (if a change detected).
		 */
		void
		modified_reconstruct_params(
				GPlatesAppLogic::ReconstructLayerParams &layer_params);

	private:

		ReconstructParams d_reconstruct_params;

		/**
		 * Whether to bring up the Set Topology Reconstruction Parameters dialog when selecting
		 * to reconstruct with topologies.
		 */
		bool d_prompt_to_change_topology_reconstruction_parameters;


		ReconstructLayerParams() :
			d_prompt_to_change_topology_reconstruction_parameters(true)
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTLAYERPARAMS_H
