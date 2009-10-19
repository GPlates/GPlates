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
 
#ifndef GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEDECLS_H
#define GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEDECLS_H

#include <boost/cstdint.hpp>
#include <boost/integer_traits.hpp>
#include <QString>


namespace GPlatesAppLogic
{
	namespace FeatureCollectionFileStateDecls
	{
		//! Typedef for the tag type used to identify specific workflow instances.
		typedef QString workflow_tag_type;


		/**
		 * Tag used to identify the reconstructable workflow which handles the active
		 * reconstructable feature collections.
		 *
		 * This is a built-in workflow inside @a FeatureCollectionFileState.
		 */
		static const workflow_tag_type RECONSTRUCTABLE_WORKFLOW_TAG = "reconstructable-workflow-tag";


		/**
		 * Tag used to identify the reconstruction workflow which handles the active
		 * reconstruction feature collections.
		 *
		 * This is a built-in workflow inside @a FeatureCollectionFileState.
		 */
		static const workflow_tag_type RECONSTRUCTION_WORKFLOW_TAG = "reconstruction-workflow-tag";


		/**
		 * Typedef for priority of a workflow.
		 *
		 * NOTE: Only use positive values (and zero) for your priorities as negative values
		 * are reserved for some internal workflows to allow them to get even
		 * lower priorities than the client workflows.
		 */
		typedef boost::int32_t workflow_priority_type;
	}
}

#endif // GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEDECLS_H
