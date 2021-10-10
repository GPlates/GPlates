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

#ifndef GPLATES_FILEIO_SHAPEFILEXMLREADER_H
#define GPLATES_FILEIO_SHAPEFILEXMLREADER_H

#include <QXmlStreamReader>
#include <QMap>
#include <QString> 

namespace GPlatesFileIO{

	class ShapefileXmlReader : public QXmlStreamReader
	{
	public:
		ShapefileXmlReader();

		/**
		 * Fills the map with data extracted from the xml file. 
		 */
		bool 
		read_file(
			QString &filename,
			QMap<QString,QString> *map);

	private:

		// Copy-constructor is private.
		ShapefileXmlReader(
			const ShapefileXmlReader &other);

		// Assignment operator is private.
		ShapefileXmlReader &
			operator=(
				const ShapefileXmlReader &other);

		/**
		 * Read the xml data.
		 */
		void 
		read_xml();

		/**
		 * Read an individual <QString,QString> pair and add it to d_map. 
		 */
		void 
		read_map_item();

		// A pointer to the map to be filled. 
		QMap<QString,QString> *d_map;

	};

} // namespace GPlatesFileIO

#endif // GPLATES_FILEIO_SHAPEFILEXMLREADER_H

