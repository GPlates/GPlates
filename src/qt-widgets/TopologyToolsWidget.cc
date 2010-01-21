/**
 * \file 
 * $Revision: 5796 $
 * $Date: 2009-05-07 17:04:21 -0700 (Thu, 07 May 2009) $ 
 * 
 * Copyright (C) 2008, 2009 California Institute of Technology
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

#include <QLocale>
#include <QDebug>
#include <QMessageBox>
#include <QVBoxLayout>

#include "TopologyToolsWidget.h"
#include "CreateFeatureDialog.h"
#include "FeatureSummaryWidget.h"

#include "app-logic/ApplicationState.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "gui/FeatureFocus.h"
#include "gui/TopologyTools.h"

#include "model/FeatureHandle.h"

#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/XsString.h"

#include "utils/UnicodeStringUtils.h"

#include "presentation/ViewState.h"


namespace
{
	/**
	 * Borrowed from FeatureTableModel.cc.
	 */
	QString
	format_time_instant(
			const GPlatesPropertyValues::GmlTimeInstant &time_instant)
	{
		QLocale locale;
		if (time_instant.time_position().is_real()) {
			return locale.toString(time_instant.time_position().value());
		} else if (time_instant.time_position().is_distant_past()) {
			return QObject::tr("past");
		} else if (time_instant.time_position().is_distant_future()) {
			return QObject::tr("future");
		} else {
			return QObject::tr("<invalid>");
		}
	}


	/**
	 * We now have four of these plate ID fields.
	 */
	void
	fill_plate_id_field(
			QLineEdit *field,
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			const GPlatesModel::PropertyName &property_name)
	{
		const GPlatesPropertyValues::GpmlPlateId *plate_id = NULL;
		if (GPlatesFeatureVisitors::get_property_value(
				feature_ref, property_name, plate_id))
		{
			// The feature has a plate ID of the desired kind.
			
			field->setText(QString::number(plate_id->value()));
		}
	}
}


GPlatesQtWidgets::TopologyToolsWidget::TopologyToolsWidget(
		GPlatesPresentation::ViewState &view_state_,
		ViewportWindow &viewport_window_,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		QWidget *parent_):
	QWidget(parent_),
	d_feature_focus_ptr(&view_state_.get_feature_focus()),
	d_model_interface(&view_state_.get_application_state().get_model_interface()),
	d_create_feature_dialog(
		new CreateFeatureDialog(
			view_state_,
			viewport_window_, 
			GPlatesQtWidgets::CreateFeatureDialog::TOPOLOGICAL, this) 
	),
	d_topology_tools_ptr(
		new GPlatesGui::TopologyTools(
			view_state_, 
			viewport_window_,
			choose_canvas_tool) 
	),
	d_feature_summary_widget_ptr(
		new FeatureSummaryWidget(view_state_)
	)
{
	setupUi(this);

	setup_widgets();
	setup_connections();
}

void
GPlatesQtWidgets::TopologyToolsWidget::setup_widgets()
{
	// The .ui file has defined the majority of things we want to use,
	// but to mix in a FeatureSummaryWidget programmatically, we need
	// to put it into a blank 'placeholder' QWidget that has been set
	// up in the Designer.
	QVBoxLayout *layout_section = new QVBoxLayout(widget_feature_summary_placeholder);
	layout_section->setSpacing(2);
	layout_section->setContentsMargins(0, 0, 0, 0);

	// add the Feature Summary Widget 
	layout_section->addWidget( d_feature_summary_widget_ptr );

}

void
GPlatesQtWidgets::TopologyToolsWidget::setup_connections()
{
	// Attach buttons to functions
	 QObject::connect( button_remove_all_sections, SIGNAL(clicked()),
 		this, SLOT(handle_remove_all_sections()));

	 QObject::connect( button_create, SIGNAL(clicked()),
 		this, SLOT(handle_create()));

	 QObject::connect( button_add_feature, SIGNAL(clicked()),
 		this, SLOT(handle_add_feature()));
}


void
GPlatesQtWidgets::TopologyToolsWidget::activate( GPlatesGui::TopologyTools::CanvasToolMode mode)
{
	setDisabled( false );
	d_topology_tools_ptr->activate( mode );

	// connect to Feature Creation Dialog signals
	QObject::connect(
		d_create_feature_dialog,
 		SIGNAL( feature_created( GPlatesModel::FeatureHandle::weak_ref) ),
		d_topology_tools_ptr,
   		SLOT( handle_create_new_feature( GPlatesModel::FeatureHandle::weak_ref ) ) );
}


void
GPlatesQtWidgets::TopologyToolsWidget::deactivate()
{
	setDisabled( true );
	d_topology_tools_ptr->deactivate();
	clear();
}


void
GPlatesQtWidgets::TopologyToolsWidget::clear()
{
	lineedit_name->clear();
	lineedit_plate_id->clear();
	lineedit_time_of_appearance->clear();
	lineedit_time_of_disappearance->clear();
	lineedit_number_of_sections->clear();

	d_feature_summary_widget_ptr->clear();
}

void
GPlatesQtWidgets::TopologyToolsWidget::set_click_point(  double lat, double lon )
{
	d_topology_tools_ptr->set_click_point(lat, lon);
}
	

void
GPlatesQtWidgets::TopologyToolsWidget::display_topology(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type /*associated_rg*/)
{
	// Always check your weak_refs!
	if ( ! feature_ref.is_valid()) {
		setDisabled(true);
		return;
	} else {
		setDisabled(false);
	}
	
	// Clear the fields first, then fill in those that we have data for.
	clear();

	// Populate the widget from the FeatureHandle:
	
	// Feature Name.
	// FIXME: Need to adapt according to user's current codeSpace setting.
	static const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");

	const GPlatesPropertyValues::XsString *name = NULL;
	if (GPlatesFeatureVisitors::get_property_value(feature_ref, name_property_name, name))
	{
		// The feature has one or more name properties. Use the first one for now.
		lineedit_name->setText(GPlatesUtils::make_qstring(name->value()));
		lineedit_name->setCursorPosition(0);
	}

	// Plate ID.
	static const GPlatesModel::PropertyName recon_plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	fill_plate_id_field(lineedit_plate_id, feature_ref, recon_plate_id_property_name);

	// Valid Time (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	const GPlatesPropertyValues::GmlTimePeriod *time_period = NULL;
	if (GPlatesFeatureVisitors::get_property_value(
			feature_ref, valid_time_property_name, time_period))
	{
		// The feature has a gml:validTime property.
		lineedit_time_of_appearance->setText(format_time_instant(*(time_period->begin())));
		lineedit_time_of_disappearance->setText(format_time_instant(*(time_period->end())));
	}
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_shift_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	// call the tools fuction
	d_topology_tools_ptr->handle_shift_left_click( 
		click_pos_on_globe, oriented_click_pos_on_globe, is_on_globe);
}
	


void
GPlatesQtWidgets::TopologyToolsWidget::handle_remove_all_sections()
{
	// call the tools fuction
	d_topology_tools_ptr->handle_remove_all_sections();
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_create()
{
	if ( d_topology_tools_ptr->get_number_of_sections() < 1 )
	{
		// post warning 
		QMessageBox::warning(this,
			tr("No boundary sections are defined for this topology feature."),
			tr("No boundary sections are defined for this topology feature.\n"
		"Click on Features on the Globe, then use the Topology Tools to Add boundary sections."),
			QMessageBox::Ok);
		return;
	}

	// check for existing topology
	if ( ! d_topology_tools_ptr->get_topology_feature_ref().is_valid() )
	{
		bool success = d_create_feature_dialog->display();

		if ( ! success ) 
		{
			// The user cancelled the creation process. 
 			// Return early and do not reset the widget.
			return;
		}

		// else, the feature was created by the dialog
		// and d_topology_tools_ptr method handle_create_new_feature() was called
		// to set the new topology feature ref
	}

	// apply the latest boundary to the new topology
	d_topology_tools_ptr->handle_apply(); 
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_add_feature()
{
	// simple short cut for no op
	if ( ! d_feature_focus_ptr->is_valid() )
	{
		return;
	}

	// call the tools fuction
	d_topology_tools_ptr->handle_add_feature();

	// Flip tab to topoology
	tabwidget_main->setCurrentWidget( tab_topology );
}

