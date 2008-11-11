/* $Id$ */

/**
 * \file 
 * Interface for writing features.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_FEATUREWRITER_H
#define GPLATES_FILEIO_FEATUREWRITER_H

namespace GPlatesModel
{
	class FeatureHandle;
}

namespace GPlatesFileIO
{
	/**
	 * Interface for writing features.
	 */
	class  FeatureWriter
	{
	public:
		virtual
			~FeatureWriter()
		{  }

		/**
		 * Writes a feature to a file (the format is defined in derived class).
		 *
		 * @param feature_handle feature to write
		 */
		virtual
			void
			write_feature(const GPlatesModel::FeatureHandle& feature_handle) = 0;
	};
}

#endif // GPLATES_FILEIO_FEATUREWRITER_H
