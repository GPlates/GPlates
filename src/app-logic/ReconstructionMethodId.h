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
#ifndef GPLATES_APPLOGIC_RECONSTRUCTIONMETHODID_H
#define GPLATES_APPLOGIC_RECONSTRUCTIONMETHODID_H

#include "boost/assign.hpp"

#include <map>

#include <QString>

namespace GPlatesAppLogic{
		/*
		* enumeration for reconstruction methods
		*/
		enum ReconstructionMethod
		{
			BY_PLATE_ID,
			HALF_STAGE_ROTATION
		};
		
		struct ReconstructionMethodMap
		{
			const ReconstructionMethod enum_index;
			const QString str_value;
		};
	
		static const std::map<
				ReconstructionMethod, 
				QString> recon_method_map = 
			boost::assign::map_list_of 
				
			(BY_PLATE_ID, "ByPlateId") 
			(HALF_STAGE_ROTATION,"HalfStageRotation");
}
	
#endif







