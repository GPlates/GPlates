/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_QTWIDGETS_FINITEROTATIONCALCULATORDIALOG_H
#define GPLATES_QTWIDGETS_FINITEROTATIONCALCULATORDIALOG_H

#include <QWidget>

#include "FiniteRotationCalculatorDialogUi.h"

#include "GPlatesDialog.h"


namespace GPlatesQtWidgets
{
	/**
	 * Dialog containing various utilities related to finite rotation calculations.                                                                     
	 */
	class FiniteRotationCalculatorDialog:
			public GPlatesDialog,
			protected Ui_FiniteRotationCalculatorDialog
	{
		Q_OBJECT
	public:

		explicit
		FiniteRotationCalculatorDialog(
			QWidget *parent_ = NULL);
	
	protected:

		/**
		 * An event filter to change the default dialog button when the focus changes between inputs.
		 */
		virtual
		bool
		eventFilter(
				QObject *watched,
				QEvent *ev);

	private Q_SLOTS:

		void
		handle_rotate_a_point();

		void 
		handle_add_finite_rotations();

		void
		handle_subtract_finite_rotations();

		void
		handle_calc_rotation_between_points();


		void
		handle_add_finite_rotations_input_changed();

		void
		handle_subtract_finite_rotations_input_changed();

		void
		handle_calc_rotation_between_points_input_changed();

		void
		handle_rotate_a_point_input_changed();

	private:

		void
		install_event_filters();

		void
		make_signal_slot_connections();

	};
}

#endif //GPLATES_QTWIDGETS_FINITEROTATIONCALCULATORDIALOG_H
