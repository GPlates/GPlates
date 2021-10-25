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
 
#ifndef GPLATES_APP_LOGIC_RASTERLAYERTASK_H
#define GPLATES_APP_LOGIC_RASTERLAYERTASK_H

#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "AppLogicFwd.h"
#include "LayerTask.h"
#include "LayerTaskParams.h"

#include "model/FeatureHandle.h"
#include "model/FeatureCollectionHandle.h"

#include "property-values/Georeferencing.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RawRaster.h"
#include "property-values/SpatialReferenceSystem.h"
#include "property-values/TextContent.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer task that resolves a geo-referenced raster (if it's time-dependent) and
	 * optionally reconstructs it using an input channel containing reconstructed polygons.
	 *
	 * Also there's another optional input channel referencing the output of another raster layer
	 * that is expected to be an age grid raster. The age grid raster provides improved resolution
	 * for masking those parts of the present-day globe that don't exist in the past
	 * (eg, ocean floor near mid-ocean ridge).
	 */
	class RasterLayerTask :
			public LayerTask
	{
	public:
		/**
		 * App-logic parameters for a raster layer.
		 */
		class Params :
				public LayerTaskParams
		{
		public:
			//! Sets the name of the band, of the raster, selected for processing.
			void
			set_band_name(
					const GPlatesPropertyValues::TextContent &band_name);


			//! Returns the name of the band of the raster selected for processing.
			const GPlatesPropertyValues::TextContent &
			get_band_name() const;

			//! Returns the list of band names that are in the raster feature.
			const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &
			get_band_names() const;

			//! Returns the georeferencing of the raster feature.
			const boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> &
			get_georeferencing() const;

			//! Returns the raster feature's spatial reference system.
			const boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> &
			get_spatial_reference_system() const;

			//! Returns the raster feature or boost::none if one is currently not set on the layer.
			const boost::optional<GPlatesModel::FeatureHandle::weak_ref> &
			get_raster_feature() const;

		private:
			//! The name of the band of the raster that has been selected for processing.
			GPlatesPropertyValues::TextContent d_band_name;

			//! The list of band names that were in the raster feature the last time we examined it.
			GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type d_band_names;

			//! The georeferencing of the raster.
			boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> d_georeferencing;

			//! The raster's spatial reference system.
			boost::optional<GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type> d_spatial_reference_system;

			//! The raster feature.
			boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_raster_feature;

			/**
			 * Is true if @a set_band_name has been called - RasterLayerTask will reset this explicitly.
			 *
			 * Used to let RasterLayerTask know that an external client has modified this state.
			 */
			bool d_set_band_name_called;

			Params();

			//! Modifies the selected raster band and list of raster band names according to new feature.
			void
			set_raster_feature(
					boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref);

			//! Update state to reflect a new, or modified, raster feature.
			void
			updated_raster_feature();

			// Make friend so can access constructor, @a d_set_band_name_called and private methods.
			friend class RasterLayerTask;
		};


		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		static
		boost::shared_ptr<RasterLayerTask>
		create_layer_task()
		{
			return boost::shared_ptr<RasterLayerTask>(new RasterLayerTask());
		}


		~RasterLayerTask();


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::RASTER;
		}


		virtual
		std::vector<LayerInputChannelType>
		get_input_channel_types() const;


		virtual
		QString
		get_main_input_feature_collection_channel() const;


		virtual
		void
		activate(
				bool active)
		{  }


		virtual
		void
		add_input_file_connection(
				const QString &input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		virtual
		void
		remove_input_file_connection(
				const QString &input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);

		virtual
		void
		modified_input_file(
				const QString &input_channel_name,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


		virtual
		void
		add_input_layer_proxy_connection(
				const QString &input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy);

		virtual
		void
		remove_input_layer_proxy_connection(
				const QString &input_channel_name,
				const LayerProxy::non_null_ptr_type &layer_proxy);


		virtual
		void
		update(
				const Reconstruction::non_null_ptr_type &reconstruction);


		virtual
		LayerProxy::non_null_ptr_type
		get_layer_proxy();


		virtual
		LayerTaskParams &
		get_layer_task_params()
		{
			return d_layer_task_params;
		}

	private:
		static const QString RASTER_FEATURE_CHANNEL_NAME;
		static const QString RECONSTRUCTED_POLYGONS_CHANNEL_NAME;
		static const QString AGE_GRID_RASTER_CHANNEL_NAME;
		static const QString NORMAL_MAP_RASTER_CHANNEL_NAME;


		/**
		 * Extra parameters for this layer.
		 */
		Params d_layer_task_params;

		/**
		 * Resolves raster.
		 */
		raster_layer_proxy_non_null_ptr_type d_raster_layer_proxy;


		//! Constructor.
		RasterLayerTask();
	};
}

#endif // GPLATES_APP_LOGIC_RASTERLAYERTASK_H
