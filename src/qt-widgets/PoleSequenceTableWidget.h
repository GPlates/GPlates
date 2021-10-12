/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_POLESEQUENCETABLEWIDGET_H
#define GPLATES_QTWIDGETS_POLESEQUENCETABLEWIDGET_H

#include <QWidget>
#include "PoleSequenceTableWidgetUi.h"
#include "model/FeatureHandle.h"

namespace GPlatesQtWidgets
{
	class PoleSequenceTableWidget:
		public QWidget,
		protected Ui_PoleSequenceTableWidget
	{
		Q_OBJECT
	public:
	
		struct PoleSequenceInfo
		{
			GPlatesModel::FeatureHandle::weak_ref d_trs;
			unsigned long d_fixed_plate;
			unsigned long d_moving_plate;
			double d_begin_time;
			double d_end_time;
			bool d_adjusted_plate_is_fixed_plate_in_seq;

			PoleSequenceInfo(
				const GPlatesModel::FeatureHandle::weak_ref &trs,
				unsigned long fixed_plate,
				unsigned long moving_plate,
				const double &begin_time,
				const double &end_time,
				bool adjusted_plate_is_fixed_plate_in_seq):
			d_trs(trs),
				d_fixed_plate(fixed_plate),
				d_moving_plate(moving_plate),
				d_begin_time(begin_time),
				d_end_time(end_time),
				d_adjusted_plate_is_fixed_plate_in_seq(adjusted_plate_is_fixed_plate_in_seq)
			{  }
		};

		struct ColumnNames
		{
			enum
			{
				FIXED_PLATE,
				MOVING_PLATE,
				BEGIN_TIME,
				END_TIME,
				NUM_COLS
			};
		};

	
		explicit
		PoleSequenceTableWidget(
			QWidget *parent_ = NULL);
			
		
	
	};
}

#endif //GPLATES_QTWIDGETS_POLESEQUENCETABLEWIDGET_H 

