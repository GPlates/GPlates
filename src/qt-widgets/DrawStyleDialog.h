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
 
#ifndef GPLATES_QTWIDGETS_DRAWSTYLEDIALOG_H
#define GPLATES_QTWIDGETS_DRAWSTYLEDIALOG_H

#include "DrawStyleDialogUi.h"
#include "gui/DrawStyleManager.h"

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

	class DrawStyleDialog  : 
			public QDialog, 
			protected Ui_DrawStyleDialog
	{
		Q_OBJECT

	public:
		DrawStyleDialog(
			GPlatesPresentation::ViewState &view_state,
			boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer,
			QWidget* parent_ = NULL);

		~DrawStyleDialog()
		{
			qDebug() << "destructing DrawStyleDialog";
		}
		
		void
		init_catagory_table();

		void
		init_dlg();

	protected:
		void
		make_signal_slot_connections();

		void
		set_style();

		void
		load_category(const GPlatesGui::StyleCatagory* );
				
	private slots:
		void
		handle_close_button_clicked();

		void
		handle_categories_table_cell_changed(
				int current_row,
				int current_column,
				int previous_row,
				int previous_column);

	private:
		static const std::size_t ICON_SIZE = 145;
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_visual_layer;
		QIcon d_blank_icon;
		GPlatesGui::DrawStyleManager* d_style_mgr;
	};
}

#endif  // GPLATES_QTWIDGETS_RENDERSETTINGDIALOG_H
