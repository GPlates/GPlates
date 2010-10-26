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
 
#include <QMenu>

#include "SpecifyAnchoredPlateIdDialog.h"

#include "app-logic/ReconstructionFeatureProperties.h"


GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::SpecifyAnchoredPlateIdDialog(
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint)
{
	setupUi(this);

	// Set up fill button's menu.
	QMenu *fill_menu = new QMenu(this);
	fill_menu->addAction(action_Reconstruction_Plate_Id);
	fill_menu->addAction(action_Left_Plate);
	fill_menu->addAction(action_Right_Plate);
	fill_toolbutton->setMenu(fill_menu);
	QObject::connect(
			fill_menu,
			SIGNAL(triggered(QAction *)),
			this,
			SLOT(handle_action_triggered(QAction *)));

	// Button box signals.
	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(accept()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));

	// Notify listeners about a change of plate ID when the user clicks OK.
	QObject::connect(
			this,
			SIGNAL(accepted()),
			this,
			SLOT(propagate_value()));
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::populate(
		GPlatesModel::integer_plate_id_type plate_id,
		const GPlatesModel::FeatureHandle::weak_ref &focused_feature)
{
	populate_spinbox(plate_id);
	populate_menu(focused_feature);
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::showEvent(
		QShowEvent *ev)
{
	fixed_plate_spinbox->setFocus();
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::propagate_value()
{
	emit value_changed(
			static_cast<GPlatesModel::integer_plate_id_type>(
				fixed_plate_spinbox->value()));
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::handle_action_triggered(
		QAction *action)
{
	QVariant qv = action->data();
	bool ok;
	uint plate_id = qv.toUInt(&ok);
	if (ok)
	{
		populate_spinbox(static_cast<GPlatesModel::integer_plate_id_type>(plate_id));
	}
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::populate_spinbox(
		GPlatesModel::integer_plate_id_type plate_id)
{
	// Update the spinbox's value. Note that the range of the spinbox is narrower
	// than the range of possible plate IDs.
	if (plate_id >= static_cast<GPlatesModel::integer_plate_id_type>(fixed_plate_spinbox->minimum()) &&
			plate_id <= static_cast<GPlatesModel::integer_plate_id_type>(fixed_plate_spinbox->maximum()))
	{
		fixed_plate_spinbox->setValue(static_cast<int>(plate_id));
	}
	else
	{
		fixed_plate_spinbox->setValue(fixed_plate_spinbox->minimum());
	}

	fixed_plate_spinbox->selectAll();
}


void
GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog::populate_menu(
		const GPlatesModel::FeatureHandle::weak_ref &focused_feature)
{
	// Find out what plate IDs are in the given feature.
	GPlatesAppLogic::ReconstructionFeatureProperties visitor;
	if (focused_feature.is_valid())
	{
		visitor.visit_feature(focused_feature);
	}

	typedef boost::optional<GPlatesModel::integer_plate_id_type> opt_plate_id_type;
	bool any_plate_id_found = false;

	const opt_plate_id_type &recon_plate_id = visitor.get_recon_plate_id();
	action_Reconstruction_Plate_Id->setVisible(recon_plate_id);
	if (recon_plate_id)
	{
		any_plate_id_found = true;
		action_Reconstruction_Plate_Id->setData(static_cast<uint>(*recon_plate_id));
	}

	const opt_plate_id_type &left_plate_id = visitor.get_left_plate_id();
	action_Left_Plate->setVisible(left_plate_id);
	if (left_plate_id)
	{
		any_plate_id_found = true;
		action_Left_Plate->setData(static_cast<uint>(*left_plate_id));
	}

	const opt_plate_id_type &right_plate_id = visitor.get_right_plate_id();
	action_Right_Plate->setVisible(right_plate_id);
	if (right_plate_id)
	{
		any_plate_id_found = true;
		action_Right_Plate->setData(static_cast<uint>(*right_plate_id));
	}

	fill_toolbutton->setEnabled(any_plate_id_found);
}

