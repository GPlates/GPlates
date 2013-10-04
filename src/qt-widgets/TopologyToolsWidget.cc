/**
 * \file 
 * $Revision: 5796 $
 * $Date: 2009-05-07 17:04:21 -0700 (Thu, 07 May 2009) $ 
 * 
 * Copyright (C) 2008, 2009 California Institute of Technology
 * Copyright (C) 2011 The University of Sydney, Australia
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
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <Qt>
#include <QAction>
#include <QDebug>
#include <QKeySequence>
#include <QLocale>
#include <QMessageBox>
#include <QVBoxLayout>

#include "TopologyToolsWidget.h"

#include "ActionButtonBox.h"
#include "CreateFeatureDialog.h"
#include "FeatureSummaryWidget.h"
#include "QtWidgetUtils.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/TopologyInternalUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/CanvasToolWorkflows.h"
#include "gui/FeatureFocus.h"
#include "gui/TopologySectionsContainer.h"
#include "gui/TopologyTools.h"

#include "model/FeatureHandle.h"
#include "model/Gpgim.h"
#include "model/GpgimProperty.h"
#include "model/ModelUtils.h"
#include "model/PropertyName.h"

#include "presentation/ViewState.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsString.h"

#include "utils/UnicodeStringUtils.h"

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
		if (time_instant.get_time_position().is_real()) {
			return locale.toString(time_instant.get_time_position().value());
		} else if (time_instant.get_time_position().is_distant_past()) {
			return QObject::tr("past");
		} else if (time_instant.get_time_position().is_distant_future()) {
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
		boost::optional<GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type> plate_id =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GpmlPlateId>(
						feature_ref, property_name);
		if (plate_id)
		{
			// The feature has a plate ID of the desired kind.
			
			field->setText(QString::number(plate_id.get()->get_value()));
		}
	}

	/**
	 * Retrieves the topological geometry property name from the specified feature.
	 */
	boost::optional<GPlatesModel::PropertyName>
	get_topological_geometry_property_name_from_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
	{
		boost::optional<GPlatesModel::PropertyName> topology_geometry_property_name;

		GPlatesModel::FeatureHandle::iterator iter = feature_ref->begin();
		GPlatesModel::FeatureHandle::iterator end = feature_ref->end();
		// loop over properties
		for ( ; iter != end; ++iter) 
		{
			// Visit the current property to determine if it's a topological geometry.
			boost::optional<GPlatesPropertyValues::StructuralType> topology_geometry_property_value_type =
					GPlatesAppLogic::TopologyInternalUtils::get_topology_geometry_property_value_type(iter);
			if (topology_geometry_property_value_type)
			{
				// Note that the property name is not fixed and there can be a few alternatives
				// (like 'boundary', 'centerLineOf', etc) so we return the property name to the caller.
				//
				// There should only be one of these properties per feature so we'll just use the
				// first one encountered if this is not true.
				if (!topology_geometry_property_name)
				{
					topology_geometry_property_name = (*iter)->get_property_name();
				}
				else
				{
					qWarning() << "Encountered multiple topological property values in one feature - "
						<< "using name of the first property encountered.";
				}
			}
		} // loop over properties

		return topology_geometry_property_name;
	}
}


GPlatesQtWidgets::TopologyToolsWidget::TopologyToolsWidget(
		GPlatesPresentation::ViewState &view_state_,
		ViewportWindow &viewport_window_,
		QAction *clear_action,
		GPlatesGui::CanvasToolWorkflows &canvas_tool_workflows,
		QWidget *parent_):
	TaskPanelWidget(parent_),
	d_view_state(view_state_),
	d_viewport_window(viewport_window_),
	d_gpgim(view_state_.get_application_state().get_gpgim()),
	d_feature_focus_ptr(&view_state_.get_feature_focus()),
	d_model_interface(&view_state_.get_application_state().get_model_interface()),
	d_canvas_tool_workflows(&canvas_tool_workflows),
	d_create_feature_dialog(
		new CreateFeatureDialog(view_state_, viewport_window_, this)),
	d_topology_tools_ptr(
		new GPlatesGui::TopologyTools(view_state_, viewport_window_)),
	d_feature_summary_widget_ptr(
		new FeatureSummaryWidget(view_state_))
{
	setupUi(this);

	// Set up the action button box to hold the "Clear" button.
	ActionButtonBox *action_button_box = new ActionButtonBox(1, 16, this);
	action_button_box->add_action(clear_action);
#ifndef Q_WS_MAC
	action_button_box->setFixedHeight(button_create->sizeHint().height());
	action_button_box->setFixedHeight(button_apply->sizeHint().height());
#endif
	QtWidgetUtils::add_widget_to_placeholder(
			action_button_box,
			action_button_box_placeholder_widget);

	setup_widgets();
	setup_connections();

	// Disable the task panel widget.
	// It will get enabled when one of the topology canvas tools is activated.
	// This prevents the user from interacting with the task panel widget if the
	// canvas tool happens to be disabled at startup.
	setEnabled(false);
}

GPlatesQtWidgets::TopologyToolsWidget::~TopologyToolsWidget()
{
	// This pointer isn't owned by anyone else
	delete d_topology_tools_ptr;
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

	// Create a QAction for the shortcut used to add a topological section (the "Add to Boundary" button).
	QAction *add_to_boundary_shortcut_action = new QAction(this);
	add_to_boundary_shortcut_action->setShortcut(QKeySequence(Qt::Key_A));
	// Set the shortcut to be active when any top-level window is active. This makes it easier to
	// add topological sections if, for example, the layers dialog is currently in focus.
	add_to_boundary_shortcut_action->setShortcutContext(Qt::ApplicationShortcut);
	// Add the QAction to the topology tools tab widget so it becomes active when the topology
	// tools tab widget is visible.
	tabwidget_main->addAction(add_to_boundary_shortcut_action);
	// Call handler when action is triggered.
	QObject::connect(
			add_to_boundary_shortcut_action, SIGNAL(triggered()),
			this, SLOT(handle_add_to_boundary_shortcut_triggered()));

	// Create a QAction for the shortcut used to add a topological section (the "Remove" button).
	QAction *remove_shortcut_action = new QAction(this);
	remove_shortcut_action->setShortcut(QKeySequence(Qt::Key_R));
	// Set the shortcut to be active when any top-level window is active. This makes it easier to
	// add topological sections if, for example, the layers dialog is currently in focus.
	remove_shortcut_action->setShortcutContext(Qt::ApplicationShortcut);
	// Add the QAction to the topology tools tab widget so it becomes active when the topology
	// tools tab widget is visible.
	tabwidget_main->addAction(remove_shortcut_action);
	// Call handler when action is triggered.
	QObject::connect(
			remove_shortcut_action, SIGNAL(triggered()),
			this, SLOT(handle_remove_shortcut_triggered()));

	// We don't currently have a shortcut for the "Add to Interior" button.
	// These are application shortcuts and hence the keyboard shortcut must not be used elsewhere
	// in GPlates such as in the canvas toolbar (otherwise get ambiguous shortcut error).
	// So the "I" key shortcut is already used by a canvas tool - we could choose a different key though.
	// For now we'll just leave it out.
}

void
GPlatesQtWidgets::TopologyToolsWidget::setup_connections()
{
	// Attach widgets to functions
	QObject::connect(
		sections_table_combobox, 
		SIGNAL(currentIndexChanged(int)),
 		this, 
		SLOT(handle_sections_combobox_index_changed(int)));
	
	QObject::connect( button_create, SIGNAL(clicked()),
 		this, SLOT(handle_create()));

	QObject::connect( button_apply, SIGNAL(clicked()),
 		this, SLOT(handle_apply()));

	QObject::connect( button_add_section, SIGNAL(clicked()),
 		this, SLOT(handle_add_to_boundary()));

	QObject::connect( button_add_interior, SIGNAL(clicked()),
 		this, SLOT(handle_add_to_interior()));

    QObject::connect( button_remove_section, SIGNAL(clicked()),
        this, SLOT(handle_remove()));

	// Connect to the topological sections containers so we see if it's possible to clear them or not.
	QObject::connect(
		&d_view_state.get_topology_boundary_sections_container(),
		SIGNAL(container_changed(GPlatesGui::TopologySectionsContainer &)),
		this, SLOT(handle_clear_action_changed()));
	QObject::connect(
		&d_view_state.get_topology_interior_sections_container(),
		SIGNAL(container_changed(GPlatesGui::TopologySectionsContainer &)),
		this, SLOT(handle_clear_action_changed()));
}


void
GPlatesQtWidgets::TopologyToolsWidget::activate(
		CanvasToolMode mode,
		GPlatesAppLogic::TopologyGeometry::Type topology_geometry_type)
{
	setEnabled(true);

	//
	// Enable/disable various topology widget components depending on the topology geometry *type*.
	//

	label_sections->setText(tr(
			(topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE)
			? "Sections:"
			: "Boundary Sections:"));

	button_add_section->setText(tr(
			(topology_geometry_type == GPlatesAppLogic::TopologyGeometry::NETWORK)
			? "Add To Boundary"
			: "Add"));

	if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::NETWORK)
	{
		widget_num_sections_interior->show();
		widget_sections_table_select->show();
		button_add_interior->show();
	}
	else // topological boundaries and lines don't have interiors...
	{
		widget_num_sections_interior->hide();
		widget_sections_table_select->hide();
		button_add_interior->hide();
	}

	//
	// Activate based on the canvas tool mode (BUILD or EDIT).
	//

	if (mode == BUILD)
	{
		// Enable and show the "Create" button.
		button_create->setEnabled(true);
		button_create->show();
		// Disable and hide the "Apply" button.
		button_apply->setDisabled(true);
		button_apply->hide();

		// There's no topology feature (yet) when building a new topology.
		d_edit_topology_feature_ref = boost::none;

		// Activate the topology tool for *building*.
		d_topology_tools_ptr->activate_build_mode(topology_geometry_type);
	}

	if (mode == EDIT)
	{
		// Enable and show the "Apply" button.
		button_apply->setEnabled(true);
		button_apply->show();
		// Disable and hide the "Create" button.
		button_create->setDisabled(true);
		button_create->hide();

		// The topology feature to be edited is the focused feature.
		// If it's not valid then disable the topology tools widget and return early.
		if (!d_feature_focus_ptr->focused_feature().is_valid())
		{
			setDisabled(true);
			return;
		}

		// Get the edit topology feature ref.
		d_edit_topology_feature_ref = d_feature_focus_ptr->focused_feature();

		//
		// Determine the time period of the edit topology and activate it in the topology tool.
		//
		// NOTE: Activating the topology tool also unsets the focused feature.
		//

		// Valid Time (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
		static const GPlatesModel::PropertyName valid_time_property_name =
				GPlatesModel::PropertyName::create_gml("validTime");

		boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> edit_topology_time_period =
				GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
						d_edit_topology_feature_ref.get(), valid_time_property_name);
		if (edit_topology_time_period)
		{
			// Activate the topology tool for *editing*.
			d_topology_tools_ptr->activate_edit_mode(
					topology_geometry_type,
					edit_topology_time_period.get());
		}
		else // Edit topology feature has no time period property...
		{
			// Assume edit topology feature exists for all time.
			// Activate the topology tool for *editing*.
			d_topology_tools_ptr->activate_edit_mode(topology_geometry_type);
		}

		//
		// Load the topology feature information into the Topology Widget in the task panel.
		//

		display_topology(d_edit_topology_feature_ref.get());
	}
}


void
GPlatesQtWidgets::TopologyToolsWidget::deactivate()
{
	setDisabled(true);

	d_topology_tools_ptr->deactivate();

	clear_task_panel();
}


void
GPlatesQtWidgets::TopologyToolsWidget::clear_task_panel()
{
	lineedit_name->clear();
	lineedit_plate_id->clear();
	lineedit_time_of_appearance->clear();
	lineedit_time_of_disappearance->clear();

	label_num_sections->setText( QString::number( 0 ) );
	label_num_sections_interior->setText( QString::number( 0 ) );

	d_feature_summary_widget_ptr->clear();
}
	

void
GPlatesQtWidgets::TopologyToolsWidget::display_topology(
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	// Always check your weak_refs!
	if ( ! feature_ref.is_valid()) {
		setDisabled(true);
		return;
	} else {
		setDisabled(false);
	}
	
	// Clear the fields first, then fill in those that we have data for.
	clear_task_panel();

	// Populate the widget from the FeatureHandle:
	
	// Feature Name.
	// FIXME: Need to adapt according to user's current codeSpace setting.
	static const GPlatesModel::PropertyName name_property_name = 
		GPlatesModel::PropertyName::create_gml("name");

	boost::optional<GPlatesPropertyValues::XsString::non_null_ptr_to_const_type> name =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::XsString>(
					feature_ref, name_property_name);
	if (name)
	{
		// The feature has one or more name properties. Use the first one for now.
		lineedit_name->setText(GPlatesUtils::make_qstring(name.get()->get_value()));
		lineedit_name->setCursorPosition(0);
	}

	// Plate ID.
	static const GPlatesModel::PropertyName recon_plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	fill_plate_id_field(lineedit_plate_id, feature_ref, recon_plate_id_property_name);

	// Valid Time (Assuming a gml:TimePeriod, rather than a gml:TimeInstant!)
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	boost::optional<GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type> time_period =
			GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::GmlTimePeriod>(
					feature_ref, valid_time_property_name);
	if (time_period)
	{
		// The feature has a gml:validTime property.
		lineedit_time_of_appearance->setText(format_time_instant(*(time_period.get()->begin())));
		lineedit_time_of_disappearance->setText(format_time_instant(*(time_period.get()->end())));
	}
}

//
void
GPlatesQtWidgets::TopologyToolsWidget::handle_sections_combobox_index_changed(
		int index)
{
	// call the tools functions
	d_topology_tools_ptr->handle_sections_combobox_index_changed( index );
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_clear()
{
	// call the tools fuction
	d_topology_tools_ptr->handle_clear();
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_clear_action_changed()
{
	emit_clear_action_enabled_changed(clear_action_enabled());
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_create()
{
	//
	// We get here if the user is in the *build* topology tool and has requested the creation
	// of a new topological feature.
	//

	// Get the edited topological geometry property(s).
	const boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> topological_geometry_property_value =
			d_topology_tools_ptr->create_topological_geometry_property();

	// All topologies require enough topological sections to form their topology.
	if (!topological_geometry_property_value)
	{
		// post warning 
		QMessageBox::warning(this,
			tr("Insufficient topological sections."),
			tr("Insufficient topological sections have been defined for this topology feature.\n"
				"Click on Features on the Globe, then use the Topology Tools to Add topological sections."),
			QMessageBox::Ok);

		// Return early so we don't switch canvas tools.
		return;
	}

	// Pop up the create feature dialog.
	if (!d_create_feature_dialog->set_geometry_and_display(topological_geometry_property_value.get()))
	{
		// The user canceled the creation process. 
		// Return early so we don't switch canvas tools.
		return;
	}

	// Now that we're finished building the topology, switch to the
	// tool used to choose a feature - this will allow the user to select
	// another topology for building/editing or do something else altogether.
	d_canvas_tool_workflows->choose_canvas_tool(
			GPlatesGui::CanvasToolWorkflows::WORKFLOW_TOPOLOGY,
			GPlatesGui::CanvasToolWorkflows::TOOL_CLICK_GEOMETRY);
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_apply()
{
	//
	// We get here if the user is in the *edit* topology tool and has requested that an existing
	// topological feature have its geometry property(s) modified.
	//

	// Get the edited topological geometry property.
	const boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> topological_geometry_property_value_opt =
			d_topology_tools_ptr->create_topological_geometry_property();

	// All topologies require enough topological sections to form their topology.
	if (!topological_geometry_property_value_opt)
	{
		// post warning 
		QMessageBox::warning(this,
			tr("Insufficient topological sections."),
			tr("Insufficient topological sections have been defined for this topology feature.\n"
				"Click on Features on the Globe, then use the Topology Tools to Add topological sections."),
			QMessageBox::Ok);

		// Return early without switching canvas tools.
		return;
	}
	GPlatesModel::PropertyValue::non_null_ptr_type topological_geometry_property_value =
			topological_geometry_property_value_opt.get();

	//
	// NOTE: We don't use the create feature dialog when *editing* a topology (only when building a new one).
	//

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_edit_topology_feature_ref && d_edit_topology_feature_ref->is_valid(),
			GPLATES_ASSERTION_SOURCE);

	//
	// First remove the topology geometry properties from the topology feature if any.
	// After this we'll add the edited topology geometry properties.
	//

	// Returns the property name of the topological property (eg, 'boundary', 'centerLineOf', etc).
	const boost::optional<GPlatesModel::PropertyName> topological_geometry_property_name =
			get_topological_geometry_property_name_from_feature(d_edit_topology_feature_ref.get());

	// We should have a topological geometry property otherwise what has the user been editing.
	if (!topological_geometry_property_name)
	{
		QMessageBox::warning(this,
				tr("Failed to find existing topological geometry."),
				tr("Edited topology feature has no topological geometry property.\n"
					"Topological edit discarded."),
				QMessageBox::Ok);
		// Return early without switching canvas tools.
		return;
	}

	// Create the edited geometry top-level property.
	// Query GPGIM to make sure correct type of time-dependent wrapper (if any) is used.
	GPlatesModel::ModelUtils::TopLevelPropertyError::Type add_property_error_code;
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> top_level_property =
			GPlatesModel::ModelUtils::create_top_level_property(
					topological_geometry_property_name.get(),
					topological_geometry_property_value,
					d_gpgim,
					d_edit_topology_feature_ref.get()->feature_type(),
					&add_property_error_code);
	if (!top_level_property)
	{
		// Not successful in adding edited topological geometry; show error message.
		static const char *const ERROR_MESSAGE_APPEND = QT_TR_NOOP("Topological edit discarded.");
		QMessageBox::warning(this,
				tr("Failed to create top-level topological geometry property."),
				tr(GPlatesModel::ModelUtils::get_error_message(add_property_error_code)) +
						" " + tr(ERROR_MESSAGE_APPEND),
				QMessageBox::Ok);
		return;
	}

	// Remove the previous topological geometry property.
	d_edit_topology_feature_ref.get()->remove_properties_by_name(
			topological_geometry_property_name.get());

	// Add the newly edited topological geometry property.
	d_edit_topology_feature_ref.get()->add(top_level_property.get());

	// Now that we're finished editing the topology, switch to the
	// tool used to choose a feature - this will allow the user to select
	// another topology for building/editing or do something else altogether.
	d_canvas_tool_workflows->choose_canvas_tool(
			GPlatesGui::CanvasToolWorkflows::WORKFLOW_TOPOLOGY,
			GPlatesGui::CanvasToolWorkflows::TOOL_CLICK_GEOMETRY);
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_add_to_boundary()
{
	// simple short cut for no op
	if ( ! d_feature_focus_ptr->is_valid() )
	{
		return;
	}

	// call the tools fuction
	d_topology_tools_ptr->handle_add_section();

	// Flip tab to topoology
	tabwidget_main->setCurrentWidget( tab_topology );
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_add_to_boundary_shortcut_triggered()
{
	handle_add_to_boundary();
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_add_to_interior()
{
	// simple short cut for no op
	if ( ! d_feature_focus_ptr->is_valid() )
	{
		return;
	}

	// call the tools fuction
	d_topology_tools_ptr->handle_add_interior();

	// Flip tab to topoology
	tabwidget_main->setCurrentWidget( tab_topology );
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_remove()
{
	// simple short cut for no op
	if ( ! d_feature_focus_ptr->is_valid() )
	{
		return;
	}

	// call the tools fuction
	d_topology_tools_ptr->handle_remove_section();

	// Flip tab to topoology
	tabwidget_main->setCurrentWidget( tab_topology );
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_remove_shortcut_triggered()
{
	handle_remove();
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_activation()
{
}

QString
GPlatesQtWidgets::TopologyToolsWidget::get_clear_action_text() const
{
	return tr("Clear");
}

bool
GPlatesQtWidgets::TopologyToolsWidget::clear_action_enabled() const
{
	return d_topology_tools_ptr->has_topological_sections();
}

void
GPlatesQtWidgets::TopologyToolsWidget::handle_clear_action_triggered()
{
	handle_clear();
}
