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
 
#ifndef GPLATES_PRESENTATION_VISUALLAYERTYPE_H
#define GPLATES_PRESENTATION_VISUALLAYERTYPE_H

#include "app-logic/LayerTaskType.h"


namespace GPlatesPresentation
{
	/**
	 * An enumeration of visual layer types. This enumeration exists separately and
	 * in addition to the enumeration of app-logic layer task types, to allow for
	 * the existence of visual layers that are not backed by an app-logic layer.
	 *
	 * Even though they are not explicitly defined, the first values of this
	 * enumeration are taken to be all of the members of the enumeration
	 * GPlatesAppLogic::LayerTaskType::Type. As such, the first explicitly defined
	 * member of this enumeration must have the value of
	 * GPlatesAppLogic::LayerTaskType::NUM_TYPES to avoid conflict between values.
	 *
	 * Note that while it is always safe to convert from a
	 * GPlatesAppLogic::LayerTaskType::Type to a VisualLayerType::Type, the reverse
	 * may result in undefined behaviour (see Stroustrup, section 4.8).
	 */
	namespace VisualLayerType
	{
		enum Type
		{
			GRATICULES = static_cast<unsigned int>(GPlatesAppLogic::LayerTaskType::NUM_TYPES), // See note above.
			SPHERE,

			NUM_TYPES // This must be the last entry.
		};
	}
}

#endif // GPLATES_PRESENTATION_VISUALLAYERTYPE_H
