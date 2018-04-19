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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTSCALARCOVERAGELAYERPARAMS_H
#define GPLATES_APP_LOGIC_RECONSTRUCTSCALARCOVERAGELAYERPARAMS_H

#include <map>
#include <vector>
#include <boost/optional.hpp>
#include <QObject>

#include "LayerParams.h"
#include "ReconstructScalarCoverageLayerProxy.h"
#include "ReconstructScalarCoverageParams.h"

#include "property-values/ScalarCoverageStatistics.h"
#include "property-values/ValueObjectType.h"

#include "utils/SubjectObserverToken.h"


namespace GPlatesAppLogic
{
	/**
	 * App-logic parameters for a reconstruct scalar coverage layer.
	 */
	class ReconstructScalarCoverageLayerParams :
			public LayerParams
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructScalarCoverageLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructScalarCoverageLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				const ReconstructScalarCoverageLayerProxy::non_null_ptr_type &layer_proxy)
		{
			return non_null_ptr_type(new ReconstructScalarCoverageLayerParams(layer_proxy));
		}


		/**
		 * Sets the reconstructing coverage parameters.
		 *
		 * NOTE: This does *not* update the reconstructed scalar coverages layer proxy.
		 */
		void
		set_reconstruct_scalar_coverage_params(
				const ReconstructScalarCoverageParams &reconstruct_scalar_coverage_params);

		/**
		 * Sets the scalar type, of the scalar coverage, for visualisation/processing.
		 *
		 * NOTE: This updates the reconstructed scalar coverages layer proxy.
		 */
		void
		set_scalar_type(
				GPlatesPropertyValues::ValueObjectType scalar_type);


		/**
		 * Returns the 'const' reconstructing coverage parameters.
		 *
		 * Emits signals 'modified_reconstruct_scalar_coverage_params' and 'modified' if a change detected.
		 */
		const ReconstructScalarCoverageParams &
		get_reconstruct_scalar_coverage_params() const
		{
			return d_reconstruct_scalar_coverage_params;
		}

		//! Returns the scalar type currently selected for visualisation/processing.
		const GPlatesPropertyValues::ValueObjectType &
		get_scalar_type() const;

		//! Returns the list of scalar types available in the scalar coverage features.
		void
		get_scalar_types(
				std::vector<GPlatesPropertyValues::ValueObjectType> &scalar_types) const
		{
			d_layer_proxy->get_scalar_types(scalar_types);
		}

		/**
		 * Gets all scalar coverages available across the scalar coverage features.
		 */
		void
		get_scalar_coverages(
				std::vector<ScalarCoverageFeatureProperties::Coverage> &scalar_coverages) const
		{
			d_layer_proxy->get_scalar_coverages(scalar_coverages);
		}

		/**
		 * Returns the scalar statistics across all scalar coverages of the specified scalar type,
		 * or none if no coverages.
		 *
		 * Note: This statistic includes the time history of evolved scalar values (where applicable).
		 */
		boost::optional<GPlatesPropertyValues::ScalarCoverageStatistics>
		get_scalar_statistics(
				const GPlatesPropertyValues::ValueObjectType &scalar_type) const;


		/**
		 * Detect any changes in the layer params due to changes in the layer proxy (due to changes in its dependencies).
		 *
		 * We need this so we can update our layer params since we don't get notified *directly* of
		 * changes in the Reconstruct layer that our Reconstruct Scalar Coverage layer is connected to.
		 * For example, if the scalar coverage features are reloaded from file they might no longer
		 * contain the currently selected scalar type.
		 */
		void
		update();


		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerParamsVisitor &visitor) const
		{
			visitor.visit_reconstruct_scalar_coverage_layer_params(*this);
		}

		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				LayerParamsVisitor &visitor)
		{
			visitor.visit_reconstruct_scalar_coverage_layer_params(*this);
		}

	Q_SIGNALS:

		/**
		 * Emitted when @a set_reconstruct_scalar_coverage_params has been called (if a change detected).
		 */
		void
		modified_reconstruct_scalar_coverage_params(
				GPlatesAppLogic::ReconstructScalarCoverageLayerParams &layer_params);

	private:

		//! Typedef for scalar types to scalar statistics.
		typedef std::map<
				GPlatesPropertyValues::ValueObjectType,
				// Statistics are only none if there are no scalars...
				boost::optional<GPlatesPropertyValues::ScalarCoverageStatistics> >
						scalar_statistics_map_type;


		ReconstructScalarCoverageParams d_reconstruct_scalar_coverage_params;

		ReconstructScalarCoverageLayerProxy::non_null_ptr_type d_layer_proxy;

		mutable scalar_statistics_map_type d_cached_scalar_statistics;

		/**
		 * Detect any changes in the layer proxy (due to changes in its dependencies).
		 *
		 * We need this so we can update our layer params since we don't get notified *directly* of
		 * changes in the Reconstruct layer that our Reconstruct Scalar Coverage layer is connected to.
		 * For example, if the scalar coverage features are reloaded from file they might no longer
		 * contain the currently selected scalar type.
		 */
		GPlatesUtils::ObserverToken d_layer_proxy_observer_token;


		explicit
		ReconstructScalarCoverageLayerParams(
				const ReconstructScalarCoverageLayerProxy::non_null_ptr_type &layer_proxy) :
			d_layer_proxy(layer_proxy)
		{  }


		/**
		 * Creates the scalar statistics across all scalar coverages of the specified scalar type,
		 * or returns none if no coverages.
		 *
		 * Note: This statistic includes the time history of evolved scalar values (where applicable).
		 */
		boost::optional<GPlatesPropertyValues::ScalarCoverageStatistics>
		create_scalar_statistics(
				const GPlatesPropertyValues::ValueObjectType &scalar_type) const;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTSCALARCOVERAGELAYERPARAMS_H
