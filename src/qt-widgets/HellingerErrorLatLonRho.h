/* $Id: HellingerErrorLatLonRho.h 132 2012-01-24 15:39:28Z juraj.cirbus $ */

/**
 * \file 
 * $Revision: 132 $
 * $Date: 2012-01-24 16:39:28 +0100 (Tue, 24 Jan 2012) $ 
 * 
 * Copyright (C) 2012 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_HELLINGERERRORLATLONRHO_H
#define GPLATES_QTWIDGETS_HELLINGERERRORLARLONRHO_H

#include <QWidget>

#include "HellingerErrorLatLonRhoUi.h"
#include "HellingerDialog.h"
#include "HellingerModel.h"

namespace GPlatesQtWidgets
{
    class HellingerDialog;    
    class HellingerModel;
    class HellingerErrorLatLonRho:
			public QDialog,
            protected Ui_HellingerErrorLatLonRho
	{
		Q_OBJECT
	public:

        HellingerErrorLatLonRho(
                        HellingerDialog *hellinger_dialog,
                        HellingerModel *hellinger_model,
            QWidget *parent_ = NULL);


	private Q_SLOTS: 
        void
        continue_process();
        void
        close_application();



	private:
	
		void
		update_buttons();
        HellingerDialog *d_hellinger_dialog_ptr;
        HellingerModel *d_hellinger_model_ptr;
        bool d_process;
	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERERRORLATLONRHO_H
