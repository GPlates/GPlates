/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_PRESENTATION_VISUALLAYERGROUP_H
#define GPLATES_PRESENTATION_VISUALLAYERGROUP_H


namespace GPlatesPresentation
{
	/**
	 * An enumeration of the groups that visual layers can be placed into.
	 * Groups are used to annotate which visual layer types should be visually
	 * adjacent to each other.
	 *
	 * The order in the enumeration is significant. The groups are set out in the
	 * order in which visual layers are automatically added to the @a VisualLayers.
	 * But note that the order in the @a VisualLayers is opposite to how it is
	 * displayed on screen, so the first group in the enumeration contains visual
	 * layers that visually appear at the bottom on screen.
	 */
	namespace VisualLayerGroup
	{
		enum Type
		{
			RASTERS,
			DERIVED_DATA,
			BASIC_DATA,

			NUM_GROUPS // Must be the last entry.
		};
	}
}

#endif // GPLATES_PRESENTATION_VISUALLAYERGROUP_H
