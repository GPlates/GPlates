/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_APPLICATIONSTATE_H
#define GPLATES_APP_LOGIC_APPLICATIONSTATE_H

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

#include "model/ModelInterface.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: Please use forward declarations (and boost::scoped_ptr) instead of including headers
// where possible.
// This header gets included in a lot of other files and we want to reduce compile times.
////////////////////////////////////////////////////////////////////////////////////////////////


namespace GPlatesAppLogic
{
	class FeatureCollectionFileIO;
	class FeatureCollectionFileState;


	/**
	 * Stores state that deals with the model and feature collection files.
	 */
	class ApplicationState :
			private boost::noncopyable
	{
	public:
		ApplicationState();

		~ApplicationState();

		GPlatesModel::ModelInterface &
		get_model_interface()
		{
			return d_model;
		}


		/**
		 * Keeps track of active feature collections loaded from files.
		 */
		GPlatesAppLogic::FeatureCollectionFileState &
		get_feature_collection_file_state();


		/**
		 * Handling reading/writing feature collection files and notification of read errors.
		 */
		GPlatesAppLogic::FeatureCollectionFileIO &
		get_feature_collection_file_io();


	private:
		GPlatesModel::ModelInterface d_model;

		//
		// NOTE: Most of these are boost::scoped_ptr's to avoid having to include header files.
		//

		boost::scoped_ptr<GPlatesAppLogic::FeatureCollectionFileState> d_feature_collection_file_state;
		boost::scoped_ptr<GPlatesAppLogic::FeatureCollectionFileIO> d_feature_collection_file_io;
	};
}

#endif // GPLATES_APP_LOGIC_APPLICATIONSTATE_H
