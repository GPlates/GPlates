/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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
 
#include <iostream>
#include <QHeaderView>

#include "ApplyReconstructionPoleAdjustmentDialog.h"

#include "app-logic/ApplicationState.h"
#include "feature-visitors/TotalReconstructionSequenceRotationInterpolater.h"
#include "feature-visitors/TotalReconstructionSequenceRotationInserter.h"
#include "maths/MathsUtils.h"
#include "model/NotificationGuard.h"
#include "presentation/ViewState.h"


void
GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::fill_in_fields_for_rotation(
		QLineEdit *lat_field_ptr,
		QLineEdit *lon_field_ptr,
		QDoubleSpinBox *angle_ptr,
		const GPlatesMaths::Rotation &r)
{
	double rot_angle_in_rads = r.angle().dval();
	double rot_angle_in_degs = GPlatesMaths::convert_rad_to_deg(rot_angle_in_rads);
	angle_ptr->setValue(rot_angle_in_degs);

	if (r.angle() != 0.0) {
		QLocale locale_;

		const GPlatesMaths::UnitVector3D &rot_axis = r.axis();
		GPlatesMaths::PointOnSphere pos_(rot_axis);
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos_);

		lat_field_ptr->setText(locale_.toString(llp.latitude(), 'f', 2));
		lon_field_ptr->setText(locale_.toString(llp.longitude(), 'f', 2));
	} else {
		lat_field_ptr->clear();
		lon_field_ptr->clear();
	}
}


GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::ApplyReconstructionPoleAdjustmentDialog(
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setupUi(this);

	// A cell is activated when the user clicks in it, or when the user presses Enter in it.
	QObject::connect(table_pole_sequences, SIGNAL(itemSelectionChanged()),
			this, SLOT(handle_pole_sequence_selection_changed()));
	QObject::connect(spinbox_pole_time, SIGNAL(valueChanged(double)),
			this, SLOT(handle_pole_time_changed(double)));

	table_pole_sequences->horizontalHeader()->setResizeMode(ColumnNames::FIXED_PLATE,
			QHeaderView::Stretch);
	table_pole_sequences->horizontalHeader()->setResizeMode(ColumnNames::MOVING_PLATE,
			QHeaderView::Stretch);
	table_pole_sequences->horizontalHeader()->setResizeMode(ColumnNames::BEGIN_TIME,
			QHeaderView::Stretch);
	table_pole_sequences->horizontalHeader()->setResizeMode(ColumnNames::END_TIME,
			QHeaderView::Stretch);
}


void
GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::setup_for_new_pole(
		unsigned long moving_plate_,
		const double &current_time_,
		const std::vector<PoleSequenceInfo> &sequence_choices_,
		const GPlatesMaths::Rotation &adjustment_)
{
	QLocale locale_;
	// We need this static-cast because apparently QLocale's 'toString' member function doesn't
	// have an overload for 'unsigned long', so the compiler complains about ambiguity.
	lineedit_moving_plate->setText(locale_.toString(static_cast<unsigned>(moving_plate_)));

	spinbox_current_time->setValue(current_time_);
	spinbox_pole_time->setValue(current_time_);
	populate_pole_sequence_table(sequence_choices_);
}


namespace
{
	void
	fill_in_fields_for_finite_rotation(
			QLineEdit *lat_field_ptr,
			QLineEdit *lon_field_ptr,
			QDoubleSpinBox *angle_ptr,
			const GPlatesMaths::FiniteRotation &fr)
	{
		using namespace GPlatesMaths;

		// Based upon GPlatesMaths::operator<<(std::ostream &os, const FiniteRotation &fr).
		const UnitQuaternion3D &uq = fr.unit_quat();
		if (represents_identity_rotation(uq)) {
			lat_field_ptr->clear();
			lon_field_ptr->clear();
			angle_ptr->setValue(0.0);
		} else {
			QLocale locale_;

			UnitQuaternion3D::RotationParams params = uq.get_rotation_params(fr.axis_hint());
			PointOnSphere p(params.axis);
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(p);
			lat_field_ptr->setText(locale_.toString(llp.latitude(), 'f', 2));
			lon_field_ptr->setText(locale_.toString(llp.longitude(), 'f', 2));

			double rot_angle_in_rads = params.angle.dval();
			double rot_angle_in_degs = GPlatesMaths::convert_rad_to_deg(rot_angle_in_rads);
			angle_ptr->setValue(rot_angle_in_degs);
		}
	}
}


void
GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::set_original_pole(
		const GPlatesMaths::FiniteRotation &fr)
{
	fill_in_fields_for_finite_rotation(field_original_lat, field_original_lon,
			spinbox_original_angle, fr);
}


void
GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::set_result_pole(
		const GPlatesMaths::FiniteRotation &fr)
{
	fill_in_fields_for_finite_rotation(field_result_lat, field_result_lon,
			spinbox_result_angle, fr);
}


void
GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::set_adjustment(
		const GPlatesMaths::Rotation &adjustment_)
{
	fill_in_fields_for_rotation(field_adjustment_lat, field_adjustment_lon,
			spinbox_adjustment_angle, adjustment_);
}


void
GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::handle_pole_sequence_selection_changed()
{
	QList<QTableWidgetItem *> selected_items = table_pole_sequences->selectedItems();
	if (selected_items.isEmpty()) {
		// Somehow there's no selection.  I'm not really sure how this happened, but
		// anyway...
		Q_EMIT pole_sequence_choice_cleared();
	} else {
		// Since the table is configured to only allow a single selection at any time, and
		// to select by whole row, presumably all the items in this list are cells in the
		// same row.
		QTableWidgetItem *an_item = selected_items.first();
		if (an_item->row() < 0) {
			// Somehow there's no selection.  I'm not really sure how this happened,
			// but anyway...
			Q_EMIT pole_sequence_choice_cleared();
		} else {
			Q_EMIT pole_sequence_choice_changed(an_item->row());
		}
	}
}


void
GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::handle_pole_time_changed(
		double new_pole_time)
{
	Q_EMIT pole_time_changed(new_pole_time);
}


void
GPlatesQtWidgets::ApplyReconstructionPoleAdjustmentDialog::populate_pole_sequence_table(
		const std::vector<PoleSequenceInfo> &sequence_choices_)
{
	QLocale locale_;

	table_pole_sequences->clearContents();
	table_pole_sequences->setRowCount(0);

	std::vector<PoleSequenceInfo>::const_iterator iter = sequence_choices_.begin();
	std::vector<PoleSequenceInfo>::const_iterator end = sequence_choices_.end();
	for ( ; iter != end; ++iter) {
		// First, insert an empty row.  (I have no idea why we have to insert an empty row
		// before we can modify that row, but that seems to be the way it's done...)
		int row_num = table_pole_sequences->rowCount();
		table_pole_sequences->insertRow(row_num);

		// Now, insert the fixed plate ID into the appropriate column of this row.
		QString fixed_plate_id_as_text =
				locale_.toString(static_cast<unsigned>(iter->d_fixed_plate));
		QTableWidgetItem *fixed_plate_id_item =
				new QTableWidgetItem(fixed_plate_id_as_text);
		fixed_plate_id_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		table_pole_sequences->setItem(row_num, ColumnNames::FIXED_PLATE, fixed_plate_id_item);

		// Next, the moving plate ID.
		QString moving_plate_id_as_text =
				locale_.toString(static_cast<unsigned>(iter->d_moving_plate));
		QTableWidgetItem *moving_plate_id_item =
				new QTableWidgetItem(moving_plate_id_as_text);
		moving_plate_id_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		table_pole_sequences->setItem(row_num, ColumnNames::MOVING_PLATE, moving_plate_id_item);

		// Next, insert the begin time.
		QString begin_time_as_text = locale_.toString(iter->d_begin_time, 'f', 2);
		QTableWidgetItem *begin_time_item =
				new QTableWidgetItem(begin_time_as_text);
		begin_time_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		table_pole_sequences->setItem(row_num, ColumnNames::BEGIN_TIME, begin_time_item);

		// Next, insert the end time.
		QString end_time_as_text = locale_.toString(iter->d_end_time, 'f', 2);
		QTableWidgetItem *end_time_item =
				new QTableWidgetItem(end_time_as_text);
		end_time_item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
		table_pole_sequences->setItem(row_num, ColumnNames::END_TIME, end_time_item);
	}

	table_pole_sequences->resizeColumnsToContents();
	table_pole_sequences->verticalHeader()->hide();

	if ( ! sequence_choices_.empty()) {
		table_pole_sequences->selectRow(0);
	}
}


GPlatesQtWidgets::AdjustmentApplicator::AdjustmentApplicator(
		GPlatesPresentation::ViewState &view_state,
		ApplyReconstructionPoleAdjustmentDialog &dialog) :
	d_application_state_ptr(&view_state.get_application_state()),
	d_dialog_ptr(&dialog),
	d_pole_time(0.0)
{
}


void
GPlatesQtWidgets::AdjustmentApplicator::handle_pole_sequence_choice_changed(
		int index)
{
	using namespace GPlatesMaths;

	d_sequence_choice_index = index;
	if (d_sequence_choices.empty()) {
		// Apparently something has gone wrong -- this slot is being invoked when there is
		// nothing in the sequence choices container.  Hence, there's nothing we can do.
		std::cerr << __FILE__
				<< ", line "
				<< __LINE__
				<< ": sequence choices container is empty."
				<< std::endl;
		return;
	}
	if ( ! d_adjustment) {
		// Apparently something has gone wrong -- this slot is being invoked when the
		// boost::optional for the adjustment is boost::none.  Hence, there's nothing we
		// can do.
		std::cerr << __FILE__
				<< ", line "
				<< __LINE__
				<< ": adjustment is boost::none."
				<< std::endl;
		return;
	}
	if ( ! d_reconstruction_tree) {
		// Apparently something has gone wrong -- this slot is being invoked when the
		// boost::optional for the reconstruction tree is boost::none.
		// Hence, there's nothing we can do.
		std::cerr << __FILE__
				<< ", line "
				<< __LINE__
				<< ": reconstruction tree is boost::none."
				<< std::endl;
		return;
	}

	// Get the interpolated original pole.
	GPlatesModel::FeatureHandle::weak_ref chosen_pole_seq = d_sequence_choices.at(index).d_trs;
	GPlatesFeatureVisitors::TotalReconstructionSequenceRotationInterpolater interpolater(d_pole_time);
	if ( ! chosen_pole_seq.is_valid()) {
		// Nothing we can do.
		// FIXME:  Should we complain?
		return;
	}

	interpolater.visit_feature(chosen_pole_seq);
	if ( ! interpolater.result()) {
		// Again, nothing we can do.
		// FIXME:  Should we complain?
		return;
	}
	const FiniteRotation &original_pole = *interpolater.result();
	d_dialog_ptr->set_original_pole(original_pole);

	// Calculate the adjustment relative to the fixed plate.
	Rotation adjustment_rel_fixed = *d_adjustment;

	// The "fixed" plate, relative to which this plate's motion is described.
	unsigned long fixed_plate = d_sequence_choices.at(index).d_fixed_plate;
	// Of course, the "fixed" plate might be moving relative to some other plate...
	FiniteRotation motion_of_fixed_plate =
			d_reconstruction_tree.get()->get_composed_absolute_rotation(fixed_plate).first;
	const UnitQuaternion3D &uq = motion_of_fixed_plate.unit_quat();
	if ( ! represents_identity_rotation(uq)) {
		// Let's compensate for the motion of the "fixed" ref frame.
		UnitQuaternion3D::RotationParams params =
				uq.get_rotation_params(motion_of_fixed_plate.axis_hint());
		Rotation rot = Rotation::create(params.axis, params.angle);
		Rotation inverse_rot = rot.get_reverse();

		adjustment_rel_fixed = inverse_rot * adjustment_rel_fixed * rot;
	}
	d_adjustment_rel_fixed = adjustment_rel_fixed;
	d_dialog_ptr->set_adjustment(adjustment_rel_fixed);

	// Calculate the new result pole.
	FiniteRotation result_pole = compose(adjustment_rel_fixed, original_pole);
	d_dialog_ptr->set_result_pole(result_pole);
}


void
GPlatesQtWidgets::AdjustmentApplicator::handle_pole_sequence_choice_cleared()
{
	d_sequence_choice_index = boost::none;
}


void
GPlatesQtWidgets::AdjustmentApplicator::handle_pole_time_changed(
		double new_pole_time)
{
}


void
GPlatesQtWidgets::AdjustmentApplicator::apply_adjustment()
{
	if (d_sequence_choices.empty() || ( ! d_adjustment_rel_fixed) ||
			( ! d_sequence_choice_index)) {
		// Nothing we can do.
		// (Is this an erroneous situation, about which we should complain?)
		return;
	}

	GPlatesModel::FeatureHandle::weak_ref chosen_pole_seq =
			d_sequence_choices.at(*d_sequence_choice_index).d_trs;
	if ( ! chosen_pole_seq.is_valid()) {
		// Nothing we can do.
		// (Should we complain?)
		return;
	}

	// We want to merge model events across this scope so that only one model event
	// is generated instead of many as we incrementally modify the feature below.
	GPlatesModel::NotificationGuard model_notification_guard(
			d_application_state_ptr->get_model_interface().access_model());

	GPlatesFeatureVisitors::TotalReconstructionSequenceRotationInserter inserter(
			d_pole_time,
			*d_adjustment_rel_fixed,
			d_dialog_ptr->get_comment_line());
	inserter.visit_feature(chosen_pole_seq);

	// We release the model notification guard which will cause a reconstruction to occur
	// because we modified the model.
	// Note that we do this before emitting the 'have_reconstructed' signal.
	model_notification_guard.release_guard();

	Q_EMIT have_reconstructed();
}
