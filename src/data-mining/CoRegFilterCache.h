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

namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesDataMining
{
	class CoRegFilterCache
	{
	public:
		void
		insert(
				const ConfigurationTableRow& key,
				const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& value)
		{
			d_data.push_back(CacheItem(key,value));
		}

		bool
		find(
				const ConfigurationTableRow& key,
				std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& values);

		void
		insert(const ConfigurationTableRow& key)
		{
			d_data.push_back(CacheItem(key));
		}

		std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>&
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
					std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> v) : 
				d_key(k),
				d_value(v)
			{ }

			CacheItem(const ConfigurationTableRow k) :
				d_key(k)
			{ }

			ConfigurationTableRow d_key;
			std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> d_value;
		};
		std::vector<CacheItem> d_data;
	};
}

#endif// GPLATESDATAMINING_COREGFILTERCACHE_H





