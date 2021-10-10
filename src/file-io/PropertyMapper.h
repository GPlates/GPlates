/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_PROPERTYMAPPER_H
#define GPLATES_FILEIO_PROPERTYMAPPER_H

namespace GPlatesFileIO
{
	/**
	 * An abstract base class for mapping file-io attributes to model properties. 
	 */
	class PropertyMapper
	{
	public:
		PropertyMapper()
		{ }

		virtual
		~PropertyMapper()
		{ }

		virtual
		bool
		map_properties(
				QString &filename,
				QStringList &field_names,
				QMap<QString,QString> &model_to_attribute_map,
				bool remapping
		) = 0;

	private:
		// Make copy and assignment private.
		PropertyMapper(
			const PropertyMapper &other);

		PropertyMapper &
		operator=(
			const PropertyMapper &other);
	};
}

#endif // GPLATES_FILEIO_PROPERTYMAPPER_H
