/* $Id: HellingerNewSegment.h 227 2012-02-24 14:46:55Z juraj.cirbus $ */

/**
 * \file 
 * $Revision: 227 $
 * $Date: 2012-02-24 15:46:55 +0100 (Fri, 24 Feb 2012) $ 
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
 
#ifndef GPLATES_QTWIDGETS_HELLINGERNEWSEGMENT_H
#define GPLATES_QTWIDGETS_HELLINGERNEWSEGMENT_H

#include <QAbstractTableModel>
#include <QtCore>
#include <QtGui>
#include <QWidget>

#include "HellingerNewSegmentUi.h"
#include "HellingerDialog.h"


namespace GPlatesQtWidgets
{
    enum ColumnValue{
        COLUMN_MOVE_FIX = 0,
        COLUMN_LAT,
        COLUMN_LON,
        COLUMN_ERROR
    };

    class HellingerDialog;
    class HellingerModel;
    class HellingerNewSegmentError;

        class HellingerNewSegment:
			public QDialog,
                        protected Ui_HellingerNewSegment
	{
		Q_OBJECT
	public:
                HellingerNewSegment(
                        HellingerDialog *hellinger_dialog,
                        HellingerModel *hellinger_model,
                        QWidget *parent_ = NULL);
                void
                reset();
                int d_type_new_segment_error;

	private Q_SLOTS: 

            void
            add_segment();

            void
            add_line();

            void
            remove_line();

            void
            add_segment_to_model();

            void
            change_table_stats_pick();

            void
            item_changed(QStandardItem *item);



	private:
            void
            change_quick_set_state();

            void
            update_buttons();

            HellingerDialog *d_hellinger_dialog_ptr;
            QStandardItemModel *model;
            HellingerModel *d_hellinger_model_ptr;
            HellingerNewSegmentError *d_hellinger_new_segment_error;
            int d_row_count;


	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERNEWSEGMENT_H
