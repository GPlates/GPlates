/* $Id$ */


/**
 * \file Allows user to modify newly loaded (or reloaded) feature collections
 * (for example, to assign plate ids to features that don't have them).
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
 
#ifndef GPLATES_GUI_MODIFYLOADEDFEATURECOLLECTIONSFILTER_H
#define GPLATES_GUI_MODIFYLOADEDFEATURECOLLECTIONSFILTER_H

#include <boost/noncopyable.hpp>
#include <QWidget>

#include "app-logic/FeatureCollectionFileIO.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class AssignReconstructionPlateIdsDialog;
}

namespace GPlatesGui
{
	class ModifyLoadedFeatureCollectionsFilter :
			public GPlatesAppLogic::FeatureCollectionFileIO::ModifyFilter,
			private boost::noncopyable
	{
	public:
		ModifyLoadedFeatureCollectionsFilter(
			GPlatesAppLogic::ApplicationState &application_state,
			GPlatesPresentation::ViewState &view_state,
			QWidget *assign_plate_ids_dialog_parent);

		/**
		 * Modify the loaded (or reloaded) feature collections in place.
		 *
		 * Currently this looks for non-reconstruction features that are missing the
		 * 'reconstructionPlateId' property and displays a GUI to allow the user to
		 * choose which loaded files should be assigned plate ids.
		 *
		 * Only those features in the feature collection(s) that have no plate id
		 * are assigned plate ids.
		 *
		 * If no TopologicalClosedPlateBoundary features are previously loaded then
		 * nothing happens since those features are required to assign plate ids
		 * (they are the plate boundaries).
		 */
		virtual
		void
		modify_loaded_files(
				const file_seq_type &loaded_or_reloaded_files);

	private:
		/**
		 * Dialog to allow user to select/clear files for assigning plate id.
		 * Memory is managed by parent QWidget.
		 */
		GPlatesQtWidgets::AssignReconstructionPlateIdsDialog *d_assign_recon_plate_ids_dialog_ptr;
	};
}

#endif // GPLATES_GUI_MODIFYLOADEDFEATURECOLLECTIONSFILTER_H
