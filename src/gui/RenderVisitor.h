/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GUI_RENDERVISITOR_H_
#define _GPLATES_GUI_RENDERVISITOR_H_

#include "geo/Visitor.h"
#include "geo/PointData.h"
#include "geo/LineData.h"
#include "geo/DataGroup.h"

namespace GPlatesGui
{
	class RenderVisitor : public GPlatesGeo::Visitor
	{
		public:
			RenderVisitor() {  }

			virtual void
			Visit(const GPlatesGeo::PointData&);

			virtual void
			Visit(const GPlatesGeo::LineData&);

			virtual void
			Visit(const GPlatesGeo::DataGroup&);
	};
}

#endif  /* _GPLATES_GUI_RENDERVISITOR_H_ */
