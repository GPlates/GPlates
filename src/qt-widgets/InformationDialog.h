/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_INFORMATIONDIALOG_H
#define GPLATES_QTWIDGETS_INFORMATIONDIALOG_H

#include <QDialog>
#include "InformationDialogUi.h"

namespace GPlatesQtWidgets
{
	class InformationDialog : 
			public QDialog, 
			protected Ui_InformationDialog 
	{
		Q_OBJECT

	public:

		InformationDialog(
				const QString &text_,
				const QString &title_,
				QWidget *parent_ = NULL);

		void
		set_text(
				const QString &text_);

		void
		set_title(
				const QString &title_);
	};
}

#endif  // GPLATES_QTWIDGETS_INFORMATIONDIALOG_H
