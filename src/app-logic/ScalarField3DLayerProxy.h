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

#ifndef GPLATES_APP_LOGIC_SCALARFIELD3DLAYERPROXY_H
#define GPLATES_APP_LOGIC_SCALARFIELD3DLAYERPROXY_H

#include <utility>
#include <boost/optional.hpp>

#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "ScalarField3DLayerTask.h"
#include "ResolvedScalarField3D.h"

#include "maths/types.h"

#include "model/FeatureHandle.h"

#include "property-values/TextContent.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesAppLogic
{
	/**
	 * A layer proxy for a 3D scalar field to be visualised using volume rendering.
	 */
	class ScalarField3DLayerProxy :
			public LayerProxy
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a ScalarField3DLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<ScalarField3DLayerProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a ScalarField3DLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ScalarField3DLayerProxy> non_null_ptr_to_const_type;


		/**
		 * Creates a @a ScalarField3DLayerProxy object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ScalarField3DLayerProxy());
		}


		/**
		 * Returns the scalar field filename for the current reconstruction time.
		 */
		const boost::optional<GPlatesPropertyValues::TextContent> &
		get_scalar_field_filename()
		{
			return get_scalar_field_filename(d_current_reconstruction_time);
		}

		/**
		 * Returns the scalar field filename at the specified reconstruction time.
		 */
		const boost::optional<GPlatesPropertyValues::TextContent> &
		get_scalar_field_filename(
				const double &reconstruction_time);


		/**
		 * Returns the resolved scalar field for the current reconstruction time.
		 *
		 * This is currently (a derivation of @a ReconstructionGeometry) that just references this layer proxy.
		 * An example client of @a ResolvedScalarField3D is @a GLVisualLayers which is
		 * responsible for *visualising* the scalar field on the screen.
		 *
		 * Returns boost::none if there is no input scalar field feature connected or it cannot be resolved.
		 */
		boost::optional<ResolvedScalarField3D::non_null_ptr_type>
		get_resolved_scalar_field_3d()
		{
			return get_resolved_scalar_field_3d(d_current_reconstruction_time);
		}

		/**
		 * Returns the resolved scalar field for the specified time.
		 *
		 * Returns boost::none if there is no input scalar field feature connected or it cannot be resolved.
		 */
		boost::optional<ResolvedScalarField3D::non_null_ptr_type>
		get_resolved_scalar_field_3d(
				const double &reconstruction_time);


		/**
		 * Returns the subject token that clients can use to determine if this scalar field layer proxy has changed.
		 *
		 * This is mainly useful for other layers that have this layer connected as their input.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token();


		/**
		 * Returns the subject token that clients can use to determine if the scalar field itself
		 * has changed for the current reconstruction time.
		 *
		 * This is useful for time-dependent scalar fields.
		 */
		const GPlatesUtils::SubjectToken &
		get_scalar_field_subject_token()
		{
			return get_scalar_field_subject_token(d_current_reconstruction_time);
		}

		/**
		 * Returns the subject token that clients can use to determine if the scalar field itself
		 * has changed for the specified reconstruction time.
		 *
		 * This is useful for time-dependent scalar fields.
		 */
		const GPlatesUtils::SubjectToken &
		get_scalar_field_subject_token(
				const double &reconstruction_time);


		/**
		 * Returns the subject token that clients can use to determine if the scalar field feature has changed.
		 *
		 * This is useful for determining if only the scalar field feature has changed.
		 */
		const GPlatesUtils::SubjectToken &
		get_scalar_field_feature_subject_token();


		/**
		 * Accept a ConstLayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerProxyVisitor &visitor) const
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}

		/**
		 * Accept a LayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				LayerProxyVisitor &visitor)
		{
			visitor.visit(GPlatesUtils::get_non_null_pointer(this));
		}


		//
		// Used by LayerTask...
		//

		/**
		 * Sets the current reconstruction time as set by the layer system.
		 */
		void
		set_current_reconstruction_time(
				const double &reconstruction_time);

		/**
		 * Specify the scalar field feature.
		 */
		void
		set_current_scalar_field_feature(
				boost::optional<GPlatesModel::FeatureHandle::weak_ref> scalar_field_feature,
				const ScalarField3DLayerTask::Params &scalar_field_params);

		/**
		 * The scalar field feature has been modified.
		 */
		void
		modified_scalar_field_feature(
				const ScalarField3DLayerTask::Params &scalar_field_params);

	private:
		/**
		 * Potentially time-varying feature properties for the currently resolved scalar field
		 * (ie, at the cached reconstruction time).
		 */
		struct ResolvedScalarFieldFeatureProperties
		{
			void
			invalidate()
			{
				cached_scalar_field_filename = boost::none;
			}

			//! The scalar field filename.
			boost::optional<GPlatesPropertyValues::TextContent> cached_scalar_field_filename;

			//! The reconstruction time.
			boost::optional<GPlatesMaths::real_t> cached_reconstruction_time;
		};


		/**
		 * The scalar field input feature.
		 */
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_current_scalar_field_feature;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		//! Time-varying (potentially) scalar field feature properties.
		ResolvedScalarFieldFeatureProperties d_cached_resolved_scalar_field_feature_properties;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;

		/**
		 * The subject token that clients can use to determine if the scalar field itself has changed.
		 */
		mutable GPlatesUtils::SubjectToken d_scalar_field_subject_token;

		/**
		 * The subject token that clients can use to determine if the scalar field feature has changed.
		 */
		mutable GPlatesUtils::SubjectToken d_scalar_field_feature_subject_token;


		ScalarField3DLayerProxy() :
			d_current_reconstruction_time(0)
		{  }


		void
		invalidate_scalar_field_feature();

		void
		invalidate_scalar_field();

		void
		invalidate();


		/**
		 * Attempts to resolve a scalar field.
		 *
		 * Can fail if not enough information is available to resolve the scalar field.
		 * Such as no scalar field feature or scalar field feature does not have the required property values.
		 * In this case the returned value will be false.
		 */
		bool
		resolve_scalar_field_feature(
				const double &reconstruction_time);


		//! Sets some scalar field parameters.
		void
		set_scalar_field_params(
				const ScalarField3DLayerTask::Params &raster_params);
	};
}

#endif // GPLATES_APP_LOGIC_SCALARFIELD3DLAYERPROXY_H
