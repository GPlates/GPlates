/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDSCALARFIELD3D_H
#define GPLATES_APP_LOGIC_RESOLVEDSCALARFIELD3D_H

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructionGeometry.h"

#include "AppLogicFwd.h"

#include "model/FeatureHandle.h"
#include "model/types.h"
#include "model/WeakObserver.h"


namespace GPlatesAppLogic
{
	/**
	 * A type of @a ReconstructionGeometry representing a 3D scalar field.
	 *
	 * Used to represent a constant or time-dependent scalar field.
	 * This currently just references the scalar field layer proxy.
	 */
	class ResolvedScalarField3D :
			public ReconstructionGeometry,
			public GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a ResolvedScalarField3D.
		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedScalarField3D> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a non-const @a ResolvedScalarField3D.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedScalarField3D> non_null_ptr_to_const_type;

		//! A convenience typedef for the WeakObserver base class of this class.
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;


		/**
		 * Create a @a ResolvedScalarField3D.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesModel::FeatureHandle &feature_handle,
				const double &reconstruction_time,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const scalar_field_3d_layer_proxy_non_null_ptr_type &scalar_field_layer_proxy)
		{
			return non_null_ptr_type(
					new ResolvedScalarField3D(
							feature_handle,
							reconstruction_time,
							reconstruction_tree_,
							scalar_field_layer_proxy));
		}


		/**
		 * Returns the reconstruction time at which raster is resolved/reconstructed.
		 */
		const double &
		get_reconstruction_time() const
		{
			return d_reconstruction_time;
		}


		/**
		 * Returns the scalar field layer proxy.
		 */
		const scalar_field_3d_layer_proxy_non_null_ptr_type &
		get_scalar_field_3d_layer_proxy() const
		{
			return d_scalar_field_layer_proxy;
		}


		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const;

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor);

	protected:
		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ResolvedScalarField3D(
				GPlatesModel::FeatureHandle &feature_handle,
				const double &reconstruction_time,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const scalar_field_3d_layer_proxy_non_null_ptr_type &scalar_field_layer_proxy);

	private:
		/**
		 * The reconstruction time at which scalar field is resolved/reconstructed.
		 */
		double d_reconstruction_time;

		/**
		 * The scalar field layer proxy.
		 */
		scalar_field_3d_layer_proxy_non_null_ptr_type d_scalar_field_layer_proxy;
	};
}

#endif // GPLATES_APP_LOGIC_RESOLVEDSCALARFIELD3D_H
