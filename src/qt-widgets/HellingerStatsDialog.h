/* $Id: HellingerStatsDialog.h 240 2012-02-27 16:30:31Z robin.watson@ngu.no $ */

/**
 * \file 
 * $Revision: 240 $
 * $Date: 2012-02-27 17:30:31 +0100 (Mon, 27 Feb 2012) $ 
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
 
#ifndef GPLATES_QTWIDGETS_HELLINGERSTATSDIALOG_H
#define GPLATES_QTWIDGETS_HELLINGERSTATSDIALOG_H

#include <QWidget>

#include "HellingerStatsDialogUi.h"

namespace GPlatesQtWidgets
{
    class HellingerDialog;

	class HellingerStatsDialog:
			public QDialog,
			protected Ui_HellingerStatsDialog
	{
		Q_OBJECT
	public:

		HellingerStatsDialog(
			const QString &python_path,
			QWidget *parent_ = NULL);
                
		void
		update();

	
	private Q_SLOTS: 

		void
        save_file();

	private:
		QString d_python_path;
	
	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERSTATSDIALOG_H
