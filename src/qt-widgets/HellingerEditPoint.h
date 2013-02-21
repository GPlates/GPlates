/* $Id: HellingerEditPoint.h 132 2012-01-24 15:39:28Z juraj.cirbus $ */

/**
 * \file 
 * $Revision: 132 $
 * $Date: 2012-01-24 16:39:28 +0100 (Tue, 24 Jan 2012) $ 
 * 
 * Copyright (C) 2011, 2012 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_HELLINGEREDITPOINT_H
#define GPLATES_QTWIDGETS_HELLINGEREDITPOINT_H

#include <QWidget>

#include "HellingerEditPointUi.h"
#include "HellingerDialog.h"

namespace GPlatesQtWidgets
{
    class HellingerDialog;
    class HellingerModel;

	class HellingerEditPoint:
			public QDialog,
			protected Ui_HellingerEditPoint
	{
		Q_OBJECT
	public:

        HellingerEditPoint(
                        HellingerDialog *hellinger_dialog,
                        HellingerModel *hellinger_model,
            QWidget *parent_ = NULL);
                void
                reset();
                void
                initialization_table(QStringList &input_value);
                void
                initialization(int &segment, int &row);
               
	private Q_SLOTS: 

            void
            edit_point();

	private:
	
		void
		update_buttons();
        HellingerDialog *d_hellinger_dialog_ptr;
        HellingerModel *d_hellinger_model_ptr;
        int d_segment;
        int d_row;


	};
}

#endif //GPLATES_QTWIDGETS_HELLINGEREDITPOINT_H
