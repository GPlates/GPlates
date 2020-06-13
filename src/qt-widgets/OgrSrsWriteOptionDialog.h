/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2016 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_OGRSRSWRITEOPTIONDIALOG_H
#define GPLATES_QTWIDGETS_OGRSRSWRITEOPTIONDIALOG_H

#include <QDialog>

#include "property-values/SpatialReferenceSystem.h"

#include "ui_OgrSrsWriteOptionDialogUi.h"


namespace GPlatesQtWidgets
{
	class OgrSrsWriteOptionDialog:
		public QDialog,
		protected Ui_OgrSrsWriteOptionDialog
	{
	Q_OBJECT

	public:

	enum BehaviourRequested
	{
		WRITE_TO_WGS84_SRS,
		WRITE_TO_ORIGINAL_SRS,
		DO_NOT_WRITE
	};

	explicit
	OgrSrsWriteOptionDialog(
		QWidget *parent = NULL);

	void
	initialise(
			const QString &filename,
			const GPlatesPropertyValues::SpatialReferenceSystem::non_null_ptr_to_const_type &srs);

	private:

	void
	set_up_connections();


	private Q_SLOTS:

	void
	handle_ok();

	void
	handle_cancel();

	};
}

#endif //GPLATES_QTWIDGETS_OGRSRSWRITEOPTIONDIALOG_H
