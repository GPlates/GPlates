/* $Id: HellingerNewSegmentError.h 132 2012-01-24 15:39:28Z juraj.cirbus $ */

/**
 * \file 
 * $Revision: 132 $
 * $Date: 2012-01-24 16:39:28 +0100 (Tue, 24 Jan 2012) $ 
 * 
 * Copyright (C) 2012. 2013 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_HELLINGERNEWSEGMENTWARNING_H
#define GPLATES_QTWIDGETS_HELLINGERNEWSEGMENTWARNING_H

#include <QWidget>

#include "HellingerNewSegmentWarningUi.h"
#include "HellingerNewSegment.h"


namespace GPlatesQtWidgets
{   
	enum NewSegmentActionType
    {
		ACTION_ADD_NEW_SEGMENT = 1,
		ACTION_REPLACE_NEW_SEGMENT,
		ACTION_INSERT_NEW_SEGMENT
    };

    class HellingerDialog;
	class HellingerNewSegmentWarning:
			public QDialog,
			protected Ui_HellingerNewSegmentWarning
	{
		Q_OBJECT
	public:

		HellingerNewSegmentWarning(
				HellingerDialog *hellinger_dialog,
				const int &segment_number,
				QWidget *parent_ = NULL);

        int
        error_type_new_segment();

		/**
		 * @brief initialise_buttons
		 * Uncheck all the radio buttons so the user is forced to make a choice.
		 */
		void
		initialise_buttons();



	private Q_SLOTS: 

        void
		handle_ok();

		void
		handle_button_clicked();

	private:
	

        HellingerDialog *d_hellinger_dialog_ptr;

        int d_type_error_new_segment;

		QButtonGroup d_button_group;

		int d_segment_number;
	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERNEWSEGMENTWARNING_H
