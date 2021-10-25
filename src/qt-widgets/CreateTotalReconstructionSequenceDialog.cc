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
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMessageBox>

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/TRSUtils.h"
#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "file-io/FeatureCollectionFileFormatClassify.h"
#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/ModelUtils.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "qt-widgets/ChooseFeatureCollectionWidget.h"
#include "qt-widgets/QtWidgetUtils.h"

#include "EditTotalReconstructionSequenceWidget.h"
#include "TotalReconstructionSequencesDialog.h"
#include "CreateTotalReconstructionSequenceDialog.h"

const QString new_feature_collection_string = QObject::tr("< Create a new feature collection >");

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

GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::CreateTotalReconstructionSequenceDialog(
	GPlatesQtWidgets::TotalReconstructionSequencesDialog &trs_dialog,
	GPlatesAppLogic::ApplicationState &app_state,
	QWidget *parent_):
		QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
		d_trs_dialog(trs_dialog),
		d_edit_widget_ptr(new GPlatesQtWidgets::EditTotalReconstructionSequenceWidget(this)),
		d_choose_feature_collection_widget_ptr(NULL),
		d_app_state(app_state)
{
	setupUi(this);
	
	// Set these to false to prevent buttons from stealing Enter events from the spinboxes
	// in the enclosed widget.
	button_create->setAutoDefault(false);
	button_cancel->setAutoDefault(false);
	button_create->setDefault(false);
	button_cancel->setDefault(false);

	make_connections();
	setup_pages();
}

void
GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::init()
{
    // Should we retain last used values, or reset everything each time
    // we show this dialog?

    make_trs_page_current();
    d_edit_widget_ptr->initialise();
}

GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::~CreateTotalReconstructionSequenceDialog()
{}

void
GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::handle_create()
{
	using namespace GPlatesModel;

	d_edit_widget_ptr->sort_table_by_time();

	if (!d_edit_widget_ptr->validate())
	{
		button_create->setEnabled(false);
		return;
	}

	// Get the selected feature collection, or create a new one.

	std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_reference, bool> collection_file_iter =
		d_choose_feature_collection_widget_ptr->get_file_reference();
	GPlatesModel::FeatureCollectionHandle::weak_ref collection =
		(collection_file_iter.first).get_file().get_feature_collection();

	// Create a new TRS feature.

	FeatureType feature_type = FeatureType::create_gpml("TotalReconstructionSequence");
	FeatureHandle::weak_ref trs_feature = GPlatesModel::FeatureHandle::create(collection, feature_type);

	// Get the irregular sampling property.
	TopLevelProperty::non_null_ptr_type trs =
		d_edit_widget_ptr->get_irregular_sampling_property_value_from_table_widget();

	// Create the plate-id properties.

	TopLevelProperty::non_null_ptr_type moving_prop = 
		TopLevelPropertyInline::create(
		PropertyName::create_gpml("movingReferenceFrame"),
		GPlatesPropertyValues::GpmlPlateId::create(d_edit_widget_ptr->moving_plate_id()));

	TopLevelProperty::non_null_ptr_type fixed_prop = 
		TopLevelPropertyInline::create(
		PropertyName::create_gpml("fixedReferenceFrame"),
		GPlatesPropertyValues::GpmlPlateId::create(d_edit_widget_ptr->fixed_plate_id()));


	trs_feature->add(fixed_prop);
	trs_feature->add(moving_prop);
	trs_feature->add(trs);

	d_trs_feature.reset(trs_feature);
	d_trs_dialog.insert_feature_to_proxy(*d_trs_feature, (collection_file_iter.first).get_file());
    accept();
}

void
GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::handle_cancel()
{
	reject();
}

void
GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::make_connections()
{
	QObject::connect(
		button_create,SIGNAL(clicked()),
		this,SLOT(handle_create()));
	QObject::connect(
		button_cancel,SIGNAL(clicked()),
		this,SLOT(handle_cancel()));
	QObject::connect(
		d_edit_widget_ptr.get(),SIGNAL(table_validity_changed(bool)),
		this,SLOT(handle_table_validity_changed(bool)));
	QObject::connect(
		button_previous,SIGNAL(clicked()),
		this,SLOT(handle_previous()));
	QObject::connect(
		button_next,SIGNAL(clicked()),
		this,SLOT(handle_next()));
}


void
GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::handle_table_validity_changed(
	bool valid)
{
	button_next->setEnabled(valid);
}

void
GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::handle_previous()
{
	// This should only be possible if we're on the feature-collection page.
	if (stacked_widget->currentIndex()==COLLECTION_PAGE)
	{
		make_trs_page_current();
	}
}

void
GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::handle_next()
{
	// This should only be possible if we're on the trs page.
	if (stacked_widget->currentIndex()==TRS_PAGE)
	{
		make_feature_collection_page_current();
	}
}

void
GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::setup_pages()
{
	stacked_widget->insertWidget(TRS_PAGE,d_edit_widget_ptr.get());

	GPlatesFileIO::FeatureCollectionFileFormat::classifications_type reconstruction_collections;

	reconstruction_collections.set(GPlatesFileIO::FeatureCollectionFileFormat::RECONSTRUCTION);

	if (!d_choose_feature_collection_widget_ptr)
	{
		d_choose_feature_collection_widget_ptr =
			new ChooseFeatureCollectionWidget(d_app_state.get_reconstruct_method_registry(),
			d_app_state.get_feature_collection_file_state(),
			d_app_state.get_feature_collection_file_io(),
			0,
			reconstruction_collections);
	}

	d_choose_feature_collection_widget_ptr->initialise();

	stacked_widget->insertWidget(COLLECTION_PAGE,d_choose_feature_collection_widget_ptr);
}

void
GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::make_trs_page_current()
{
	button_previous->setEnabled(false);
	button_next->setEnabled(true);
	button_create->setEnabled(false);
	stacked_widget->setCurrentIndex(TRS_PAGE);
}

void
GPlatesQtWidgets::CreateTotalReconstructionSequenceDialog::make_feature_collection_page_current()
{
	button_previous->setEnabled(true);
	button_next->setEnabled(false);
	button_create->setEnabled(true);
	stacked_widget->setCurrentIndex(COLLECTION_PAGE);
    d_choose_feature_collection_widget_ptr->initialise();
	d_choose_feature_collection_widget_ptr->setFocus();
}

