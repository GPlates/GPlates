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
#ifndef GPLATESDATAMINING_DATAMININGCACHE_H
#define GPLATESDATAMINING_DATAMININGCACHE_H
#include <map>

namespace GPlatesDataMining
{
	enum CacheHitTypes
	{
		PERFECT_HIT,
		NEED_FURTHER_PROCESS,
		NO_HIT
	};
	/*	
	*	TODO:
	*	Comments....
	*/
	template< class Key, class Data>
	class DataMiningCache
	{
	public:
		/*
		* TODO: comments....
		*/
		virtual
		void
		insert(
				Key key,
				Data data);
		
		/*
		* TODO: comments....
		*/
		virtual
		CacheHitTypes
		query(
				Key key);

		/*
		* Clear the cache.
		*/
		inline
		void
		clear()
		{
			d_cache.clear();
		}

		virtual
		~DataMiningCache()
		{ }

	protected:
		std::map<Key, Data> d_cache;
	};
}
#endif








