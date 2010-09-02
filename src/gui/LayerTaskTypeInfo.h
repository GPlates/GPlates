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

#ifndef GPLATES_GUI_LAYERTASKTYPEINFO_H
#define GPLATES_GUI_LAYERTASKTYPEINFO_H

#include <QString>
#include <QIcon>

#include "Colour.h"

#include "app-logic/LayerTaskType.h"


namespace GPlatesGui
{
	namespace LayerTaskTypeInfo
	{
		/**
		 * Returns a human-readable name for the given layer task type.
		 */
		const QString &
		get_name(
				GPlatesAppLogic::LayerTaskType::Type layer_type);

		/**
		 * Returns a human-readable description for the given layer task type.
		 */
		const QString &
		get_description(
				GPlatesAppLogic::LayerTaskType::Type layer_type);

		/**
		 * Returns the colour associated with the given layer task type.
		 */
		const Colour &
		get_colour(
				GPlatesAppLogic::LayerTaskType::Type layer_type);

		/**
		 * Returns an icon associated with the given layer task type.
		 */
		const QIcon &
		get_icon(
				GPlatesAppLogic::LayerTaskType::Type layer_type);
	}
}

#endif // GPLATES_GUI_LAYERTASKTYPEINFO_H
