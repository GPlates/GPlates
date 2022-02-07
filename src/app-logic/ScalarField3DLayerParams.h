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

#ifndef GPLATES_APP_LOGIC_SCALARFIELD3DLAYERPARAMS_H
#define GPLATES_APP_LOGIC_SCALARFIELD3DLAYERPARAMS_H

#include <boost/optional.hpp>
#include <QObject>

#include "LayerParams.h"

#include "model/FeatureHandle.h"


namespace GPlatesAppLogic
{
	/**
	 * App-logic parameters for a 3D scalar field layer.
	 */
	class ScalarField3DLayerParams :
			public LayerParams
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ScalarField3DLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ScalarField3DLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ScalarField3DLayerParams());
		}


		/**
		 * Sets (or unsets) the 3D scalar field feature.
		 *
		 * Emits 'modified' signal.
		 */
		void
		set_scalar_field_feature(
				boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref);


		//! Returns the raster feature or boost::none if one is currently not set on the layer.
		const boost::optional<GPlatesModel::FeatureHandle::weak_ref> &
		get_scalar_field_feature() const
		{
			return d_scalar_field_feature;
		}

		//! Returns the minimum depth layer radius of scalar field or none if no field.
		boost::optional<double>
		get_minimum_depth_layer_radius() const
		{
			return d_minimum_depth_layer_radius;
		}

		//! Returns the maximum depth layer radius of scalar field or none if no field.
		boost::optional<double>
		get_maximum_depth_layer_radius() const
		{
			return d_maximum_depth_layer_radius;
		}

		/**
		 * Returns the minimum scalar value across the entire scalar field or none if no field.
		 *
		 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
		 */
		boost::optional<double>
		get_scalar_min() const
		{
			return d_scalar_min;
		}

		/**
		 * Returns the maximum scalar value across the entire scalar field or none if no field.
		 *
		 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
		 */
		boost::optional<double>
		get_scalar_max() const
		{
			return d_scalar_max;
		}

		/**
		 * Returns the mean scalar value across the entire scalar field or none if no field.
		 *
		 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
		 */
		boost::optional<double>
		get_scalar_mean() const
		{
			return d_scalar_mean;
		}

		/**
		 * Returns the standard deviation of scalar values across the entire scalar field or none if no field.
		 *
		 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
		 */
		boost::optional<double>
		get_scalar_standard_deviation() const
		{
			return d_scalar_standard_deviation;
		}

		/**
		 * Returns the minimum gradient magnitude across the entire scalar field or none if no field.
		 *
		 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
		 */
		boost::optional<double>
		get_gradient_magnitude_min() const
		{
			return d_gradient_magnitude_min;
		}

		/**
		 * Returns the maximum gradient magnitude across the entire scalar field or none if no field.
		 *
		 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
		 */
		boost::optional<double>
		get_gradient_magnitude_max() const
		{
			return d_gradient_magnitude_max;
		}

		/**
		 * Returns the mean gradient magnitude across the entire scalar field or none if no field.
		 *
		 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
		 */
		boost::optional<double>
		get_gradient_magnitude_mean() const
		{
			return d_gradient_magnitude_mean;
		}

		/**
		 * Returns the standard deviation of gradient magnitudes across the entire scalar field or none if no field.
		 *
		 * NOTE: When time-dependent fields are supported this will be a statistic of the field at present day.
		 */
		boost::optional<double>
		get_gradient_magnitude_standard_deviation() const
		{
			return d_gradient_magnitude_standard_deviation;
		}


		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerParamsVisitor &visitor) const
		{
			visitor.visit_scalar_field_3d_layer_params(*this);
		}

		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				LayerParamsVisitor &visitor)
		{
			visitor.visit_scalar_field_3d_layer_params(*this);
		}

	private:

		//! The scalar field feature.
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_scalar_field_feature;

		boost::optional<double> d_minimum_depth_layer_radius;
		boost::optional<double> d_maximum_depth_layer_radius;

		boost::optional<double> d_scalar_min;
		boost::optional<double> d_scalar_max;
		boost::optional<double> d_scalar_mean;
		boost::optional<double> d_scalar_standard_deviation;

		boost::optional<double> d_gradient_magnitude_min;
		boost::optional<double> d_gradient_magnitude_max;
		boost::optional<double> d_gradient_magnitude_mean;
		boost::optional<double> d_gradient_magnitude_standard_deviation;


		ScalarField3DLayerParams()
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_SCALARFIELD3DLAYERPARAMS_H
