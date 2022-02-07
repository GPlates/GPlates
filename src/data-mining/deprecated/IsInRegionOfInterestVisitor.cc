/* $Id$ */

/**
 * \file .
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

#include "IsInRegionOfInterestVisitor.h"
#include "DualGeometryVisitor.h"
#include "IsCloseEnoughChecker.h"


bool
GPlatesDataMining::is_close_enough(
		const GPlatesMaths::GeometryOnSphere& g1, 
		const  GPlatesMaths::GeometryOnSphere& g2,
		const double range)
{
	IsCloseEnoughChecker checker(range);
	DualGeometryVisitor< IsCloseEnoughChecker > dual_visitor(g1,g2,&checker);
	checker.is_close_enough();

	//we have to do multiple dispatch here(more than double)
	IsInReigonOfInterestDispatchVisitor is_in_ROI_visitor(&g1, range);
	g2.accept_visitor(is_in_ROI_visitor);
	
	if( is_in_ROI_visitor.is_in_reigon_of_interest() )
	{
		return true;
	}
	else
	{
		return false;
	}
}





