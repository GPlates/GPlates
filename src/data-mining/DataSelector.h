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

#ifndef GPLATESDATAMINING_DATASELECTOR_H
#define GPLATESDATAMINING_DATASELECTOR_H

#include <vector>
#include <string>
#include <QString>
#include <map>
#include <boost/optional.hpp>
#include <boost/version.hpp>

#if BOOST_VERSION > 103600
#include <boost/unordered_map.hpp>
#endif

#include "CoRegConfigurationTable.h"
#include "CoRegFilterCache.h"
#include "DataTable.h"

#include "app-logic/LayerProxy.h"
#include "app-logic/ReconstructedFeatureGeometry.h"

#include "model/FeatureHandle.h"

#include "utils/UnicodeStringUtils.h"


namespace GPlatesOpenGL
{
	class GLRasterCoRegistration;
	class GLRenderer;
}

namespace GPlatesDataMining
{
	class DataSelector
	{
	public:

		//! Used for co-registering target rasters.
		struct RasterCoRegistration
		{
			RasterCoRegistration(
					GPlatesOpenGL::GLRenderer &renderer_,
					GPlatesOpenGL::GLRasterCoRegistration &co_registration_) :
				renderer(renderer_),
				co_registration(co_registration_)
			{  }

			GPlatesOpenGL::GLRenderer &renderer;
			GPlatesOpenGL::GLRasterCoRegistration &co_registration;
		};


		static
		boost::shared_ptr<DataSelector> 
		create(
				const CoRegConfigurationTable& table)
		{
			return boost::shared_ptr<DataSelector>(new DataSelector(table));
		}

		
		/**
		 * Given the seed and target, select() will return the associated data in DataTable.
		 *
		 * Note that @a co_register_rasters is required for *raster* co-registration since
		 * (raster co-registration is accelerated using OpenGL).
		 * If @a co_register_rasters is boost::none then any target layers that are rasters will not be co-registered.
		 */
		void
		select(
				const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &reconstructed_seed_features,	
				const std::vector<GPlatesAppLogic::LayerProxy::non_null_ptr_type> &target_layer_proxies,
				const double &reconstruction_time,
				DataTable &result_data_table,
				boost::optional<RasterCoRegistration> co_register_rasters);

		static
		void
		set_data_table(
				const DataTable& table)
		{
			d_data_table = table;
		}

		static
		const DataTable&
		get_data_table() 
		{
			return d_data_table;
		}

		void
		populate_table_header();

		~DataSelector()
		{ }
		
	protected:

		/**
		 * It's possible that some config rows might reference non-existent, or inactive, target layers
		 * in which case this method returns false.
		 *
		 * NOTE: Normally the co-registration configuration dialog will remove these rows for us but
		 * due to the effectively undefined order in which Qt slots receive signals it's possible
		 * for data co-registration to proceed (ie, an app-logic wide reconstruction is performed)
		 * before the dialog gets a chance to remove the rows.
		 */
		bool
		is_config_table_valid(
				const std::vector<GPlatesAppLogic::LayerProxy::non_null_ptr_type> &target_layer_proxies);

		void
		fill_seed_info(
				const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_seed_feature,
				DataRowSharedPtr);

		void
		co_register_target_reconstructed_rasters(
				GPlatesOpenGL::GLRenderer &renderer,
				GPlatesOpenGL::GLRasterCoRegistration &raster_co_registration,
				const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &reconstructed_seed_features,	
				const double &reconstruction_time,
				GPlatesDataMining::DataTable &result_data_table);

		void
		co_register_target_reconstructed_geometries(
				const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &reconstructed_seed_features,	
				const double &reconstruction_time,
				GPlatesDataMining::DataTable &result_data_table);

		//default constructor
		DataSelector();

		//copy constructor
		DataSelector(const DataSelector&);
		
		//assignment 
		const DataSelector&
		operator=(const DataSelector&);

		DataSelector(const CoRegConfigurationTable &table) : 
			d_cfg_table(table),
			d_data_index(0)
		{
			populate_table_header();
			if(!d_cfg_table.is_optimized())
				d_cfg_table.optimize();
		}

		CoRegConfigurationTable d_cfg_table;

		TableHeader d_table_header;
		unsigned d_data_index;
		/*
		* TODO:
		* Need to remove the "static" in the future.
		* Find a way to move result data more gracefully.
		*/
		static DataTable d_data_table;
	};
}

#endif


