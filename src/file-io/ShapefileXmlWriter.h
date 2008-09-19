/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_SHAPEFILEXMLWRITER_H
#define GPLATES_FILEIO_SHAPEFILEXMLWRITER_H

#include <QMap>
#include <QXmlStreamWriter>

namespace GPlatesFileIO{

	class ShapefileXmlWriter:
		public QXmlStreamWriter
	{
	public:
		ShapefileXmlWriter();

		/**
		 * Writes the data contained in <map> as an xml file. Returns true on success. 
		 */ 
		bool 
		write_file(
			QString &filename,
			QMap<QString,QString> *map);

	private:

		// Copy-constructor is made private
		ShapefileXmlWriter(
			const ShapefileXmlWriter &other);

		// Assignment is made private.
		ShapefileXmlWriter &
			operator=(
				const ShapefileXmlWriter &other);

		/**
		 * Writes a map item to the xml file. 
		 */
		void 
		write_map_item(
			QString key,
			QString value);

	};
}

#endif // GPLATES_FILEIO_SHAPEFILEXMLWRITER_H

