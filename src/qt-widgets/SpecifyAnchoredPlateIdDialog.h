/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_SPECIFYANCHOREDPLATEIDDIALOG_H
#define GPLATES_QTWIDGETS_SPECIFYANCHOREDPLATEIDDIALOG_H

#include <QDialog>
#include <QMenu>

#include "ui_SpecifyAnchoredPlateIdDialogUi.h"

#include "GPlatesDialog.h"

#include "model/FeatureHandle.h"
#include "model/types.h"


namespace GPlatesQtWidgets
{
	class SpecifyAnchoredPlateIdDialog : 
			public GPlatesDialog,
			protected Ui_SpecifyAnchoredPlateIdDialog 
	{
		Q_OBJECT
		
	public:

		explicit
		SpecifyAnchoredPlateIdDialog(
				QWidget *parent_ = NULL);

		/**
		 * Call this function before showing the dialog to repopulate its fields with
		 * the latest values.
		 */
		void
		populate(
				GPlatesModel::integer_plate_id_type plate_id,
				const GPlatesModel::FeatureHandle::weak_ref &focused_feature);

	protected:

		virtual
		void
		showEvent(
				QShowEvent *ev);

	private Q_SLOTS:

		void
		propagate_value();

		void
		handle_action_triggered(
				QAction *action);

		void
		reset_to_zero();

	Q_SIGNALS:

		void
		value_changed(
				GPlatesModel::integer_plate_id_type new_value);

	private:

		void
		populate_spinbox(
				GPlatesModel::integer_plate_id_type plate_id);

		void
		populate_menu(
				const GPlatesModel::FeatureHandle::weak_ref &focused_feature);

		QMenu *d_fill_menu;
	};
}

#endif  // GPLATES_QTWIDGETS_SPECIFYANCHOREDPLATEIDDIALOG_H
