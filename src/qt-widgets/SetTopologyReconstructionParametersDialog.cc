/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <boost/optional.hpp> 
#include <boost/shared_ptr.hpp>

#include "SetTopologyReconstructionParametersDialog.h"

#include "QtWidgetUtils.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructLayerParams.h"
#include "app-logic/ReconstructParams.h"

#include "presentation/ReconstructVisualLayerParams.h"
#include "presentation/VisualLayer.h"


namespace
{
	const QString HELP_START_RECONSTRUCTION_AT_TIME_OF_DISAPPEARANCE_DIALOG_TITLE =
			QObject::tr("Time to start topology reconstruction from");
	const QString HELP_START_RECONSTRUCTION_AT_TIME_OF_DISAPPEARANCE_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>Reconstruction using topologies starts at an initial geological time which could be present day "
			"or a past geological time.</p>"
			"<ul>"
			"<li>If this check box is ticked then a feature's time of appearance is used as the "
			"initial time for that feature.</li>"
			"<li>Otherwise the feature's geometry import time is used (if the feature has one). "
			"Features digitised using GPlates 2.0 or above record the geometry import time property "
			"(<i>gpml:geometryImportTime</i>) as the geological time the feature was digitised at. "
			"This includes generated crustal thickness points.</li>"
			"<li>Otherwise present day (0Ma) is used.</li>"
			"</ul>"
			"</body></html>\n");

	const QString HELP_DETECT_LIFETIMES_DIALOG_TITLE =
			QObject::tr("Detecting individual point lifetimes");
	const QString HELP_DETECT_LIFETIMES_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>If you choose to have the lifetimes of individual points detected then they can disappear when they are:</p>"
			"<ul>"
			"<li>subducted (going forward in time), or</li>"
			"<li>consumed by a mid-ocean ridge (going backward in time).</li>"
			"</ul>"
			"<p>Otherwise the points never disappear and are just propagated from one plate/network to another over time.</p>"
			"<p>When detecting lifetimes, two parameters can be tweaked to affect the detection algorithm:</p>"
			"<ul>"
			"<li><i>threshold velocity delta</i>: A point that transitions from one plate/network to another can "
			"disappear if the change in velocity exceeds this threshold.</li>"
			"<li><i>threshold distance to boundary</i>: Only those transitioning points exceeding a delta velocity threshold "
			"that are close enough to a plate/network boundary can disappear. This distance depends on the relative velocity. "
			"However a small threshold distance can be added to this velocity-dependent distance to account for plate boundaries "
			"that change shape significantly from one time step to the next (note that some boundaries are meant to do this and "
			"others are a result of digitisation).</li>"
			"</ul>"
			"<p>Furthermore, there is the option to have points inside a deforming network disappear as soon as they "
			"fall outside all deforming networks. This option is enabled by checking the "
			"<b>Deactivate points that fall outside a network</b> check box. This is useful for initial crustal thickness points that have "
			"been generated inside a deforming network and where subsequently deformed points should be limited to the deformed network regions. "
			"In this case sudden large changes to the deforming network boundary can progressively exclude points over time. "
			"However in the case where the topologies (deforming networks and rigid plates) have global coverage this option should "
			"generally be left disabled so that points falling outside deforming networks can then be reconstructed using rigid plates. "
			"And these rigidly reconstructed points may even re-enter a subsequent deforming network.</p>"
			"</body></html>\n");

	const QString HELP_TESSELLATE_LINES_DIALOG_TITLE =
			QObject::tr("Tessellating lines");
	const QString HELP_TESSELLATE_LINES_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>Polyline and polygon geometries are uniformly sampled into points "
			"(rather than retaining the line segments) with a sample spacing that can be controlled.</p>"
			"<p>The individual points of the polyline or polygon can deform (to change the shape of the geometry) "
			"and subduct (if inside an oceanic plate) just as with multipoint geometries.</p>"
			"<p>In the future the line segments will returned.</p>"
			"</body></html>\n");

	const QString HELP_DEFORMED_NETWORK_INTERPOLATION_DIALOG_TITLE =
			QObject::tr("Interpolation in deformed networks");
	const QString HELP_DEFORMED_NETWORK_INTERPOLATION_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>Points falling inside topological networks deform according to the their location within "
			"the network's triangulation.</p>"
			"<ul>"
			"<li>For <i>barycentric</i> interpolation, only the triangle containing the point will deform it.<li>"
			"<li>For <i>natural neighbour</i> interpolation, nearby triangles also contribute to a point's deformation. "
			"This tends to reduce the faceted effect of the triangulation on deformed point positions.<li>"
			"</ul>"
			"</body></html>\n");

	const QString HELP_STRAIN_ACCUMULATION_DIALOG_TITLE =
			QObject::tr("Strain accumulation");
	const QString HELP_STRAIN_ACCUMULATION_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>Total strain is accumulated for each point from oldest age of the time span/range of topology reconstruction to the "
			"current reconstruction time. If strain is displayed then each point will render the principal components of its strain "
			"oriented in the principal directions, with outwards-facing red arrows for extension and inward-facing blue arrows for compression.</p>"
			"</body></html>\n");
}


GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::SetTopologyReconstructionParametersDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		bool only_ok_button,
		QWidget *parent_) :
	QDialog(parent_),
	d_application_state(application_state),
	d_help_start_reconstruction_at_time_of_appearance_dialog(
			new InformationDialog(
					HELP_START_RECONSTRUCTION_AT_TIME_OF_DISAPPEARANCE_DIALOG_TEXT,
					HELP_START_RECONSTRUCTION_AT_TIME_OF_DISAPPEARANCE_DIALOG_TITLE,
					this)),
	d_help_detect_lifetimes_dialog(
			new InformationDialog(
					HELP_DETECT_LIFETIMES_DIALOG_TEXT,
					HELP_DETECT_LIFETIMES_DIALOG_TITLE,
					this)),
	d_help_tessellate_lines_dialog(
			new InformationDialog(
					HELP_TESSELLATE_LINES_DIALOG_TEXT,
					HELP_TESSELLATE_LINES_DIALOG_TITLE,
					this)),
	d_help_deformed_network_interpolation_dialog(
			new InformationDialog(
					HELP_DEFORMED_NETWORK_INTERPOLATION_DIALOG_TEXT,
					HELP_DEFORMED_NETWORK_INTERPOLATION_DIALOG_TITLE,
					this)),
	d_help_strain_accumulation_dialog(
			new InformationDialog(
					HELP_STRAIN_ACCUMULATION_DIALOG_TEXT,
					HELP_STRAIN_ACCUMULATION_DIALOG_TITLE,
					this))
{
	setupUi(this);

	if (only_ok_button)
	{
        main_buttonbox->setStandardButtons(QDialogButtonBox::Ok);
	}

	// Show/hide line tessellation controls if enabling/disabling tessellation.
	detect_lifetime_widget->setVisible(
			enable_detect_lifetime_check_box->isChecked());

	// Show/hide line tessellation controls if enabling/disabling tessellation.
	line_tessellation_widget->setVisible(
			enable_line_tessellation_degrees_check_box->isChecked());

	// Show/hide strain accumulation controls if showing/hiding strain accumulation.
	strain_accumulation_widget->setVisible(
			show_strain_accumulation_checkbox->isChecked());

	setup_connections();

	QtWidgetUtils::resize_based_on_size_hint(this);
}


bool
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::populate(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	// Store pointer so we can write the settings back later.
	d_current_visual_layer = visual_layer;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock())
	{
		// Acquire a pointer to a @a ReconstructParams.
		// NOTE: Make sure we get a 'const' pointer to the reconstruct layer params
		// otherwise it will think we are modifying it which will mean the reconstruct
		// layer will think it needs to regenerate its reconstructed feature geometries.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		const GPlatesAppLogic::ReconstructLayerParams *layer_params =
			dynamic_cast<const GPlatesAppLogic::ReconstructLayerParams *>(
					layer.get_layer_params().get());
		if (!layer_params)
		{
			return false;
		}

		// Acquire a pointer to a @a ReconstructVisualLayerParams.
		const GPlatesPresentation::ReconstructVisualLayerParams *visual_layer_params =
			dynamic_cast<const GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!visual_layer_params)
		{
			return false;
		}

		const GPlatesAppLogic::ReconstructParams &reconstruct_params = layer_params->get_reconstruct_params();

		// Handle delta t.
		spinbox_end_time->setValue(reconstruct_params.get_topology_reconstruction_end_time());
		spinbox_begin_time->setValue(reconstruct_params.get_topology_reconstruction_begin_time());
		spinbox_time_increment->setValue(reconstruct_params.get_topology_reconstruction_time_increment());

		// Deformed position interpolation.
		if (reconstruct_params.get_topology_deformation_use_natural_neighbour_interpolation())
		{
			natural_neighbour_radio_button->setChecked(true);
		}
		else
		{
			barycentric_radio_button->setChecked(true);
		}

		// Whether to start reconstruction at each feature's time of appearance, or use geometry import time.
		start_reconstruction_at_time_of_appearance_checkbox->setChecked(
				reconstruct_params.get_topology_reconstruction_use_time_of_appearance());

		// Line tessellation.
		enable_line_tessellation_degrees_check_box->setChecked(
				reconstruct_params.get_topology_reconstruction_enable_line_tessellation());
		line_tessellation_degrees_spinbox->setValue(
				reconstruct_params.get_topology_reconstruction_line_tessellation_degrees());
		line_tessellation_widget->setVisible(
				reconstruct_params.get_topology_reconstruction_enable_line_tessellation());

		// Lifetime detection.
		enable_detect_lifetime_check_box->setChecked(
				reconstruct_params.get_topology_reconstruction_enable_lifetime_detection());
		detect_lifetime_threshold_velocity_delta_spin_box->setValue(
				reconstruct_params.get_topology_reconstruction_lifetime_detection_threshold_velocity_delta());
		detect_lifetime_threshold_distance_to_boundary_spin_box->setValue(
				reconstruct_params.get_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary());
		deactivate_points_that_fall_outside_a_network_checkbox->setChecked(
				reconstruct_params.get_topology_reconstruction_deactivate_points_that_fall_outside_a_network());
		detect_lifetime_widget->setVisible(
				reconstruct_params.get_topology_reconstruction_enable_lifetime_detection());

		// Show topology-reconstructed feature geometries.
		show_reconstructed_feature_geometries_checkbox->setChecked(
				visual_layer_params->get_show_topology_reconstructed_feature_geometries());

		// Show strain accumulation.
		show_strain_accumulation_checkbox->setChecked(
				visual_layer_params->get_show_strain_accumulation());
		// Set strain accumulation scale.
		strain_accumulation_scale_spinbox->setValue(
				visual_layer_params->get_strain_accumulation_scale());

		return true;
	}

	return false;
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::setup_connections()
{
	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(handle_apply()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));

	QObject::connect(
			spinbox_begin_time, SIGNAL(valueChanged(double)),
			this, SLOT(handle_begin_time_spinbox_changed(double)));
	QObject::connect(
			spinbox_end_time, SIGNAL(valueChanged(double)),
			this, SLOT(handle_end_time_spinbox_changed(double)));
	QObject::connect(
			spinbox_time_increment, SIGNAL(valueChanged(double)),
			this, SLOT(handle_time_increment_spinbox_changed(double)));

	QObject::connect(
			enable_detect_lifetime_check_box, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_detect_lifetime_changed(int)));

	QObject::connect(
			enable_line_tessellation_degrees_check_box, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_line_tessellation_changed(int)));

	QObject::connect(
			show_strain_accumulation_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(react_show_strain_accumulation_changed(int)));

	QObject::connect(
			show_strain_accumulation_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(react_show_strain_accumulation_changed(int)));

	QObject::connect(
			push_button_help_start_reconstruction_at_time_of_appearance, SIGNAL(clicked()),
			d_help_start_reconstruction_at_time_of_appearance_dialog, SLOT(show()));
	QObject::connect(
			push_button_help_detect_lifetimes, SIGNAL(clicked()),
			d_help_detect_lifetimes_dialog, SLOT(show()));
	QObject::connect(
			push_button_help_tessellate_lines, SIGNAL(clicked()),
			d_help_tessellate_lines_dialog, SLOT(show()));
	QObject::connect(
			push_button_help_deformed_network_interpolation, SIGNAL(clicked()),
			d_help_deformed_network_interpolation_dialog, SLOT(show()));
	QObject::connect(
			push_button_help_strain_accumulation, SIGNAL(clicked()),
			d_help_strain_accumulation_dialog, SLOT(show()));
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::handle_begin_time_spinbox_changed(
		double value)
{
	// Keep begin time from getting too close to end time (at the very least they should not be equal).
	spinbox_end_time->setMaximum(value - spinbox_time_increment->value());
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::handle_end_time_spinbox_changed(
		double value)
{
	// Keep begin time from getting too close to end time (at the very least they should not be equal).
	spinbox_begin_time->setMinimum(value + spinbox_time_increment->value());
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::handle_time_increment_spinbox_changed(
		double value)
{
	// Keep begin time from getting too close to end time (at the very least they should not be equal).
	spinbox_begin_time->setMinimum(spinbox_end_time->value() + value);
	spinbox_end_time->setMaximum(spinbox_begin_time->value() - value);
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::react_enable_detect_lifetime_changed(
		int state)
{
	// Show/hide line tessellation controls if enabling/disabling tessellation.
	detect_lifetime_widget->setVisible(
			enable_detect_lifetime_check_box->isChecked());
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::react_enable_line_tessellation_changed(
		int state)
{
	// Show/hide line tessellation controls if enabling/disabling tessellation.
	line_tessellation_widget->setVisible(
			enable_line_tessellation_degrees_check_box->isChecked());
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::react_show_strain_accumulation_changed(
		int state)
{
	// Show/hide strain accumulation controls if showing/hiding strain accumulation.
	strain_accumulation_widget->setVisible(
			show_strain_accumulation_checkbox->isChecked());
}


void
GPlatesQtWidgets::SetTopologyReconstructionParametersDialog::handle_apply()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		// Acquire a pointer to a @a ReconstructParams.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::ReconstructLayerParams *>(
					layer.get_layer_params().get());
		if (!layer_params)
		{
			accept();
		}

		// Acquire a pointer to a @a ReconstructVisualLayerParams.
		GPlatesPresentation::ReconstructVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!visual_layer_params)
		{
			accept();
		}

		{
			// Delay any calls to 'ApplicationState::reconstruct()' until scope exit.
			GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(d_application_state);

			// Handle settings
			GPlatesAppLogic::ReconstructParams reconstruct_params = layer_params->get_reconstruct_params();

			reconstruct_params.set_topology_reconstruction_end_time(spinbox_end_time->value());
			reconstruct_params.set_topology_reconstruction_begin_time(spinbox_begin_time->value());
			reconstruct_params.set_topology_reconstruction_time_increment(spinbox_time_increment->value());

			// Whether to start reconstruction at each feature's time of appearance, or use geometry import time.
			reconstruct_params.set_topology_reconstruction_use_time_of_appearance(
					start_reconstruction_at_time_of_appearance_checkbox->isChecked());

			// Deformed position interpolation.
			reconstruct_params.set_topology_deformation_use_natural_neighbour_interpolation(
					natural_neighbour_radio_button->isChecked());

			// Line tessellation.
			reconstruct_params.set_topology_reconstruction_enable_line_tessellation(
					enable_line_tessellation_degrees_check_box->isChecked());
			reconstruct_params.set_topology_reconstruction_line_tessellation_degrees(
					line_tessellation_degrees_spinbox->value());

			// Lifetime detection.
			reconstruct_params.set_topology_reconstruction_enable_lifetime_detection(
					enable_detect_lifetime_check_box->isChecked());
			reconstruct_params.set_topology_reconstruction_lifetime_detection_threshold_velocity_delta(
					detect_lifetime_threshold_velocity_delta_spin_box->value());
			reconstruct_params.set_topology_reconstruction_lifetime_detection_threshold_distance_to_boundary(
					detect_lifetime_threshold_distance_to_boundary_spin_box->value());
			reconstruct_params.set_topology_reconstruction_deactivate_points_that_fall_outside_a_network(
					deactivate_points_that_fall_outside_a_network_checkbox->isChecked());

			layer_params->set_reconstruct_params(reconstruct_params);

			// If any reconstruct parameters were modified then 'ApplicationState::reconstruct()'
			// will get called here (at scope exit).
		}

		// Show topology-reconstructed feature geometries.
		visual_layer_params->set_show_topology_reconstructed_feature_geometries(
				show_reconstructed_feature_geometries_checkbox->isChecked());

		// Show strain accumulation.
		visual_layer_params->set_show_strain_accumulation(
				show_strain_accumulation_checkbox->isChecked());
		// Set strain accumulation scale.
		visual_layer_params->set_strain_accumulation_scale(
				strain_accumulation_scale_spinbox->value());
	}

	accept();
}
