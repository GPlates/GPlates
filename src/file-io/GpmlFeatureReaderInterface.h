/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_GPMLFEATUREREADERINTERFACE_H
#define GPLATES_FILE_IO_GPMLFEATUREREADERINTERFACE_H

#include "GpmlFeatureReaderImpl.h"

#include "model/FeatureHandle.h"
#include "model/XmlNode.h"


namespace GPlatesFileIO
{
	namespace GpmlReaderUtils
	{
		struct ReaderParams;
	}

	/**
	 * Interface class for reading a feature from a GPML file using a @a GpmlFeatureReaderImpl implementation.
	 */
	class GpmlFeatureReaderInterface
	{
	public:

		//! Construct from a feature reader implementation.
		explicit
		GpmlFeatureReaderInterface(
				const GpmlFeatureReaderImpl::non_null_ptr_type &impl);


		/**
		 * Creates and reads a feature from the specified feature XML element node.
		 */
		GPlatesModel::FeatureHandle::non_null_ptr_type
		read_feature(
				const GPlatesModel::XmlElementNode::non_null_ptr_type &feature_xml_element,
				GpmlReaderUtils::ReaderParams &reader_params) const;

	private:

		GpmlFeatureReaderImpl::non_null_ptr_type d_impl;

	};
}

#endif // GPLATES_FILE_IO_GPMLFEATUREREADERINTERFACE_H
