/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_RECONSTRUCTTOTIMEDIALOG_H
#define GPLATES_GUI_RECONSTRUCTTOTIMEDIALOG_H

#include <QDialog>
#include "ReconstructToTimeDialogUi.h"


namespace GPlatesGui
{
	class ReconstructToTimeDialog: 
			public QDialog,
			protected Ui_ReconstructToTimeDialog 
	{
		Q_OBJECT
		
	public:
		ReconstructToTimeDialog(
				const double &init_value,
				QWidget *parent_ = NULL);

		virtual
		~ReconstructToTimeDialog()
		{  }

	public slots:
		void
		change_value(
				double new_value)
		{
			d_value = new_value;
		}

		void
		propagate_value()
		{
			emit value_changed(d_value);
		}

	signals:
		void
		value_changed(
				double new_value);

	private:
		double d_value;
	};
}

#endif
