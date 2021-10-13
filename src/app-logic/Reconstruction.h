/* $Id$ */

/**
 * \file 
 * Contains the definition of the class Reconstruction.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTION_H
#define GPLATES_APP_LOGIC_RECONSTRUCTION_H

#include <boost/optional.hpp>

#include "ReconstructionLayerProxy.h"

#include "maths/Real.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * This class represents a plate-tectonic reconstruction at a particular geological
	 * time-instant.
	 *
	 * It represents the output of the layer reconstruct graph and contains the layer outputs
	 * (layer proxy objects) for all *active* layers.
	 *
	 * Results can be obtained by determining the derived types of the layer proxy objects
	 * and then querying those interfaces.
	 */
	class Reconstruction :
			public GPlatesUtils::ReferenceCount<Reconstruction>
	{
	public:
		//! A convenience typedef for a shared pointer to non-const @a Reconstruction.
		typedef GPlatesUtils::non_null_intrusive_ptr<Reconstruction> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to const @a Reconstruction.
		typedef GPlatesUtils::non_null_intrusive_ptr<const Reconstruction> non_null_ptr_to_const_type;

		//! Typedef for a sequence of *active* layer outputs (in the form of layer proxies).
		typedef std::vector<LayerProxy::non_null_ptr_type> layer_output_seq_type;


		/**
		 * Create a new blank Reconstruction instance with the default reconstruction tree
		 * as @a default_reconstruction_tree.
		 */
		static
		const non_null_ptr_type
		create(
				const double &reconstruction_time,
				const ReconstructionLayerProxy::non_null_ptr_type &default_reconstruction_layer_proxy)
		{
			return non_null_ptr_type(new Reconstruction(reconstruction_time, default_reconstruction_layer_proxy));
		}


		/**
		 * Create a new blank Reconstruction instance with the default reconstruction layer output
		 * being one that returns empty reconstruction trees (ie, returns identity rotations for all plates).
		 */
		static
		const non_null_ptr_type
		create(
				const double &reconstruction_time)
		{
			return non_null_ptr_type(new Reconstruction(reconstruction_time));
		}


		/**
		 * Adds the output of an *active* layer to this reconstruction.
		 */
		void
		add_active_layer_output(
				const LayerProxy::non_null_ptr_type &layer_proxy)
		{
			d_active_layer_outputs.push_back(layer_proxy);
		}


		/**
		 * Returns the sequence of *active* layer outputs for this reconstruction.
		 */
		const layer_output_seq_type &
		get_active_layer_outputs() const
		{
			return d_active_layer_outputs;
		}


		/**
		 * Returns the reconstruction time used for all reconstruction trees and
		 * all reconstructed geometries.
		 */
		const double &
		get_reconstruction_time() const
		{
			return d_reconstruction_time.dval();
		}

		
		/**
		 * Returns the reconstruction layer proxy used to reconstruct layers that are not explicitly
		 * connected to an input reconstruction layer.
		 */
		ReconstructionLayerProxy::non_null_ptr_type
		get_default_reconstruction_layer_output() const
		{
			return d_default_reconstruction_layer_proxy;
		}


		/**
		 * Sets the reconstruction layer proxy used to reconstruct layers that are not explicitly
		 * connected to an input reconstruction layer.
		 *
		 * If this is never called then the default reconstruction layer proxy is
		 * the one generated in the constructor.
		 */
		void
		set_default_reconstruction_layer_output(
				const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy)
		{
			d_default_reconstruction_layer_proxy = reconstruction_layer_proxy;
		}

	private:
		/**
		 * The reconstruction time at which all reconstructions are performed.
		 */
		GPlatesMaths::Real d_reconstruction_time;

		/**
		 * The reconstruction layer proxy used to reconstruct layers that are not explicitly
		 * connected to an input reconstruction layer.
		 */
		ReconstructionLayerProxy::non_null_ptr_type d_default_reconstruction_layer_proxy;

		/**
		 * The sequence of active layer outputs.
		 */
		layer_output_seq_type d_active_layer_outputs;


		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		Reconstruction(
				const double &reconstruction_time,
				const ReconstructionLayerProxy::non_null_ptr_type &default_reconstruction_layer_proxy);

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		explicit
		Reconstruction(
				const double &reconstruction_time);
	};
}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTION_H
