/* $Id$ */

/**
 * \file Dialog for handling exporting of reconstruction.
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

#ifndef GPLATES_QTWIDGETS_EXPORTRECONSTRUCTIONDIALOG_H
#define GPLATES_QTWIDGETS_EXPORTRECONSTRUCTIONDIALOG_H

#include <QWidget>

#include "SaveFileDialog.h"
#include "model/types.h"
#include "view-operations/VisibleReconstructedFeatureGeometryExport.h"

namespace GPlatesModel
{
	class Reconstruction;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesQtWidgets
{
	/**
	 * Dialog for handling exporting of reconstruction.
	 * Currently this isn't actually a dialog (doesn't inherit from QDialog).
	 * It is here because it uses Qt widgets (file save dialog) and can be
	 * turned into a dialog if more input from user is needed.
	 */
	class ExportReconstructedFeatureGeometryDialog
	{

	public:
		ExportReconstructedFeatureGeometryDialog(
				QWidget *parent_ = NULL);

		/**
		 * Requests input from user and exports @a reconstruction to a file.
		 * Only those @a ReconstructionFeatureGeometry objects that are visible in
		 * @a rendered_geom_collection are exported.
		 */
		void
		export_visible_reconstructed_feature_geometries(
				const GPlatesModel::Reconstruction &reconstruction,
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesViewOperations::VisibleReconstructedFeatureGeometryExport::
						files_collection_type &active_reconstructable_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);

	private:

		boost::shared_ptr<SaveFileDialog> d_save_file_dialog_ptr;

	};
}

#endif // GPLATES_QTWIDGETS_EXPORTRECONSTRUCTIONDIALOG_H
