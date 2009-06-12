/* $Id$ */

/**
 * \file 
 * Interface for writing features.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#include "model/FeatureHandle.h"
#include "model/FeatureCollectionHandle.h"


namespace GPlatesFileIO
{
	/**
	 * Interface for writing features.
	 *
	 * FIXME:  Is this class really necessary in addition to ConstFeatureVisitor?
	 * FIXME:  Should this class really be an ABC?  Why not just contain a ConstFeatureVisitor?
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
		write_feature(
				const GPlatesModel::FeatureHandle::const_weak_ref &feature) = 0;

		/**
		 * Writes a feature to a file (the format is defined in derived class).
		 *
		 * @param feature_handle feature to write
		 */
		virtual
		void
		write_feature(
				const GPlatesModel::FeatureCollectionHandle::features_const_iterator &feature) = 0;
	};
}

#endif // GPLATES_FILEIO_FEATUREWRITER_H
