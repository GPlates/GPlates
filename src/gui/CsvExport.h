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

#ifndef GPLATES_GUI_CSVEXPORT_H
#define GPLATES_GUI_CSVEXPORT_H

namespace GPlatesGui {

	/**
	 * Class for exporting data in csv (comma-separated value) format files. 
	 */
	class CsvExport
	{
	public:
		/**
		 * This struct is used to specify what variant of CSV to write.
		 */
		struct ExportOptions
		{
			char delimiter;
			// TODO: other possible options include:
			//       ignore locale settings (tricky as TotalReconstructionPolesDialog is currently hard-coding them into strings)
			//       write table headings
			//       quote character
			//       how to escape quote character
		};

		/**
		 * Export the contents of the QTableWidget table to the file filename in csv form. 
		 */
		static
		void
		export_table(
				const QString &filename,
				const ExportOptions &options,
				const QTableWidget &table);

	};

}

#endif // GPLATES_GUI_CSVEXPORT_H
