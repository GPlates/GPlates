/* $Id: EditTimeSequenceWidget.cc 8310 2010-05-06 15:02:23Z rwatson $ */

/**
 * \file 
 * $Revision: 8310 $
 * $Date: 2010-05-06 17:02:23 +0200 (to, 06 mai 2010) $ 
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>

#include "app-logic/TRSUtils.h"
#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/ModelUtils.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "qt-widgets/QtWidgetUtils.h"

#include "EditTotalReconstructionSequenceWidget.h"
#include "TotalReconstructionSequencesDialog.h"
#include "EditTotalReconstructionSequenceDialog.h"


namespace{
    /**
	 * Borrowed from the TopologySectionsTable. Will try using it for itemChanged here. 
	 *
	 * Tiny convenience class to help suppress the @a QTableWidget::cellChanged()
	 * notification in situations where we are updating the table data
	 * programatically. This allows @a react_cell_changed() to differentiate
	 * between changes made by us, and changes made by the user.
	 *
	 * For it to work properly, you must declare one in any TopologySectionsTable
	 * method that directly mucks with table cell data.
	 */
	struct TableUpdateGuard :
			public boost::noncopyable
	{
		TableUpdateGuard(
				bool &guard_flag_ref):
			d_guard_flag_ptr(&guard_flag_ref)
		{
			// Nesting these guards is an error.
			Q_ASSERT(*d_guard_flag_ptr == false);
			*d_guard_flag_ptr = true;
		}
		
		~TableUpdateGuard()
		{
			*d_guard_flag_ptr = false;
		}
		
		bool *d_guard_flag_ptr;
	};
}

GPlatesQtWidgets::EditTotalReconstructionSequenceDialog::EditTotalReconstructionSequenceDialog(
	GPlatesModel::FeatureHandle::weak_ref &trs_feature,
	GPlatesQtWidgets::TotalReconstructionSequencesDialog &trs_dialog,
	QWidget *parent_):
		QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
		d_trs_feature(trs_feature),
		d_trs_dialog(trs_dialog),
		d_edit_widget_ptr(new GPlatesQtWidgets::EditTotalReconstructionSequenceWidget(this))
{
	setupUi(this);
	
	// Set these to false to prevent buttons from stealing Enter events from the spinboxes
	// in the enclosed widget.
	buttonbox->button(QDialogButtonBox::Apply)->setAutoDefault(false);
	buttonbox->button(QDialogButtonBox::Cancel)->setAutoDefault(false);
	buttonbox->button(QDialogButtonBox::Apply)->setDefault(false);
	buttonbox->button(QDialogButtonBox::Cancel)->setDefault(false);

	buttonbox->button(QDialogButtonBox::Apply)->setText(tr("&Apply"));
	buttonbox->button(QDialogButtonBox::Cancel)->setText(tr("&Cancel"));

	make_connections();

    QtWidgetUtils::add_widget_to_placeholder(d_edit_widget_ptr.get(),widget_placeholder);

	GPlatesAppLogic::TRSUtils::TRSFinder trs_finder;
	trs_finder.visit_feature(d_trs_feature);

	if (!trs_finder.can_process_trs())
	{
		return;
	}

	d_irregular_sampling_property_iterator = trs_finder.irregular_sampling_property_iterator();	
	d_irregular_sampling = trs_finder.irregular_sampling();
	d_moving_plate_id = trs_finder.moving_ref_frame_plate_id();
	d_fixed_plate_id = trs_finder.fixed_ref_frame_plate_id();
	d_moving_ref_frame_iterator = trs_finder.moving_ref_frame_property_iterator();
	d_fixed_ref_frame_iterator = trs_finder.fixed_ref_frame_property_iterator();

	d_edit_widget_ptr->update_table_widget_from_property(*d_irregular_sampling);
	d_edit_widget_ptr->set_moving_plate_id(*d_moving_plate_id);
	d_edit_widget_ptr->set_fixed_plate_id(*d_fixed_plate_id);
	d_edit_widget_ptr->set_action_widget_in_row(0);

	buttonbox->button(QDialogButtonBox::Apply)->setEnabled(false);

}

GPlatesQtWidgets::EditTotalReconstructionSequenceDialog::~EditTotalReconstructionSequenceDialog()
{  }

void
GPlatesQtWidgets::EditTotalReconstructionSequenceDialog::handle_apply()
{
	//FIXME: should we close the edit dialog on apply?


	d_edit_widget_ptr->sort_table_by_time();

	if (!d_edit_widget_ptr->validate())
	{
		return;
	}

	// Update the irregular sampling property.
	GPlatesModel::TopLevelProperty::non_null_ptr_type trs =
		d_edit_widget_ptr->get_irregular_sampling_property_value_from_table_widget();

	// Replace the irregular sampling property of the TRS feature with the TRS we've just created. 
	if (d_irregular_sampling_property_iterator)
	{
		**d_irregular_sampling_property_iterator = trs;
	}

	// Update the plate-id properties.

	GPlatesModel::TopLevelProperty::non_null_ptr_type moving_prop = 
		GPlatesModel::TopLevelPropertyInline::create(
		GPlatesModel::PropertyName::create_gpml("movingReferenceFrame"),
		GPlatesPropertyValues::GpmlPlateId::create(d_edit_widget_ptr->moving_plate_id()));

	**d_moving_ref_frame_iterator = moving_prop;

	GPlatesModel::TopLevelProperty::non_null_ptr_type fixed_prop = 
		GPlatesModel::TopLevelPropertyInline::create(
		GPlatesModel::PropertyName::create_gpml("fixedReferenceFrame"),
		GPlatesPropertyValues::GpmlPlateId::create(d_edit_widget_ptr->fixed_plate_id()));

	**d_fixed_ref_frame_iterator = fixed_prop;

	d_trs_dialog.update_edited_feature();

	buttonbox->button(QDialogButtonBox::Apply)->setEnabled(false);

}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceDialog::handle_cancel()
{
	reject();
}


void
GPlatesQtWidgets::EditTotalReconstructionSequenceDialog::make_connections()
{
	QObject::connect(
		buttonbox->button(QDialogButtonBox::Apply),SIGNAL(clicked()),
		this,SLOT(handle_apply()));
	QObject::connect(
		buttonbox->button(QDialogButtonBox::Cancel),SIGNAL(clicked()),
		this,SLOT(handle_cancel()));
	QObject::connect(
		d_edit_widget_ptr.get(),SIGNAL(table_validity_changed(bool)),
		this,SLOT(handle_table_validity_changed(bool)));
	QObject::connect(
		d_edit_widget_ptr.get(),SIGNAL(plate_ids_have_changed()),
		this,SLOT(handle_plate_ids_changed()));
}


void
GPlatesQtWidgets::EditTotalReconstructionSequenceDialog::handle_table_validity_changed(
	bool valid)
{
	buttonbox->button(QDialogButtonBox::Apply)->setEnabled(valid);
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceDialog::handle_plate_ids_changed()
{
	buttonbox->button(QDialogButtonBox::Apply)->setEnabled(true);
}
