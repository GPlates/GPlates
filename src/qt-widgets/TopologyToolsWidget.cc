/**
 * \file 
 * $Revision: 5796 $
 * $Date: 2009-05-07 17:04:21 -0700 (Thu, 07 May 2009) $ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "TopologyToolsWidget.h"
#include "CreateFeatureDialog.h"
#include "FeatureSummaryWidget.h"

#include "gui/FeatureFocus.h"
#include "gui/TopologyTools.h"

#include "model/FeatureHandle.h"

#include "utils/UnicodeStringUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/XsString.h"


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
				*feature_ref, property_name, plate_id))
		{
			// The feature has a plate ID of the desired kind.
			
			field->setText(QString::number(plate_id->value()));
		}
	}
}


GPlatesQtWidgets::TopologyToolsWidget::TopologyToolsWidget(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesModel::ModelInterface &model_interface,
		ViewportWindow &view_state_,
		QWidget *parent_):
	QWidget(parent_),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_feature_focus_ptr(&feature_focus),
	d_model_interface(&model_interface),
	d_view_state_ptr(&view_state_),
	d_create_feature_dialog(
		new CreateFeatureDialog(
			model_interface, 
			view_state_, 
			GPlatesQtWidgets::CreateFeatureDialog::TOPOLOGICAL, this) 
	),
	d_topology_tools_ptr(
		new GPlatesGui::TopologyTools(
			rendered_geom_collection, 
			feature_focus, 
			view_state_) 
	),
	d_feature_summary_widget_ptr(
		new FeatureSummaryWidget(feature_focus, tab_section)
	)
{
	setupUi(this);


	// Attach buttons to functions
	 QObject::connect( button_remove_all_sections, SIGNAL(clicked()),
 		this, SLOT(handle_remove_all_sections()));

	 QObject::connect( button_create, SIGNAL(clicked()),
 		this, SLOT(handle_create()));

	 QObject::connect( button_add_feature, SIGNAL(clicked()),
 		this, SLOT(handle_add_feature()));

	// add a little space 
	layout_section->addItem( 
		new QSpacerItem(2, 2, QSizePolicy::Minimum, QSizePolicy::Expanding));

	// add the Feature Summary Widget 
	layout_section->addWidget( d_feature_summary_widget_ptr );

	// more space at the bottom to size things up a bit
	layout_section->addItem( 
		new QSpacerItem(3, 3, QSizePolicy::Minimum, QSizePolicy::Expanding));
}

void
GPlatesQtWidgets::TopologyToolsWidget::activate( GPlatesGui::TopologyTools::CanvasToolMode mode)
{
	setDisabled( false );

	clear();

#if 0
	if ( d_feature_focus_ptr->is_valid() ) 
	{
		qDebug() << "TopologyToolsWidget::activate():: d_feature_focus_ptr->is_valid() TRUE ";
	}
	else
	{
		qDebug() << "TopologyToolsWidget::activate():: d_feature_focus_ptr->is_valid() FALSE";
	}
#endif
	
	// activate the TopologyTools object
	d_topology_tools_ptr->activate( mode );
}


void
GPlatesQtWidgets::TopologyToolsWidget::deactivate()
{
	setDisabled( true );
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
GPlatesQtWidgets::TopologyToolsWidget::display_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg)
{
	// Clear the fields first, then fill in those that we have data for.
	clear();

	// Always check your weak_refs!
	if ( ! feature_ref.is_valid()) {
		setDisabled(true);
		return;
	} else {
		setDisabled(false);
	}
	
	// Populate the widget from the FeatureHandle:
	
	// Feature Name.
	// FIXME: Need to adapt according to user's current codeSpace setting.
	static const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");

	const GPlatesPropertyValues::XsString *name = NULL;
	if (GPlatesFeatureVisitors::get_property_value(*feature_ref, name_property_name, name))
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
			*feature_ref, valid_time_property_name, time_period))
	{
		// The feature has a gml:validTime property.
		lineedit_time_of_appearance->setText(format_time_instant(*(time_period->begin())));
		lineedit_time_of_disappearance->setText(format_time_instant(*(time_period->end())));
	}

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
	// call the tools fuction
	d_topology_tools_ptr->handle_apply();
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_add_feature()
{
	// call the tools fuction
	d_topology_tools_ptr->handle_add_feature();

	// Flip tab to topoology
	tabwidget_main->setCurrentWidget( tab_topology );
}

