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
#include <QFileDialog>

#include "model/types.h"
#include "view-operations/ExportReconstructedFeatureGeometries.h"


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
		 * the RECONSTRUCTION_LAYER of @a rendered_geom_collection are exported.
		 */
		void
		export_visible_reconstructed_feature_geometries(
				const GPlatesModel::Reconstruction &reconstruction,
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesViewOperations::ExportReconstructedFeatureGeometries::
						active_files_collection_type &active_reconstructable_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);


	private:
		/**
		 * A QFileDialog instance that we use for specifying the destination file.
		 * We keep it as a member so that it will remember where the user last
		 * saved to, for convenience.
		 *
		 * Memory managed by Qt.
		 */
		QFileDialog *d_export_file_dialog;
	};
}

#endif // GPLATES_QTWIDGETS_EXPORTRECONSTRUCTIONDIALOG_H
