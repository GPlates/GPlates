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

#ifndef GPLATES_APP_LOGIC_SCRIBEEXPORTAPPLOGIC_H
#define GPLATES_APP_LOGIC_SCRIBEEXPORTAPPLOGIC_H

#include "FeatureCollectionFileState.h"
#include "LayerProxy.h"
#include "ReconstructGraphImpl.h"
#include "ReconstructionLayerProxy.h"
#include "ReconstructionLayerTask.h"
#include "ReconstructLayerProxy.h"
#include "ReconstructLayerTask.h"


/**
 * Scribe export registered classes/types in the 'app-logic' source sub-directory.
 *
 * See "ScribeExportRegistration.h" for more details.
 *
 *******************************************************************************
 * WARNING: Changing the string ids will break backward/forward compatibility. *
 *******************************************************************************
 */
#define SCRIBE_EXPORT_APP_LOGIC \
		\
		((GPlatesAppLogic::FeatureCollectionFileState::FeatureCollectionUnloadCallback, \
			"FeatureCollectionFileState::FeatureCollectionUnloadCallback")) \
		\
		((GPlatesAppLogic::FeatureCollectionFileState::file_reference, \
			"FeatureCollectionFileState::file_reference")) \
		((GPlatesAppLogic::FeatureCollectionFileState::const_file_reference, \
			"FeatureCollectionFileState::const_file_reference")) \
		\
		((GPlatesAppLogic::LayerProxy::non_null_ptr_type, "LayerProxy::non_null_ptr_type")) \
		\
		((GPlatesAppLogic::ReconstructionLayerProxy, "ReconstructionLayerProxy")) \
		\
		((GPlatesAppLogic::ReconstructionLayerTask, "ReconstructionLayerTask")) \
		\
		((GPlatesAppLogic::ReconstructLayerProxy, "ReconstructLayerProxy")) \
		\
		((GPlatesAppLogic::ReconstructLayerTask, "ReconstructLayerTask")) \
		\
		((GPlatesAppLogic::ReconstructGraphImpl::LayerInputConnection::FeatureCollectionModified, \
			"ReconstructGraphImpl::LayerInputConnection::FeatureCollectionModified")) \
		\


#endif // GPLATES_APP_LOGIC_SCRIBEEXPORTAPPLOGIC_H
