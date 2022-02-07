/* $Id: DataSelector.h 11574 2011-05-17 07:46:29Z mchin $ */

/**
 * \file 
 * $Revision: 11574 $
 * $Date: 2011-05-17 17:46:29 +1000 (Tue, 17 May 2011) $
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

#ifndef GPLATESDATAMINING_COREGFILTERCACHE_H
#define GPLATESDATAMINING_COREGFILTERCACHE_H

#include <vector>

#include "CoRegConfigurationTable.h"

#include "app-logic/ReconstructContext.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesDataMining
{
	class CoRegFilterCache
	{
	public:
		typedef std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature>
				reconstructed_feature_vector_type;


		void
		insert(
				const ConfigurationTableRow& key,
				const reconstructed_feature_vector_type &value)
		{
			d_data.push_back(CacheItem(key,value));
		}

		bool
		find(
				const ConfigurationTableRow& key,
				reconstructed_feature_vector_type &value);

		void
		insert(const ConfigurationTableRow& key)
		{
			d_data.push_back(CacheItem(key));
		}

		const reconstructed_feature_vector_type &
		front_value()
		{
			return d_data.front().d_value;
		}

		const ConfigurationTableRow&
		front_key()
		{
			return d_data.front().d_key;
		}

	private:
		class CacheItem
		{
		public:
			CacheItem(
					const ConfigurationTableRow k,
					const reconstructed_feature_vector_type &v) : 
				d_key(k),
				d_value(v)
			{ }

			CacheItem(const ConfigurationTableRow k) :
				d_key(k)
			{ }

			ConfigurationTableRow d_key;
			reconstructed_feature_vector_type d_value;
		};
		std::vector<CacheItem> d_data;
	};
}

#endif// GPLATESDATAMINING_COREGFILTERCACHE_H





