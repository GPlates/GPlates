/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_APPLYRECONSTRUCTIONPOLEADJUSTMENTDIALOG_H
#define GPLATES_QTWIDGETS_APPLYRECONSTRUCTIONPOLEADJUSTMENTDIALOG_H

#include <vector>
#include <QDialog>
#include "ApplyReconstructionPoleAdjustmentDialogUi.h"
#include "model/FeatureHandle.h"
#include "maths/Rotation.h"


namespace GPlatesAppLogic
{
	class Reconstruct;
}

namespace GPlatesMaths
{
	class FiniteRotation;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class ApplyReconstructionPoleAdjustmentDialog: 
			public QDialog,
			protected Ui_ApplyReconstructionPoleAdjustmentDialog 
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
			bool d_dragged_plate_is_fixed_plate_in_seq;

			PoleSequenceInfo(
					const GPlatesModel::FeatureHandle::weak_ref &trs,
					unsigned long fixed_plate,
					unsigned long moving_plate,
					const double &begin_time,
					const double &end_time,
					bool dragged_plate_is_fixed_plate_in_seq):
				d_trs(trs),
				d_fixed_plate(fixed_plate),
				d_moving_plate(moving_plate),
				d_begin_time(begin_time),
				d_end_time(end_time),
				d_dragged_plate_is_fixed_plate_in_seq(dragged_plate_is_fixed_plate_in_seq)
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

		static
		void
		fill_in_fields_for_rotation(
				QLineEdit *lat_field_ptr,
				QLineEdit *lon_field_ptr,
				QDoubleSpinBox *angle_ptr,
				const GPlatesMaths::Rotation &r);

		explicit
		ApplyReconstructionPoleAdjustmentDialog(
				QWidget *parent_ = NULL);

		virtual
		~ApplyReconstructionPoleAdjustmentDialog()
		{  }

		void
		setup_for_new_pole(
				unsigned long moving_plate_,
				const double &current_time_,
				const std::vector<PoleSequenceInfo> &sequence_choices_,
				const GPlatesMaths::Rotation &adjustment_);

		void
		set_original_pole(
				const GPlatesMaths::FiniteRotation &fr);

		void
		set_result_pole(
				const GPlatesMaths::FiniteRotation &fr);

		void
		set_adjustment(
				const GPlatesMaths::Rotation &adjustment_);

		QString
		get_comment_line()
		{
			return line_comment->text();
		}

#if 0
	public slots:
		void
		change_value(
				int new_value)
		{
			// Since the value is a plate ID, it should always be non-negative.
			d_value = static_cast<unsigned long>(new_value);
		}

		void
		propagate_value()
		{
			emit value_changed(d_value);
		}
#endif
	protected slots:
		void
		handle_pole_sequence_selection_changed();

		void
		handle_pole_time_changed(
				double new_pole_time);

	signals:
		void
		pole_sequence_choice_changed(
				int new_choice);

		void
		pole_sequence_choice_cleared();

		void
		pole_time_changed(
				double new_pole_time);

	private:

		void
		populate_pole_sequence_table(
				const std::vector<PoleSequenceInfo> &sequence_choices_);

	};


	class AdjustmentApplicator:
			public QObject
	{
		Q_OBJECT
	public:
		explicit
		AdjustmentApplicator(
				GPlatesPresentation::ViewState &view_state,
				ApplyReconstructionPoleAdjustmentDialog &dialog);

		void
		set(
				const std::vector<ApplyReconstructionPoleAdjustmentDialog::PoleSequenceInfo> &
						sequence_choices_,
				const GPlatesMaths::Rotation &adjustment_,
				const double &pole_time_)
		{
			d_sequence_choices = sequence_choices_;
			d_adjustment = adjustment_;
			d_pole_time = pole_time_;
		}

	public slots:

		void
		handle_pole_sequence_choice_changed(
				int index);

		void
		handle_pole_sequence_choice_cleared();

		void
		handle_pole_time_changed(
				double new_pole_time);

		void
		apply_adjustment();

	signals:

		void
		have_reconstructed();

	private:
		GPlatesAppLogic::Reconstruct *d_reconstruct_ptr;

		ApplyReconstructionPoleAdjustmentDialog *d_dialog_ptr;
		// The adjustment as calculated interactively, relative to the stationary plate.
		boost::optional<GPlatesMaths::Rotation> d_adjustment;

		/// The adjustment, compensating for the motion of the fixed plate (if any).
		boost::optional<GPlatesMaths::Rotation> d_adjustment_rel_fixed;

		double d_pole_time;
		std::vector<ApplyReconstructionPoleAdjustmentDialog::PoleSequenceInfo> d_sequence_choices;
		boost::optional<int> d_sequence_choice_index;
	};
}

#endif  // GPLATES_QTWIDGETS_APPLYRECONSTRUCTIONPOLEADJUSTMENTDIALOG_H
