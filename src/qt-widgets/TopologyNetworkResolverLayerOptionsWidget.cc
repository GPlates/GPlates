/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <cmath>
#include <boost/shared_ptr.hpp>
#include <QFileInfo>
#include <QDir>
#include <QPalette>

#include "TopologyNetworkResolverLayerOptionsWidget.h"

#include "ColourScaleWidget.h"
#include "DrawStyleDialog.h"
#include "FriendlyLineEdit.h"
#include "LinkWidget.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "app-logic/TopologyNetworkLayerParams.h"
#include "app-logic/TopologyNetworkParams.h"

#include "file-io/CptReader.h"
#include "file-io/ReadErrorAccumulation.h"

#include "global/GPlatesAssert.h"

#include "gui/Colour.h"
#include "gui/ColourPaletteUtils.h"
#include "gui/ColourSpectrum.h"
#include "gui/Dialogs.h"
#include "gui/RasterColourPalette.h"

#include "presentation/TopologyNetworkVisualLayerParams.h"
#include "presentation/VisualLayer.h"

#include "utils/ComponentManager.h"
#include "property-values/XsString.h"


namespace
{
	const QString HELP_STRAIN_RATE_SMOOTHING_DIALOG_TITLE =
			QObject::tr("Smoothing strain rates");
	const QString HELP_STRAIN_RATE_SMOOTHING_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>The strain rate at an arbitrary point location within a deforming network can be smoothed with the following choices:</p>"
			"<ul>"
			"<li><i>No smoothing</i>: The strain rate is constant within each triangle of the network triangulation and is the "
			"gradient of the triangle's three vertex velocities. This gives a faceted appearance to the triangulation when coloured "
			"by dilatation strain rate.</li>"
			"<li><i>Barycentric smoothing</i>: First each vertex is initialised with a strain rate that is an area-weighting of the "
			"strain rates of incident triangles. Then the smoothed strain rate, at an arbitrary point, is a barycentric interpolation "
			"of the smoothed vertex strain rates.</li>"
			"<li><i>Natural neighbour smoothing</i>: First each vertex is initialised with a strain rate that is an area-weighting of the "
			"strain rates of incident triangles. Then the smoothed strain rate, at an arbitrary point, is a natural neighbour interpolation "
			"of the smoothed vertex strain rates. Natural neighbour interpolation uses a larger number of nearby vertices compared to "
			"barycentric (which only interpolates the three vertices of the triangle containing the point).</li>"
			"</ul>"
			"<p>Note that the choice of smoothing affects the strain rates and hence also affects crustal thinning.</p>"
			"</body></html>\n");

	const QString HELP_STRAIN_RATE_CLAMPING_DIALOG_TITLE =
			QObject::tr("Clamping strain rates");
	const QString HELP_STRAIN_RATE_CLAMPING_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>Strain rates can optionally be clamped to a maximum value.</p>"
			"<p>Clamping can be enabled when strain rates in some triangles of the network triangulation are excessively high. "
			"Note clamping is applied to each triangle and this occurs prior to any smoothing of strain rates.</p>"
			"<p>The <i>total</i> strain rate (of each triangle) is clamped since this includes both the normal and shear "
			"components of deformation. When clamped, all components are scaled equally such that the <i>total</i> strain rate "
			"equals the maximum total strain rate.</p>"
			"<p>Note that, by affecting strain rates, clamping can also affect crustal thinning.</p>"
			"</body></html>\n");

	const QString HELP_RIFT_EXPONENTIAL_STRETCHING_CONSTANT_DIALOG_TITLE =
			QObject::tr("Rift exponential stretching constant");
	const QString HELP_RIFT_EXPONENTIAL_STRETCHING_CONSTANT_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>Controls exponential variation of stretching across rift profile in the network triangulation.</p>"
			"<p>The strain rate in the rift stretching direction varies exponentially from un-stretched side of rift towards the rift axis.</p>"
			"<p>The spatial variation in strain rate is SR(x) = SR_constant * exp(C*x) * [C * / (exp(C) - 1)] "
			"where SR_constant is un-subdivided strain rate, C is stretching constant and x=0 at un-stretched side and x=1 at stretched point. "
			"Therefore SR(0) &lt; SR_constant &lt; SR(1). For example, when C=1.0 then SR(0) = 0.58 * SR_constant and SR(1) = 1.58 * SR_constant.</p>"
			"</body></html>\n");

	const QString HELP_RIFT_STRAIN_RATE_RESOLUTION_DIALOG_TITLE =
			QObject::tr("Rift strain rate resolution");
	const QString HELP_RIFT_STRAIN_RATE_RESOLUTION_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>Rift edges in network triangulation are sub-divided until strain rate matches exponential curve within this tolerance "
			"(in units of 1/second).</p>"
			"</body></html>\n");

	const QString HELP_RIFT_EDGE_LENGTH_THRESHOLD_DIALOG_TITLE =
			QObject::tr("Rift edge length threshold");
	const QString HELP_RIFT_EDGE_LENGTH_THRESHOLD_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>Rift edges in network triangulation shorter than this length, in degrees, will not be further sub-divided.</p>"
			"<p>Rifts edges in network triangulation are sub-divided to fit an exponential strain rate profile in the rift stretching direction.</p>"
			"</body></html>\n");

	const QString HELP_TRIANGULATION_COLOUR_MODE_DIALOG_TITLE =
			QObject::tr("Network triangulation colour mode");
	const QString HELP_TRIANGULATION_COLOUR_MODE_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>This selects the colour mode for the <i>triangulation</i> of the network.</p>"
			"<p>By default, the triangulation is coloured with the <i>draw style</i> (this uses the colour scheme selected for the layer).</p>"
			"<p>Alternatively the triangulation can be coloured using <i>dilatation strain rate</i>, "
			"<i>total strain rate</i> or <i>strain rate style</i> and a colour palette.</p>"
			"<p><i>Strain rate style</i> is typically in the range [-1.0, 1.0] where -1.0 implies contraction, "
			"1.0 implies extension and 0.0 implies strike-slip.</p>"
			"<p>Note that the <i>boundary</i> of the network (including any interior rigid blocks) is always coloured with the <i>draw style</i> "
			"regardless of the <i>triangulation colour mode</i>.</p>"
			"</body></html>\n");

	const QString HELP_TRIANGULATION_DRAW_MODE_DIALOG_TITLE =
			QObject::tr("Network triangulation draw mode");
	const QString HELP_TRIANGULATION_DRAW_MODE_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>The wireframe mesh of the triangulation can be displayed with <i>Mesh</i>, or "
			"the triangulation can be filled with <i>Fill</i>, or not displayed with <i>None</i>.</p>"
			"<p>The triangulation is coloured using the <i>triangulation colour mode</i> (when <i>Mesh</i> or <i>Fill</i> is selected).</p>"
			"<p>Note that the <i>boundary</i> of the network (including any interior rigid blocks) is always displayed "
			"regardless of the <i>triangulation draw mode</i>, and is always coloured with the <i>draw style</i>.</p>"
			"<p>Also note that the rigid interior blocks (if any) can be independently filled by selecting <i>fill rigid interior blocks</i> "
			"(and are always coloured with the <i>draw style</i>).</p>"
			"</body></html>\n");
}


const double GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::STRAIN_RATE_SCALE = 1e+17;
const double GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::SCALED_STRAIN_RATE_MIN = 0.0;
const double GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::SCALED_STRAIN_RATE_MAX = 1e+6;
const int GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::SCALED_STRAIN_RATE_DECIMAL_PLACES = 6;
const int GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::RIFT_EXPONENTIAL_STRETCHING_CONSTANT_DECIMAL_PLACES = 3;
const int GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::RIFT_EDGE_LENGTH_THRESHOLD_DECIMAL_PLACES = 3;


GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::TopologyNetworkResolverLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_draw_style_dialog_ptr(&viewport_window->dialogs().draw_style_dialog()),
	d_open_file_dialog( 
		this, 
		tr("Open CPT File"), 
		tr("Regular CPT file (*.cpt);;All files (*)"), 
		view_state),
	d_dilatation_palette_filename_lineedit(
			new FriendlyLineEdit( 
				QString(), 
				tr("Default Palette"), 
				this)),
	d_dilatation_colour_scale_widget(
			new ColourScaleWidget(
				view_state, 
				viewport_window, 
				this)),
	d_second_invariant_palette_filename_lineedit(
			new FriendlyLineEdit( 
				QString(), 
				tr("Default Palette"), 
				this)),
	d_second_invariant_colour_scale_widget(
			new ColourScaleWidget(
				view_state, 
				viewport_window, 
				this)),
	d_strain_rate_style_palette_filename_lineedit(
			new FriendlyLineEdit( 
				QString(), 
				tr("Default Palette"), 
				this)),
	d_strain_rate_style_colour_scale_widget(
			new ColourScaleWidget(
				view_state, 
				viewport_window, 
				this)),
	d_help_strain_rate_smoothing_dialog(
			new InformationDialog(
					HELP_STRAIN_RATE_SMOOTHING_DIALOG_TEXT,
					HELP_STRAIN_RATE_SMOOTHING_DIALOG_TITLE,
					viewport_window)),
	d_help_strain_rate_clamping_dialog(
			new InformationDialog(
					HELP_STRAIN_RATE_CLAMPING_DIALOG_TEXT,
					HELP_STRAIN_RATE_CLAMPING_DIALOG_TITLE,
					viewport_window)),
	d_help_rift_exponential_stretching_constant_dialog(
			new InformationDialog(
					HELP_RIFT_EXPONENTIAL_STRETCHING_CONSTANT_DIALOG_TEXT,
					HELP_RIFT_EXPONENTIAL_STRETCHING_CONSTANT_DIALOG_TITLE,
					viewport_window)),
	d_help_rift_strain_rate_resolution_dialog(
			new InformationDialog(
					HELP_RIFT_STRAIN_RATE_RESOLUTION_DIALOG_TEXT,
					HELP_RIFT_STRAIN_RATE_RESOLUTION_DIALOG_TITLE,
					viewport_window)),
	d_help_rift_edge_length_threshold_dialog(
			new InformationDialog(
					HELP_RIFT_EDGE_LENGTH_THRESHOLD_DIALOG_TEXT,
					HELP_RIFT_EDGE_LENGTH_THRESHOLD_DIALOG_TITLE,
					viewport_window)),
	d_help_triangulation_colour_mode_dialog(
			new InformationDialog(
					HELP_TRIANGULATION_COLOUR_MODE_DIALOG_TEXT,
					HELP_TRIANGULATION_COLOUR_MODE_DIALOG_TITLE,
					viewport_window)),
	d_help_triangulation_draw_mode_dialog(
			new InformationDialog(
					HELP_TRIANGULATION_DRAW_MODE_DIALOG_TEXT,
					HELP_TRIANGULATION_DRAW_MODE_DIALOG_TITLE,
					viewport_window))
{
	setupUi(this);

	fill_rigid_blocks_checkbox->setCursor(QCursor(Qt::ArrowCursor));
	segment_velocity_checkbox->setCursor(QCursor(Qt::ArrowCursor));

	// Smoothing mode.
	no_smoothing_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			no_smoothing_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_strain_rate_smoothing_button(bool)));
	barycentric_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			barycentric_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_strain_rate_smoothing_button(bool)));
	natural_neighbour_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			natural_neighbour_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_strain_rate_smoothing_button(bool)));
	push_button_help_strain_rate_smoothing->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			push_button_help_strain_rate_smoothing, SIGNAL(clicked()),
			d_help_strain_rate_smoothing_dialog, SLOT(show()));

	// Clamping mode.
	enable_clamping_checkbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			enable_clamping_checkbox, SIGNAL(clicked()),
			this, SLOT(handle_strain_rate_clamping_clicked()));
	clamp_strain_rate_line_edit->setCursor(QCursor(Qt::ArrowCursor));
	d_clamp_strain_rate_line_edit_double_validator = 
			new DoubleValidator(
					// Text must be numeric 'double' within a reasonable range of total strain rates.
					// And these values are subsequently multiplied by 'STRAIN_RATE_SCALE'.
					SCALED_STRAIN_RATE_MIN,
					SCALED_STRAIN_RATE_MAX,
					SCALED_STRAIN_RATE_DECIMAL_PLACES,
					clamp_strain_rate_line_edit);
	clamp_strain_rate_line_edit->setValidator(d_clamp_strain_rate_line_edit_double_validator);
	QObject::connect(
			clamp_strain_rate_line_edit, SIGNAL(editingFinished()),
			this, SLOT(handle_strain_rate_clamping_line_editing_finished()));
	push_button_help_strain_rate_clamping->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			push_button_help_strain_rate_clamping, SIGNAL(clicked()),
			d_help_strain_rate_clamping_dialog, SLOT(show()));

	// Rift parameters.
	rift_exponential_stretching_constant_line_edit->setCursor(QCursor(Qt::ArrowCursor));
	d_rift_exponential_stretching_constant_line_edit_double_validator =
			new DoubleValidator(
					1e-3, // min
					100, // max
					RIFT_EXPONENTIAL_STRETCHING_CONSTANT_DECIMAL_PLACES/*decimal places*/,
					rift_exponential_stretching_constant_line_edit);
	rift_exponential_stretching_constant_line_edit->setValidator(d_rift_exponential_stretching_constant_line_edit_double_validator);
	QObject::connect(
			rift_exponential_stretching_constant_line_edit, SIGNAL(editingFinished()),
			this, SLOT(handle_rift_exponential_stretching_constant_line_editing_finished()));
	push_button_help_rift_exponential_stretching_constant->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			push_button_help_rift_exponential_stretching_constant, SIGNAL(clicked()),
			d_help_rift_exponential_stretching_constant_dialog, SLOT(show()));
	rift_strain_rate_resolution_line_edit->setCursor(QCursor(Qt::ArrowCursor));
	d_rift_strain_rate_resolution_line_edit_double_validator =
		new DoubleValidator(
			// Text must be numeric 'double' within a reasonable range of total strain rates.
			// And these values are subsequently multiplied by 'STRAIN_RATE_SCALE'.
			SCALED_STRAIN_RATE_MIN,
			SCALED_STRAIN_RATE_MAX,
			SCALED_STRAIN_RATE_DECIMAL_PLACES,
			rift_strain_rate_resolution_line_edit);
	rift_strain_rate_resolution_line_edit->setValidator(d_rift_strain_rate_resolution_line_edit_double_validator);
	QObject::connect(
			rift_strain_rate_resolution_line_edit, SIGNAL(editingFinished()),
			this, SLOT(handle_rift_strain_rate_resolution_line_editing_finished()));
	push_button_help_rift_strain_rate_resolution->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			push_button_help_rift_strain_rate_resolution, SIGNAL(clicked()),
			d_help_rift_strain_rate_resolution_dialog, SLOT(show()));
	rift_edge_length_threshold_line_edit->setCursor(QCursor(Qt::ArrowCursor));
	d_rift_edge_length_threshold_line_edit_double_validator =
			new DoubleValidator(
					1e-3, // min
					100, // max
					RIFT_EDGE_LENGTH_THRESHOLD_DECIMAL_PLACES/*decimal places*/,
					rift_edge_length_threshold_line_edit);
	rift_edge_length_threshold_line_edit->setValidator(d_rift_edge_length_threshold_line_edit_double_validator);
	QObject::connect(
			rift_edge_length_threshold_line_edit, SIGNAL(editingFinished()),
			this, SLOT(handle_rift_edge_length_threshold_line_editing_finished()));
	push_button_help_rift_edge_length_threshold->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			push_button_help_rift_edge_length_threshold, SIGNAL(clicked()),
			d_help_rift_edge_length_threshold_dialog, SLOT(show()));

	// Colour mode.
	dilatation_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			dilatation_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_colour_mode_button(bool)));
	second_invariant_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			second_invariant_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_colour_mode_button(bool)));
	strain_rate_style_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			strain_rate_style_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_colour_mode_button(bool)));
	default_draw_style_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			default_draw_style_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_colour_mode_button(bool)));
	push_button_help_triangulation_colour_mode->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			push_button_help_triangulation_colour_mode, SIGNAL(clicked()),
			d_help_triangulation_colour_mode_dialog, SLOT(show()));

	// Draw mode.
	none_draw_mode_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			none_draw_mode_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_draw_mode_button(bool)));
	mesh_draw_mode_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			mesh_draw_mode_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_draw_mode_button(bool)));
	fill_draw_mode_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			fill_draw_mode_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_draw_mode_button(bool)));
	push_button_help_triangulation_draw_mode->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			push_button_help_triangulation_draw_mode, SIGNAL(clicked()),
			d_help_triangulation_draw_mode_dialog, SLOT(show()));

	// Set up the dilatation controls.
	min_abs_dilatation_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	max_abs_dilatation_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	select_dilatation_palette_filename_button->setCursor(QCursor(Qt::ArrowCursor));
	use_default_dilatation_palette_button->setCursor(QCursor(Qt::ArrowCursor));
	d_dilatation_palette_filename_lineedit->setReadOnly(true);
	QtWidgetUtils::add_widget_to_placeholder(
		d_dilatation_palette_filename_lineedit,
		dilatation_palette_filename_placeholder_widget);

	// Set up the dilatation colour scale.
	QtWidgetUtils::add_widget_to_placeholder(
			d_dilatation_colour_scale_widget,
			dilatation_colour_scale_placeholder_widget);
	QPalette dilatation_colour_scale_palette = d_dilatation_colour_scale_widget->palette();
	dilatation_colour_scale_palette.setColor(QPalette::Window, Qt::white);
	d_dilatation_colour_scale_widget->setPalette(dilatation_colour_scale_palette);

	// Set up the second invariant controls.
	min_abs_second_invariant_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	max_abs_second_invariant_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	select_second_invariant_palette_filename_button->setCursor(QCursor(Qt::ArrowCursor));
	use_default_second_invariant_palette_button->setCursor(QCursor(Qt::ArrowCursor));
	d_second_invariant_palette_filename_lineedit->setReadOnly(true);
	QtWidgetUtils::add_widget_to_placeholder(
		d_second_invariant_palette_filename_lineedit,
		second_invariant_palette_filename_placeholder_widget);

	// Set up the second invariant colour scale.
	QtWidgetUtils::add_widget_to_placeholder(
			d_second_invariant_colour_scale_widget,
			second_invariant_colour_scale_placeholder_widget);
	QPalette second_invariant_colour_scale_palette = d_second_invariant_colour_scale_widget->palette();
	second_invariant_colour_scale_palette.setColor(QPalette::Window, Qt::white);
	d_second_invariant_colour_scale_widget->setPalette(second_invariant_colour_scale_palette);

	// Set up the strain rate style controls.
	min_strain_rate_style_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	max_strain_rate_style_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	select_strain_rate_style_palette_filename_button->setCursor(QCursor(Qt::ArrowCursor));
	use_default_strain_rate_style_palette_button->setCursor(QCursor(Qt::ArrowCursor));
	d_strain_rate_style_palette_filename_lineedit->setReadOnly(true);
	QtWidgetUtils::add_widget_to_placeholder(
		d_strain_rate_style_palette_filename_lineedit,
		strain_rate_style_palette_filename_placeholder_widget);

	// Set up the strain rate style colour scale.
	QtWidgetUtils::add_widget_to_placeholder(
			d_strain_rate_style_colour_scale_widget,
			strain_rate_style_colour_scale_placeholder_widget);
	QPalette strain_rate_style_colour_scale_palette = d_strain_rate_style_colour_scale_widget->palette();
	strain_rate_style_colour_scale_palette.setColor(QPalette::Window, Qt::white);
	d_strain_rate_style_colour_scale_widget->setPalette(strain_rate_style_colour_scale_palette);

	QObject::connect(
			segment_velocity_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_segment_velocity_clicked()));

	QObject::connect(
			fill_rigid_blocks_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_fill_rigid_blocks_clicked()));

	QObject::connect(
			min_abs_dilatation_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_min_abs_dilatation_spinbox_changed(double)));
	QObject::connect(
			max_abs_dilatation_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_max_abs_dilatation_spinbox_changed(double)));

	QObject::connect(
			select_dilatation_palette_filename_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_select_dilatation_palette_filename_button_clicked()));

	QObject::connect(
			use_default_dilatation_palette_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_use_default_dilatation_palette_button_clicked()));

	QObject::connect(
			min_abs_second_invariant_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_min_abs_second_invariant_spinbox_changed(double)));
	QObject::connect(
			max_abs_second_invariant_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_max_abs_second_invariant_spinbox_changed(double)));

	QObject::connect(
			select_second_invariant_palette_filename_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_select_second_invariant_palette_filename_button_clicked()));

	QObject::connect(
			use_default_second_invariant_palette_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_use_default_second_invariant_palette_button_clicked()));

	QObject::connect(
			min_strain_rate_style_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_min_strain_rate_style_spinbox_changed(double)));
	QObject::connect(
			max_strain_rate_style_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_max_strain_rate_style_spinbox_changed(double)));

	QObject::connect(
			select_strain_rate_style_palette_filename_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_select_strain_rate_style_palette_filename_button_clicked()));

	QObject::connect(
			use_default_strain_rate_style_palette_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_use_default_strain_rate_style_palette_button_clicked()));

	fill_opacity_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			fill_opacity_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(handle_fill_opacity_spinbox_changed(double)));

	fill_intensity_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			fill_intensity_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(handle_fill_intensity_spinbox_changed(double)));

	LinkWidget *draw_style_link = new LinkWidget(
			tr("Set Draw style..."), this);
	QtWidgetUtils::add_widget_to_placeholder(
			draw_style_link,
			draw_style_placeholder_widget);
	QObject::connect(
			draw_style_link,
			SIGNAL(link_activated()),
			this,
			SLOT(open_draw_style_setting_dlg()));
	if(!GPlatesUtils::ComponentManager::instance().is_enabled(GPlatesUtils::ComponentManager::Component::python()))
	{
		draw_style_link->setVisible(false);
	}
}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new TopologyNetworkResolverLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	// Set the state of the checkboxes.
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = 
			d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::TopologyNetworkLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::TopologyNetworkLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			const GPlatesAppLogic::TopologyNetworkParams &topology_network_params =
					layer_params->get_topology_network_params();

			// Smoothing Mode.
			//
			// Changing the current mode will emit signals which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.
			QObject::disconnect(
					no_smoothing_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			QObject::disconnect(
					barycentric_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			QObject::disconnect(
					natural_neighbour_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			switch (topology_network_params.get_strain_rate_smoothing())
			{
			case GPlatesAppLogic::TopologyNetworkParams::NO_SMOOTHING:
				no_smoothing_radio_button->setChecked(true);
				break;
			case GPlatesAppLogic::TopologyNetworkParams::BARYCENTRIC_SMOOTHING:
				barycentric_radio_button->setChecked(true);
				break;
			case GPlatesAppLogic::TopologyNetworkParams::NATURAL_NEIGHBOUR_SMOOTHING:
				natural_neighbour_radio_button->setChecked(true);
				break;
			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
			QObject::connect(
					no_smoothing_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			QObject::connect(
					barycentric_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			QObject::connect(
					natural_neighbour_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));

			// Clamping mode.
			//
			const GPlatesAppLogic::TopologyNetworkParams::StrainRateClamping &strain_rate_clamping =
					topology_network_params.get_strain_rate_clamping();
			const double scaled_max_total_strain_rate =
					strain_rate_clamping.max_total_strain_rate * STRAIN_RATE_SCALE;
			// Since QLineEdit::setText() does not validate we need to expand the validator's
			// acceptable range to include our clamp value before we set it.
			if (scaled_max_total_strain_rate < d_clamp_strain_rate_line_edit_double_validator->bottom())
			{
				d_clamp_strain_rate_line_edit_double_validator->setBottom(scaled_max_total_strain_rate);
			}
			else if (scaled_max_total_strain_rate > d_clamp_strain_rate_line_edit_double_validator->top())
			{
				d_clamp_strain_rate_line_edit_double_validator->setTop(scaled_max_total_strain_rate);
			}
			// Changing the line edit text might(?) emit signals which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.
			QObject::disconnect(
					clamp_strain_rate_line_edit, SIGNAL(editingFinished()),
					this, SLOT(handle_strain_rate_clamping_line_editing_finished()));
			// Use same locale (to convert double to text) as line edit validator (to convert text to double).
			clamp_strain_rate_line_edit->setText(
					clamp_strain_rate_line_edit->validator()->locale().toString(
							scaled_max_total_strain_rate, 'f', SCALED_STRAIN_RATE_DECIMAL_PLACES));
			QObject::connect(
					clamp_strain_rate_line_edit, SIGNAL(editingFinished()),
					this, SLOT(handle_strain_rate_clamping_line_editing_finished()));

			enable_clamping_checkbox->setChecked(strain_rate_clamping.enable_clamping);
			// All clamping controls only apply if 'Enable clamping' is checked.
			clamp_strain_rate_parameters_widget->setEnabled(
					enable_clamping_checkbox->isChecked());

			// Rift parameters.
			//
			const GPlatesAppLogic::TopologyNetworkParams::RiftParams &rift_params =
					topology_network_params.get_rift_params();

			// Since QLineEdit::setText() does not validate we need to expand the validator's
			// acceptable range to include our value before we set it.
			if (rift_params.exponential_stretching_constant < d_rift_exponential_stretching_constant_line_edit_double_validator->bottom())
			{
				d_rift_exponential_stretching_constant_line_edit_double_validator->setBottom(rift_params.exponential_stretching_constant);
			}
			else if (rift_params.exponential_stretching_constant > d_rift_exponential_stretching_constant_line_edit_double_validator->top())
			{
				d_rift_exponential_stretching_constant_line_edit_double_validator->setTop(rift_params.exponential_stretching_constant);
			}
			// Changing the line edit text might(?) emit signals which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.
			QObject::disconnect(
					rift_exponential_stretching_constant_line_edit, SIGNAL(editingFinished()),
					this, SLOT(handle_rift_exponential_stretching_constant_line_editing_finished()));
			// Use same locale (to convert double to text) as line edit validator (to convert text to double).
			rift_exponential_stretching_constant_line_edit->setText(
					rift_exponential_stretching_constant_line_edit->validator()->locale().toString(
							rift_params.exponential_stretching_constant, 'f', RIFT_EXPONENTIAL_STRETCHING_CONSTANT_DECIMAL_PLACES));
			QObject::connect(
					rift_exponential_stretching_constant_line_edit, SIGNAL(editingFinished()),
					this, SLOT(handle_rift_exponential_stretching_constant_line_editing_finished()));

			const double scaled_strain_rate_resolution = rift_params.strain_rate_resolution * STRAIN_RATE_SCALE;
			// Since QLineEdit::setText() does not validate we need to expand the validator's
			// acceptable range to include our value before we set it.
			if (scaled_strain_rate_resolution < d_rift_strain_rate_resolution_line_edit_double_validator->bottom())
			{
				d_rift_strain_rate_resolution_line_edit_double_validator->setBottom(scaled_strain_rate_resolution);
			}
			else if (scaled_strain_rate_resolution > d_rift_strain_rate_resolution_line_edit_double_validator->top())
			{
				d_rift_strain_rate_resolution_line_edit_double_validator->setTop(scaled_strain_rate_resolution);
			}
			// Changing the line edit text might(?) emit signals which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.
			QObject::disconnect(
					rift_strain_rate_resolution_line_edit, SIGNAL(editingFinished()),
					this, SLOT(handle_rift_strain_rate_resolution_line_editing_finished()));
			// Use same locale (to convert double to text) as line edit validator (to convert text to double).
			rift_strain_rate_resolution_line_edit->setText(
					rift_strain_rate_resolution_line_edit->validator()->locale().toString(
							scaled_strain_rate_resolution, 'f', SCALED_STRAIN_RATE_DECIMAL_PLACES));
			QObject::connect(
					rift_strain_rate_resolution_line_edit, SIGNAL(editingFinished()),
					this, SLOT(handle_rift_strain_rate_resolution_line_editing_finished()));

			// Since QLineEdit::setText() does not validate we need to expand the validator's
			// acceptable range to include our value before we set it.
			if (rift_params.edge_length_threshold_degrees < d_rift_edge_length_threshold_line_edit_double_validator->bottom())
			{
				d_rift_edge_length_threshold_line_edit_double_validator->setBottom(rift_params.edge_length_threshold_degrees);
			}
			else if (rift_params.edge_length_threshold_degrees > d_rift_edge_length_threshold_line_edit_double_validator->top())
			{
				d_rift_edge_length_threshold_line_edit_double_validator->setTop(rift_params.edge_length_threshold_degrees);
			}
			// Changing the line edit text might(?) emit signals which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.
			QObject::disconnect(
				rift_edge_length_threshold_line_edit, SIGNAL(editingFinished()),
				this, SLOT(handle_rift_edge_length_threshold_line_editing_finished()));
			// Use same locale (to convert double to text) as line edit validator (to convert text to double).
			rift_edge_length_threshold_line_edit->setText(
					rift_edge_length_threshold_line_edit->validator()->locale().toString(
							rift_params.edge_length_threshold_degrees, 'f', RIFT_EDGE_LENGTH_THRESHOLD_DECIMAL_PLACES));
			QObject::connect(
				rift_edge_length_threshold_line_edit, SIGNAL(editingFinished()),
				this, SLOT(handle_rift_edge_length_threshold_line_editing_finished()));
		}

		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());

		if (params)
		{
			// Check boxes.
			fill_rigid_blocks_checkbox->setChecked(params->get_fill_rigid_blocks());
			segment_velocity_checkbox->setChecked(params->show_segment_velocity());

			// Colour Mode.
			//
			// Changing the current mode will emit signals which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.
			QObject::disconnect(
					dilatation_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			QObject::disconnect(
					second_invariant_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			QObject::disconnect(
					strain_rate_style_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			QObject::disconnect(
					default_draw_style_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			switch (params->get_triangulation_colour_mode())
			{
			case GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DILATATION_STRAIN_RATE:
				dilatation_radio_button->setChecked(true);
				break;
			case GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_SECOND_INVARIANT_STRAIN_RATE:
				second_invariant_radio_button->setChecked(true);
				break;
			case GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_STRAIN_RATE_STYLE:
				strain_rate_style_radio_button->setChecked(true);
				break;
			case GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DRAW_STYLE:
				default_draw_style_radio_button->setChecked(true);
				break;
			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
			QObject::connect(
					dilatation_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			QObject::connect(
					second_invariant_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			QObject::connect(
					strain_rate_style_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));
			QObject::connect(
					default_draw_style_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_colour_mode_button(bool)));

			// Draw Mode.
			//
			// Changing the current mode will emit signals which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.
			QObject::disconnect(
					none_draw_mode_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_draw_mode_button(bool)));
			QObject::disconnect(
					mesh_draw_mode_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_draw_mode_button(bool)));
			QObject::disconnect(
					fill_draw_mode_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_draw_mode_button(bool)));
			switch (params->get_triangulation_draw_mode())
			{
			case GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_NONE:
				none_draw_mode_radio_button->setChecked(true);
				break;
			case GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_MESH:
				mesh_draw_mode_radio_button->setChecked(true);
				break;
			case GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_FILL:
				fill_draw_mode_radio_button->setChecked(true);
				break;
			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
			QObject::connect(
					none_draw_mode_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_draw_mode_button(bool)));
			QObject::connect(
					mesh_draw_mode_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_draw_mode_button(bool)));
			QObject::connect(
					fill_draw_mode_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_draw_mode_button(bool)));

			//
			// Dilatation.
			//

			// Populate the dilatation palette filename.
			d_dilatation_palette_filename_lineedit->setText(params->get_dilatation_colour_palette_filename());

			QObject::disconnect(
					min_abs_dilatation_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_min_abs_dilatation_spinbox_changed(double)));
			QObject::disconnect(
					max_abs_dilatation_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_max_abs_dilatation_spinbox_changed(double)));
			// Set the dilatation values.
			// Scale to a reasonable range since we only have a finite number of decimal places in spinbox.
			min_abs_dilatation_spinbox->setValue(params->get_min_abs_dilatation() * STRAIN_RATE_SCALE);
			max_abs_dilatation_spinbox->setValue(params->get_max_abs_dilatation() * STRAIN_RATE_SCALE);
			QObject::connect(
					min_abs_dilatation_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_min_abs_dilatation_spinbox_changed(double)));
			QObject::connect(
					max_abs_dilatation_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_max_abs_dilatation_spinbox_changed(double)));

			if (params->get_dilatation_colour_palette())
			{
				// Start with a reasonable default value.
				boost::optional<double> use_log_scale(4.0);

				// Specify a tailored value if using default palette.
				if (params->get_dilatation_colour_palette_filename().isEmpty())
				{
					// Both values should be positive but we'll check just to be sure.
					if (params->get_max_abs_dilatation() > params->get_min_abs_dilatation() &&
						params->get_min_abs_dilatation() > 0)
					{
						// The default min/max range includes zero so specify how close the log scaling gets to zero.
						// Increase it a little beyond the abs(min)/abs(max range) so that values in
						// range [-min, min] around zero show up in the colour scale.
						use_log_scale = 1.05 * (std::log10(params->get_max_abs_dilatation()) - std::log10(params->get_min_abs_dilatation()));
					}
				}

				d_dilatation_colour_scale_widget->populate(
						GPlatesGui::RasterColourPalette::create<double>(
								params->get_dilatation_colour_palette().get()),
						use_log_scale);
			}
			else
			{
				d_dilatation_colour_scale_widget->populate(
						GPlatesGui::RasterColourPalette::create());
			}

			// Only show default dilatation palette controls when not using a user-defined palette.
			dilatation_default_palette_controls->setVisible(
					params->get_dilatation_colour_palette_filename().isEmpty());

			//
			// Second invariant.
			//

			// Populate the second invariant palette filename.
			d_second_invariant_palette_filename_lineedit->setText(params->get_second_invariant_colour_palette_filename());

			QObject::disconnect(
					min_abs_second_invariant_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_min_abs_second_invariant_spinbox_changed(double)));
			QObject::disconnect(
					max_abs_second_invariant_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_max_abs_second_invariant_spinbox_changed(double)));
			// Set the second invariant values.
			// Scale to a reasonable range since we only have a finite number of decimal places in spinbox.
			min_abs_second_invariant_spinbox->setValue(params->get_min_abs_second_invariant() * STRAIN_RATE_SCALE);
			max_abs_second_invariant_spinbox->setValue(params->get_max_abs_second_invariant() * STRAIN_RATE_SCALE);
			QObject::connect(
					min_abs_second_invariant_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_min_abs_second_invariant_spinbox_changed(double)));
			QObject::connect(
					max_abs_second_invariant_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_max_abs_second_invariant_spinbox_changed(double)));

			if (params->get_second_invariant_colour_palette())
			{
				// Start with a reasonable default value.
				boost::optional<double> use_log_scale(4.0);

				// Specify a tailored value if using default palette.
				if (params->get_second_invariant_colour_palette_filename().isEmpty())
				{
					// Both values should be positive but we'll check just to be sure.
					if (params->get_max_abs_second_invariant() > params->get_min_abs_second_invariant() &&
						params->get_min_abs_second_invariant() > 0)
					{
						// The default min/max range includes zero so specify how close the log scaling gets to zero.
						// Increase it a little beyond the abs(min)/abs(max range) so that values in
						// range [-min, min] around zero show up in the colour scale.
						use_log_scale = 1.05 * (std::log10(params->get_max_abs_second_invariant()) - std::log10(params->get_min_abs_second_invariant()));
					}
				}

				d_second_invariant_colour_scale_widget->populate(
						GPlatesGui::RasterColourPalette::create<double>(
								params->get_second_invariant_colour_palette().get()),
						use_log_scale);
			}
			else
			{
				d_second_invariant_colour_scale_widget->populate(
						GPlatesGui::RasterColourPalette::create());
			}

			// Only show default second invariant palette controls when not using a user-defined palette.
			second_invariant_default_palette_controls->setVisible(
					params->get_second_invariant_colour_palette_filename().isEmpty());

			//
			// Strain rate style.
			//

			// Populate the strain rate style palette filename.
			d_strain_rate_style_palette_filename_lineedit->setText(params->get_strain_rate_style_colour_palette_filename());

			QObject::disconnect(
					min_strain_rate_style_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_min_strain_rate_style_spinbox_changed(double)));
			QObject::disconnect(
					max_strain_rate_style_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_max_strain_rate_style_spinbox_changed(double)));
			// Set the strain rate style values.
			min_strain_rate_style_spinbox->setValue(params->get_min_strain_rate_style());
			max_strain_rate_style_spinbox->setValue(params->get_max_strain_rate_style());
			QObject::connect(
					min_strain_rate_style_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_min_strain_rate_style_spinbox_changed(double)));
			QObject::connect(
					max_strain_rate_style_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_max_strain_rate_style_spinbox_changed(double)));

			if (params->get_strain_rate_style_colour_palette())
			{
				d_strain_rate_style_colour_scale_widget->populate(
						GPlatesGui::RasterColourPalette::create<double>(
								params->get_strain_rate_style_colour_palette().get()));
			}
			else
			{
				d_strain_rate_style_colour_scale_widget->populate(
						GPlatesGui::RasterColourPalette::create());
			}

			// Only show default strain rate style palette controls when not using a user-defined palette.
			strain_rate_style_default_palette_controls->setVisible(
					params->get_strain_rate_style_colour_palette_filename().isEmpty());


			// Setting the values in the spin boxes will emit signals if the value changes
			// which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.

			QObject::disconnect(
					fill_opacity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_fill_opacity_spinbox_changed(double)));
			fill_opacity_spinbox->setValue(params->get_fill_opacity());
			QObject::connect(
					fill_opacity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_fill_opacity_spinbox_changed(double)));

			QObject::disconnect(
					fill_intensity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_fill_intensity_spinbox_changed(double)));
			fill_intensity_spinbox->setValue(params->get_fill_intensity());
			QObject::connect(
					fill_intensity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_fill_intensity_spinbox_changed(double)));

			//
			// Enable/disable colour mode options.
			//

			switch (params->get_triangulation_colour_mode())
			{
			case GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DILATATION_STRAIN_RATE:
				dilatation_group_box->show();
				second_invariant_group_box->hide();
				strain_rate_style_group_box->hide();
				break;

			case GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_SECOND_INVARIANT_STRAIN_RATE:
				second_invariant_group_box->show();
				dilatation_group_box->hide();
				strain_rate_style_group_box->hide();
				break;

			case GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_STRAIN_RATE_STYLE:
				strain_rate_style_group_box->show();
				dilatation_group_box->hide();
				second_invariant_group_box->hide();
				break;

			case GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DRAW_STYLE:
				dilatation_group_box->hide();
				second_invariant_group_box->hide();
				strain_rate_style_group_box->hide();
				break;

			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
		}
	}
}


const QString &
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::get_title()
{
	static const QString TITLE = tr("Network & Triangulation options");
	return TITLE;
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_strain_rate_smoothing_button(
		bool checked)
{
	// All radio buttons in the group are connected to the same slot (this method).
	// Hence there will be *two* calls to this slot even though there's only *one* user action (clicking a button).
	// One slot call is for the button that is toggled off and the other slot call for the button toggled on.
	// However we handle all buttons in one call to this slot so it should only be called once.
	// So we only look at one signal.
	// We arbitrarily choose the signal from the button toggled *on* (*off* would have worked fine too).
	if (!checked)
	{
		return;
	}

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::TopologyNetworkLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::TopologyNetworkLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			GPlatesAppLogic::TopologyNetworkParams topology_network_params = layer_params->get_topology_network_params();

			if (no_smoothing_radio_button->isChecked())
			{
				topology_network_params.set_strain_rate_smoothing(
						GPlatesAppLogic::TopologyNetworkParams::NO_SMOOTHING);
			}
			if (barycentric_radio_button->isChecked())
			{
				topology_network_params.set_strain_rate_smoothing(
						GPlatesAppLogic::TopologyNetworkParams::BARYCENTRIC_SMOOTHING);
			}
			if (natural_neighbour_radio_button->isChecked())
			{
				topology_network_params.set_strain_rate_smoothing(
						GPlatesAppLogic::TopologyNetworkParams::NATURAL_NEIGHBOUR_SMOOTHING);
			}

			layer_params->set_topology_network_params(topology_network_params);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_strain_rate_clamping_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::TopologyNetworkLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::TopologyNetworkLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			// All clamping controls only apply if 'Enable clamping' is checked.
			//
			// Note: We enable/disable before updating the layer params since the latter can take
			// a long time (since triggers full topology reconstruction through time history)
			// and we'd like the enable/disable status to be visible while that's happening
			// (otherwise user might think GPlates has crashed or just not responded to their click).
			clamp_strain_rate_parameters_widget->setEnabled(
					enable_clamping_checkbox->isChecked());

			GPlatesAppLogic::TopologyNetworkParams topology_network_params = layer_params->get_topology_network_params();

			GPlatesAppLogic::TopologyNetworkParams::StrainRateClamping strain_rate_clamping =
					topology_network_params.get_strain_rate_clamping();

			strain_rate_clamping.enable_clamping = enable_clamping_checkbox->isChecked();

			topology_network_params.set_strain_rate_clamping(strain_rate_clamping);

			layer_params->set_topology_network_params(topology_network_params);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_strain_rate_clamping_line_editing_finished()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::TopologyNetworkLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::TopologyNetworkLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			GPlatesAppLogic::TopologyNetworkParams topology_network_params = layer_params->get_topology_network_params();

			GPlatesAppLogic::TopologyNetworkParams::StrainRateClamping strain_rate_clamping =
					topology_network_params.get_strain_rate_clamping();

			// Conversion to double assuming the system locale, falling back to C locale.
			bool ok;
			double scaled_max_total_strain_rate = d_clamp_strain_rate_line_edit_double_validator->locale().toDouble(
					clamp_strain_rate_line_edit->text(), &ok);
			if (!ok)
			{
				// QString::toDouble() only uses C locale.
				scaled_max_total_strain_rate = clamp_strain_rate_line_edit->text().toDouble(&ok);
			}
			if (ok)
			{
				strain_rate_clamping.max_total_strain_rate = scaled_max_total_strain_rate / STRAIN_RATE_SCALE;
				topology_network_params.set_strain_rate_clamping(strain_rate_clamping);
			}

			layer_params->set_topology_network_params(topology_network_params);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_rift_exponential_stretching_constant_line_editing_finished()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::TopologyNetworkLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::TopologyNetworkLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			GPlatesAppLogic::TopologyNetworkParams topology_network_params = layer_params->get_topology_network_params();

			GPlatesAppLogic::TopologyNetworkParams::RiftParams rift_params = topology_network_params.get_rift_params();

			// Conversion to double assuming the system locale, falling back to C locale.
			bool ok;
			double exponential_stretching_constant = d_rift_exponential_stretching_constant_line_edit_double_validator->locale().toDouble(
					rift_exponential_stretching_constant_line_edit->text(), &ok);
			if (!ok)
			{
				// QString::toDouble() only uses C locale.
				exponential_stretching_constant = rift_exponential_stretching_constant_line_edit->text().toDouble(&ok);
			}
			if (ok)
			{
				rift_params.exponential_stretching_constant = exponential_stretching_constant;
				topology_network_params.set_rift_params(rift_params);
			}

			layer_params->set_topology_network_params(topology_network_params);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_rift_strain_rate_resolution_line_editing_finished()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::TopologyNetworkLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::TopologyNetworkLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			GPlatesAppLogic::TopologyNetworkParams topology_network_params = layer_params->get_topology_network_params();

			GPlatesAppLogic::TopologyNetworkParams::RiftParams rift_params = topology_network_params.get_rift_params();

			// Conversion to double assuming the system locale, falling back to C locale.
			bool ok;
			double scaled_rift_strain_rate_resolution = d_rift_strain_rate_resolution_line_edit_double_validator->locale().toDouble(
					rift_strain_rate_resolution_line_edit->text(), &ok);
			if (!ok)
			{
				// QString::toDouble() only uses C locale.
				scaled_rift_strain_rate_resolution = rift_strain_rate_resolution_line_edit->text().toDouble(&ok);
			}
			if (ok)
			{
				rift_params.strain_rate_resolution = scaled_rift_strain_rate_resolution / STRAIN_RATE_SCALE;
				topology_network_params.set_rift_params(rift_params);
			}

			layer_params->set_topology_network_params(topology_network_params);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_rift_edge_length_threshold_line_editing_finished()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::TopologyNetworkLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::TopologyNetworkLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			GPlatesAppLogic::TopologyNetworkParams topology_network_params = layer_params->get_topology_network_params();

			GPlatesAppLogic::TopologyNetworkParams::RiftParams rift_params = topology_network_params.get_rift_params();

			// Conversion to double assuming the system locale, falling back to C locale.
			bool ok;
			double edge_length_threshold_degrees = d_rift_edge_length_threshold_line_edit_double_validator->locale().toDouble(
					rift_edge_length_threshold_line_edit->text(), &ok);
			if (!ok)
			{
				// QString::toDouble() only uses C locale.
				edge_length_threshold_degrees = rift_edge_length_threshold_line_edit->text().toDouble(&ok);
			}
			if (ok)
			{
				rift_params.edge_length_threshold_degrees = edge_length_threshold_degrees;
				topology_network_params.set_rift_params(rift_params);
			}

			layer_params->set_topology_network_params(topology_network_params);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_fill_rigid_blocks_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_fill_rigid_blocks(fill_rigid_blocks_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_segment_velocity_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_segment_velocity(segment_velocity_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_colour_mode_button(
		bool checked)
{
	// All radio buttons in the group are connected to the same slot (this method).
	// Hence there will be *two* calls to this slot even though there's only *one* user action (clicking a button).
	// One slot call is for the button that is toggled off and the other slot call for the button toggled on.
	// However we handle all buttons in one call to this slot so it should only be called once.
	// So we only look at one signal.
	// We arbitrarily choose the signal from the button toggled *on* (*off* would have worked fine too).
	if (!checked)
	{
		return;
	}

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			if (dilatation_radio_button->isChecked())
			{
				params->set_triangulation_colour_mode(
						GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DILATATION_STRAIN_RATE);
			}
			if (second_invariant_radio_button->isChecked())
			{
				params->set_triangulation_colour_mode(
						GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_SECOND_INVARIANT_STRAIN_RATE);
			}
			if (strain_rate_style_radio_button->isChecked())
			{
				params->set_triangulation_colour_mode(
						GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_STRAIN_RATE_STYLE);
			}
			if (default_draw_style_radio_button->isChecked())
			{
				params->set_triangulation_colour_mode(
						GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_COLOUR_DRAW_STYLE);
			}
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_draw_mode_button(
		bool checked)
{
	// All radio buttons in the group are connected to the same slot (this method).
	// Hence there will be *two* calls to this slot even though there's only *one* user action (clicking a button).
	// One slot call is for the button that is toggled off and the other slot call for the button toggled on.
	// However we handle all buttons in one call to this slot so it should only be called once.
	// So we only look at one signal.
	// We arbitrarily choose the signal from the button toggled *on* (*off* would have worked fine too).
	if (!checked)
	{
		return;
	}

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			if (none_draw_mode_radio_button->isChecked())
			{
				params->set_triangulation_draw_mode(
						GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_NONE);
			}
			if (mesh_draw_mode_radio_button->isChecked())
			{
				params->set_triangulation_draw_mode(
						GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_MESH);
			}
			if (fill_draw_mode_radio_button->isChecked())
			{
				params->set_triangulation_draw_mode(
						GPlatesPresentation::TopologyNetworkVisualLayerParams::TRIANGULATION_DRAW_FILL);
			}
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_min_abs_dilatation_spinbox_changed(
		double min_abs_dilatation)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			// Scale to a reasonable range since we only have a finite number of decimal places in spinbox.
			if (min_abs_dilatation > params->get_max_abs_dilatation() * STRAIN_RATE_SCALE)
			{
				// Setting the spinbox value will trigger this slot again so return after setting.
				min_abs_dilatation_spinbox->setValue(params->get_max_abs_dilatation() * STRAIN_RATE_SCALE);
				return;
			}

			params->set_min_abs_dilatation(min_abs_dilatation / STRAIN_RATE_SCALE);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_max_abs_dilatation_spinbox_changed(
		double max_abs_dilatation)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			// Scale to a reasonable range since we only have a finite number of decimal places in spinbox.
			if (max_abs_dilatation < params->get_min_abs_dilatation() * STRAIN_RATE_SCALE)
			{
				// Setting the spinbox value will trigger this slot again so return after setting.
				max_abs_dilatation_spinbox->setValue(params->get_min_abs_dilatation() * STRAIN_RATE_SCALE);
				return;
			}

			params->set_max_abs_dilatation(max_abs_dilatation / STRAIN_RATE_SCALE);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_select_dilatation_palette_filename_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!params)
		{
			return;
		}

		QString palette_file_name = d_open_file_dialog.get_open_file_name();
		if (palette_file_name.isEmpty())
		{
			return;
		}

		d_view_state.get_last_open_directory() = QFileInfo(palette_file_name).path();

		GPlatesFileIO::ReadErrorAccumulation cpt_read_errors;
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
				GPlatesGui::ColourPaletteUtils::read_cpt_raster_colour_palette(
						palette_file_name,
						// We only allow real-valued colour palettes since our data is real-valued...
						false/*allow_integer_colour_palette*/,
						cpt_read_errors);

		// If we successfully read a real-valued colour palette.
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> colour_palette =
				GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(*raster_colour_palette);
		if (colour_palette)
		{
			params->set_dilatation_colour_palette(palette_file_name, colour_palette.get());

			d_dilatation_palette_filename_lineedit->setText(QDir::toNativeSeparators(palette_file_name));
		}

		// Show any read errors.
		if (cpt_read_errors.size() > 0)
		{
			d_viewport_window->handle_read_errors(cpt_read_errors);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_use_default_dilatation_palette_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->use_default_dilatation_colour_palette();
		}
	}
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_min_abs_second_invariant_spinbox_changed(
		double min_abs_second_invariant)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			// Scale to a reasonable range since we only have a finite number of decimal places in spinbox.
			if (min_abs_second_invariant > params->get_max_abs_second_invariant() * STRAIN_RATE_SCALE)
			{
				// Setting the spinbox value will trigger this slot again so return after setting.
				min_abs_second_invariant_spinbox->setValue(params->get_max_abs_second_invariant() * STRAIN_RATE_SCALE);
				return;
			}

			params->set_min_abs_second_invariant(min_abs_second_invariant / STRAIN_RATE_SCALE);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_max_abs_second_invariant_spinbox_changed(
		double max_abs_second_invariant)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			// Scale to a reasonable range since we only have a finite number of decimal places in spinbox.
			if (max_abs_second_invariant < params->get_min_abs_second_invariant() * STRAIN_RATE_SCALE)
			{
				// Setting the spinbox value will trigger this slot again so return after setting.
				max_abs_second_invariant_spinbox->setValue(params->get_min_abs_second_invariant() * STRAIN_RATE_SCALE);
				return;
			}

			params->set_max_abs_second_invariant(max_abs_second_invariant / STRAIN_RATE_SCALE);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_select_second_invariant_palette_filename_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!params)
		{
			return;
		}

		QString palette_file_name = d_open_file_dialog.get_open_file_name();
		if (palette_file_name.isEmpty())
		{
			return;
		}

		d_view_state.get_last_open_directory() = QFileInfo(palette_file_name).path();

		GPlatesFileIO::ReadErrorAccumulation cpt_read_errors;
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
				GPlatesGui::ColourPaletteUtils::read_cpt_raster_colour_palette(
						palette_file_name,
						// We only allow real-valued colour palettes since our data is real-valued...
						false/*allow_integer_colour_palette*/,
						cpt_read_errors);

		// If we successfully read a real-valued colour palette.
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> colour_palette =
				GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(*raster_colour_palette);
		if (colour_palette)
		{
			params->set_second_invariant_colour_palette(palette_file_name, colour_palette.get());

			d_second_invariant_palette_filename_lineedit->setText(QDir::toNativeSeparators(palette_file_name));
		}

		// Show any read errors.
		if (cpt_read_errors.size() > 0)
		{
			d_viewport_window->handle_read_errors(cpt_read_errors);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_use_default_second_invariant_palette_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->use_default_second_invariant_colour_palette();
		}
	}
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_min_strain_rate_style_spinbox_changed(
		double min_strain_rate_style)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			if (min_strain_rate_style > params->get_max_strain_rate_style())
			{
				// Setting the spinbox value will trigger this slot again so return after setting.
				min_strain_rate_style_spinbox->setValue(params->get_max_strain_rate_style());
				return;
			}

			params->set_min_strain_rate_style(min_strain_rate_style);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_max_strain_rate_style_spinbox_changed(
		double max_strain_rate_style)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			if (max_strain_rate_style < params->get_min_strain_rate_style())
			{
				// Setting the spinbox value will trigger this slot again so return after setting.
				max_strain_rate_style_spinbox->setValue(params->get_min_strain_rate_style());
				return;
			}

			params->set_max_strain_rate_style(max_strain_rate_style);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_select_strain_rate_style_palette_filename_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!params)
		{
			return;
		}

		QString palette_file_name = d_open_file_dialog.get_open_file_name();
		if (palette_file_name.isEmpty())
		{
			return;
		}

		d_view_state.get_last_open_directory() = QFileInfo(palette_file_name).path();

		GPlatesFileIO::ReadErrorAccumulation cpt_read_errors;
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
				GPlatesGui::ColourPaletteUtils::read_cpt_raster_colour_palette(
						palette_file_name,
						// We only allow real-valued colour palettes since our data is real-valued...
						false/*allow_integer_colour_palette*/,
						cpt_read_errors);

		// If we successfully read a real-valued colour palette.
		boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> colour_palette =
				GPlatesGui::RasterColourPaletteExtract::get_colour_palette<double>(*raster_colour_palette);
		if (colour_palette)
		{
			params->set_strain_rate_style_colour_palette(palette_file_name, colour_palette.get());

			d_strain_rate_style_palette_filename_lineedit->setText(QDir::toNativeSeparators(palette_file_name));
		}

		// Show any read errors.
		if (cpt_read_errors.size() > 0)
		{
			d_viewport_window->handle_read_errors(cpt_read_errors);
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_use_default_strain_rate_style_palette_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->use_default_strain_rate_style_colour_palette();
		}
	}
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_fill_opacity_spinbox_changed(
		double value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_fill_opacity(value);
		}
	}
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_fill_intensity_spinbox_changed(
		double value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_fill_intensity(value);
		}
	}
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::open_draw_style_setting_dlg()
{
	QtWidgetUtils::pop_up_dialog(d_draw_style_dialog_ptr);
	d_draw_style_dialog_ptr->reset(d_current_visual_layer);
}

