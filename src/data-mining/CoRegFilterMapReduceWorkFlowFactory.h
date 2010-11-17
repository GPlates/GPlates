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
#ifndef GPLATESDATAMINING_COREGFILTERMAPREDUCEWORKFLOWFACTORY_H
#define GPLATESDATAMINING_COREGFILTERMAPREDUCEWORKFLOWFACTORY_H

#include <vector>

#include <boost/shared_ptr.hpp>

namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesDataMining
{
	struct ConfigurationTableRow;
	class CoRegFilterMapReduceWorkFlow;

	/*
	* TODO: comments
	*/
	class FilterMapReduceWorkFlowFactory
	{
	public:
		static
		boost::shared_ptr< CoRegFilterMapReduceWorkFlow >
		create(
				const ConfigurationTableRow& row,
				const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& seed_geos);
	};
}

#endif



