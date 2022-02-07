/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_VISUALLAYERINPUTCHANNELNAME_H
#define GPLATES_PRESENTATION_VISUALLAYERINPUTCHANNELNAME_H

#include <QString>

#include "app-logic/LayerInputChannelName.h"


namespace GPlatesPresentation
{
	namespace VisualLayerInputChannelName
	{
		/**
		 * The visual layer input channel names.
		 *
		 * There is one input channel *string* name associated with each
		 * GPlatesAppLogic::LayerInputChannelName enumeration value.
		 *
		 * These strings are used in the GUI interface.
		 */
		QString
		get_input_channel_name(
				GPlatesAppLogic::LayerInputChannelName::Type layer_input_channel_name);
	}
}

#endif // GPLATES_PRESENTATION_VISUALLAYERINPUTCHANNELNAME_H
