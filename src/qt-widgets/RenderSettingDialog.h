/* $Id: ColouringDialog.h 10521 2010-12-11 06:33:37Z elau $ */

/**
 * \file 
 * $Revision: 10521 $
 * $Date: 2010-12-11 17:33:37 +1100 (Sat, 11 Dec 2010) $ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_RENDERSETTINGDIALOG_H
#define GPLATES_QTWIDGETS_RENDERSETTINGDIALOG_H

#include "RenderSettingDialogUi.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	class ColourSchemeContainer;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class GlobeAndMapWidget;
	class ReadErrorAccumulationDialog;

	class RenderSettingDialog  : 
			public QDialog, 
			protected Ui_RenderSettingDialog 
	{
		Q_OBJECT

	public:
		RenderSettingDialog(
			GPlatesPresentation::ViewState &view_state,
			const GlobeAndMapWidget &existing_globe_and_map_widget,
			ReadErrorAccumulationDialog &read_error_accumulation_dialog,
			QWidget* parent_ = NULL)
		{
			setupUi(this);
		}

	private slots:

	private:
	};
}

#endif  // GPLATES_QTWIDGETS_RENDERSETTINGDIALOG_H
