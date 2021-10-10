/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_FEATURECOLOURTABLE_H
#define GPLATES_GUI_FEATURECOLOURTABLE_H

#include <map>

#include "Colour.h"
#include "ColourTable.h"

#include "model/types.h"
#include "model/Model.h"
#include "model/FeatureHandle.h"

namespace GPlatesGui 
{
	class FeatureColourTable :
			public ColourTable
	{
	public:
		typedef std::map<
				GPlatesModel::FeatureType, 
				GPlatesGui::Colour>
				colour_map_type;

		static
		FeatureColourTable *
		Instance();

		virtual 
		~FeatureColourTable()
		{  }

		virtual
		ColourTable::const_iterator
		lookup(
				const GPlatesModel::ReconstructedFeatureGeometry &feature) const;

	protected:
		/**
		 * Private constructor to enforce singleton design.
		 */
		FeatureColourTable();

	private:
		/**
		 * The singleton instance.
		 */
		static FeatureColourTable *d_instance;

		colour_map_type d_colours;
	};		
}

#endif  /* GPLATES_GUI_FEATURECOLOURTABLE_H */
