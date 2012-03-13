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

#ifndef GPLATES_APP_LOGIC_COREGISTRATIONLAYERPROXY_H
#define GPLATES_APP_LOGIC_COREGISTRATIONLAYERPROXY_H

#include <vector>
#include <boost/optional.hpp>

#include "AppLogicFwd.h"
#include "CoRegistrationData.h"
#include "LayerProxy.h"
#include "LayerProxyUtils.h"
#include "RasterLayerProxy.h"
#include "ReconstructionLayerProxy.h"
#include "ReconstructLayerProxy.h"

#include "data-mining/CoRegConfigurationTable.h"

#include "global/PointerTraits.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRasterCoRegistration;
	class GLRenderer;
}

namespace GPlatesAppLogic
{
	/**
	 * A layer proxy that co-registers reconstructed seed geometries with reconstructed target features.
	 */
	class CoRegistrationLayerProxy :
			public LayerProxy
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a CoRegistrationLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<CoRegistrationLayerProxy> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a CoRegistrationLayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<const CoRegistrationLayerProxy> non_null_ptr_to_const_type;


		/**
		 * Creates a @a CoRegistrationLayerProxy object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new CoRegistrationLayerProxy());
		}


		~CoRegistrationLayerProxy();


		/**
		 * Returns the co-registration data for the current reconstruction time.
		 *
		 * @a renderer is required since *raster* co-registration is accelerated using OpenGL.
		 * If you do not already have a @a GLRenderer available then you'll need to retrieve a
		 * @a GLContext object and use that to create a @a GLRenderer. An OpenGL context usually
		 * requires some kind of operating system window (in Qt this can be a QGLWidget) which is
		 * what the globe and map views use - see "GlobeAndMapWidget::get_gl_context()".
		 * For situations where there's not typically a window handy (eg, using the GPlates
		 * python API without using the GPlates application) you'll probably need to create a
		 * window of some sort and associate an OpenGL context with it
		 * (perhaps the GPlates python API can provide something like this).
		 *
		 * Returns boost::none if the input layers are not connected or if
		 * @a set_current_coregistration_configuration_table has not yet been called (ie, the
		 * co-registration has not yet been configured by the user for this layer).
		 */
		boost::optional<coregistration_data_non_null_ptr_type>
		get_coregistration_data(
				GPlatesOpenGL::GLRenderer &renderer)
		{
			return get_coregistration_data(renderer, d_current_reconstruction_time);
		}


		/**
		 * Returns the co-registration data for the specified reconstruction time.
		 */
		boost::optional<coregistration_data_non_null_ptr_type>
		get_coregistration_data(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &reconstruction_time);


		/**
		 * Returns the subject token that clients can use to determine if the co-registration data
		 * has changed since it was last retrieved.
		 *
		 * This is mainly useful for other layers that have this layer connected as their input.
		 */
		const GPlatesUtils::SubjectToken &
		get_subject_token();


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
		 * Set the current input reconstruction layer proxy.
		 */
		void
		set_current_reconstruction_layer_proxy(
				const ReconstructionLayerProxy::non_null_ptr_type &reconstruction_layer_proxy);

		/**
		 * Adds a co-registration seed layer proxy.
		 */
		void
		add_coregistration_seed_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &coregistration_seed_layer_proxy);

		/**
		 * Removes a co-registration seed layer proxy.
		 */
		void
		remove_coregistration_seed_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &coregistration_seed_layer_proxy);

		/**
		 * Adds a co-registration target (reconstructed geometries) layer proxy.
		 */
		void
		add_coregistration_target_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &coregistration_target_layer_proxy);

		/**
		 * Removes a co-registration target (reconstructed geometries) layer proxy.
		 */
		void
		remove_coregistration_target_layer_proxy(
				const ReconstructLayerProxy::non_null_ptr_type &coregistration_target_layer_proxy);

		/**
		 * Adds a co-registration target (raster) layer proxy.
		 */
		void
		add_coregistration_target_layer_proxy(
				const RasterLayerProxy::non_null_ptr_type &coregistration_target_layer_proxy);

		/**
		 * Removes a co-registration target (raster) layer proxy.
		 */
		void
		remove_coregistration_target_layer_proxy(
				const RasterLayerProxy::non_null_ptr_type &coregistration_target_layer_proxy);

		/**
		 * Sets the configuration table to use for co-registration.
		 */
		void
		set_current_coregistration_configuration_table(
				const GPlatesDataMining::CoRegConfigurationTable &coregistration_configuration_table);

	private:
		/**
		 * Used to get reconstruction trees at desired reconstruction times.
		 *
		 * TODO: I'm not sure we really need a reconstruction tree layer ?
		 */
		LayerProxyUtils::InputLayerProxy<ReconstructionLayerProxy> d_current_reconstruction_layer_proxy;

		/**
		 * Used to get the co-registration reconstructed seed geometries.
		 */
		LayerProxyUtils::InputLayerProxySequence<ReconstructLayerProxy> d_current_seed_layer_proxies;

		/**
		 * Used to get the co-registration target (reconstructed geometries) layer proxies.
		 */
		LayerProxyUtils::InputLayerProxySequence<ReconstructLayerProxy> d_current_target_reconstruct_layer_proxies;

		/**
		 * Used to get the co-registration target (raster) layer proxies.
		 */
		LayerProxyUtils::InputLayerProxySequence<RasterLayerProxy> d_current_target_raster_layer_proxies;

		/**
		 * The current co-registration configuration.
		 */
		GPlatesDataMining::CoRegConfigurationTable d_current_coregistration_configuration_table;

		/**
		 * The current reconstruction time as set by the layer system.
		 */
		double d_current_reconstruction_time;

		/**
		 * Used to co-register rasters.
		 *
		 * The one instance is used to co-register all/any rasters and is only created when first used.
		 *
		 * NOTE: Used the method @a get_raster_co_registration to retrieve this.
		 */
		boost::optional<GPlatesGlobal::PointerTraits<GPlatesOpenGL::GLRasterCoRegistration>::non_null_ptr_type>
				d_raster_co_registration;

		/**
		 * The cached co-registration data - the output of co-registration.
		 */
		boost::optional<coregistration_data_non_null_ptr_type> d_cached_coregistration_data;

		/**
		 * Cached reconstruction time.
		 */
		boost::optional<GPlatesMaths::real_t> d_cached_reconstruction_time;

		/**
		 * Used to notify polling observers that we've been updated.
		 */
		mutable GPlatesUtils::SubjectToken d_subject_token;


		//! Default constructor.
		CoRegistrationLayerProxy();


		/**
		 * Resets any cached variables forcing them to be recalculated next time they're accessed.
		 */
		void
		reset_cache();


		/**
		 * Checks if the specified input layer proxy has changed.
		 *
		 * If so then reset caches and invalidates subject token.
		 */
		template <class InputLayerProxyWrapperType>
		void
		check_input_layer_proxy(
				InputLayerProxyWrapperType &input_layer_proxy_wrapper);


		/**
		 * Checks if any input layer proxies have changed.
		 *
		 * If so then reset caches and invalidates subject token.
		 */
		void
		check_input_layer_proxies();

		/**
		 * Returns the raster co-registration and creates one the first time this method is called.
		 *
		 * Returns boost::none if the OpenGL extensions required for raster co-registration are not available.
		 */
		boost::optional<GPlatesOpenGL::GLRasterCoRegistration &>
		get_raster_co_registration(
				GPlatesOpenGL::GLRenderer &renderer);
	};
}

#endif // GPLATES_APP_LOGIC_COREGISTRATIONLAYERPROXY_H
