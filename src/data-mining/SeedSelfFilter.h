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
#ifndef GPLATESDATAMINING_SEEDSELFFILTER_H
#define GPLATESDATAMINING_SEEDSELFFILTER_H

#include <vector>
#include <QDebug>
#include <boost/foreach.hpp>

#include "CoRegFilter.h"

namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesDataMining
{
	class SeedSelfFilter : public CoRegFilter
	{
	public:
		class Config : public CoRegFilter::Config
		{
		public:
			CoRegFilter*
			create_filter(const CoRegFilter::RFGVector& seed)
			{
				return new SeedSelfFilter(seed);
			}

			bool
			is_same_type(const CoRegFilter::Config* other)
			{
				return dynamic_cast<const SeedSelfFilter*>(other);
			}

			const QString
			filter_name()
			{
				return "Seed";
			}

			bool
			operator< (const CoRegFilter::Config& other)
			{
				return false;
			}

			bool
			operator==(const CoRegFilter::Config&) 
			{
				return true;
			}

			~Config(){ }
		};

		explicit
		SeedSelfFilter(
				const CoRegFilter::RFGVector& seed_rfgs):
			d_seed_rfgs(seed_rfgs)
			{ }
			
		void
		process(
				CoRegFilter::RFGVector::const_iterator input_begin,
				CoRegFilter::RFGVector::const_iterator input_end,
				CoRegFilter::RFGVector& output) 
		{
			output = d_seed_rfgs;
			return;
		}

		~SeedSelfFilter(){ }

	protected:
		const CoRegFilter::RFGVector& d_seed_rfgs;
	};
}
#endif //GPLATESDATAMINING_SEEDSELFFILTER_H










