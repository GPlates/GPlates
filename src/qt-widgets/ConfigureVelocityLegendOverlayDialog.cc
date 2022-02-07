/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2017 Geological Survey of Norway
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
 
#include "ConfigureVelocityLegendOverlayDialog.h"

#include "ChooseFontButton.h"
#include "InformationDialog.h"
#include "QtWidgetUtils.h"
#include "VisualLayersComboBox.h"

#include "app-logic/LayerTaskType.h"
#include "gui/VelocityLegendOverlaySettings.h"
#include "presentation/ViewState.h"
#include "presentation/VisualLayers.h"

const QString GPlatesQtWidgets::ConfigureVelocityLegendOverlayDialog::s_fixed_scale_text = QObject::tr(
			"<html><body>\n"
			"<h3>Fixed scale</h3>"
			"The scale (cm/yr) of the arrow is fixed, but the legend's arrow length will change "
			"as appropriate in response to changes in zoom, or changes in the Arrow Scale in the Layers dialog."
			"</body></html>\n");

const QString GPlatesQtWidgets::ConfigureVelocityLegendOverlayDialog::s_max_arrow_length_text = QObject::tr(
			"<html><body>\n"
			"<h3>Maximum arrow length</h3>"
			"The arrow length on the screen (in pixels) will not exceed the value provided by the user. The scale used (cm/yr) will "
			"be the largest multiple of 1,2,5, 10 etc which satisfies the user-provided maximum arrow length. "
			"<p>"
			"The scale "
			"will change as appropriate in response to changes in zoom, or changes in the Arrow Scale in the "
			"Layers dialog."
			"</body></html>\n");


GPlatesQtWidgets::ConfigureVelocityLegendOverlayDialog::ConfigureVelocityLegendOverlayDialog(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	GPlatesDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_scale_text_colour_button(new ColourButton(this)),
	d_arrow_colour_button(new ColourButton(this)),
	d_background_colour_button(new ColourButton(this)),
	d_scale_text_font_button(new ChooseFontButton(this)),
	d_visual_layers_combo_box(
		new VisualLayersComboBox(
			view_state.get_visual_layers(),
			view_state.get_visual_layer_registry(),
			[] (GPlatesPresentation::VisualLayerType::Type layer_type)
			{ return layer_type == GPlatesAppLogic::LayerTaskType::VELOCITY_FIELD_CALCULATOR; } )),
	d_fixed_scale_help_dialog(new InformationDialog(s_fixed_scale_text,QObject::tr("Fixed scale"),this)),
	d_max_arrow_length_help_dialog(new InformationDialog(s_max_arrow_length_text,QObject::tr("Maximum arrow length"),this))
{
	setupUi(this);
	QtWidgetUtils::add_widget_to_placeholder(
			d_scale_text_colour_button,
			scale_text_colour_placeholder_widget);
	label_scale_colour->setBuddy(d_scale_text_colour_button);
	QtWidgetUtils::add_widget_to_placeholder(
			d_scale_text_font_button,
			scale_text_font_placeholder_widget);
	label_scale_font->setBuddy(d_scale_text_font_button);
	QtWidgetUtils::add_widget_to_placeholder(
			d_arrow_colour_button,
			arrow_colour_placeholder_widget);
	label_arrow_colour->setBuddy(d_arrow_colour_button);
	QtWidgetUtils::add_widget_to_placeholder(
				d_background_colour_button,
				background_colour_placeholder_widget);
	label_background_colour->setBuddy(d_background_colour_button);

	QtWidgetUtils::add_widget_to_placeholder(
			d_visual_layers_combo_box,
			widget_combo_placeholder);
	label_velocity_layer->setBuddy(d_visual_layers_combo_box);

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

	QObject::connect(
				radio_button_arrow_length,
				SIGNAL(clicked()),
				this,
				SLOT(handle_radio_buttons_checked()));
	QObject::connect(
				radio_button_scale,
				SIGNAL(clicked()),
				this,
				SLOT(handle_radio_buttons_checked()));

	QObject::connect(
				button_help_fixed_scale,
				SIGNAL(clicked()),
				d_fixed_scale_help_dialog,
				SLOT(show()));

	QObject::connect(
				button_help_maximum_length,
				SIGNAL(clicked()),
				d_max_arrow_length_help_dialog,
				SLOT(show()));

	//NOTE: This configure-dialog is modal, and nothing is updated on the globe until after the
	// dialog is accepted, at which point everything is stored in VelocityLegendOverlaySettings.
	// So we don't need to react to any changed of the combo box here. It might be desirable to
	// change the scale on the fly though, so that users can see the effects of changing the settings
	// without having to close the dialog.
#if 0
	QObject::connect(
			d_visual_layers_combo_box,
			SIGNAL(selected_visual_layer_changed(
					   boost::weak_ptr<GPlatesPresentation::VisualLayer>)),
			this,
			SLOT(handle_visual_layer_changed()));
#endif

	QtWidgetUtils::resize_based_on_size_hint(this);

	radio_button_arrow_length->setChecked(true);
	spinbox_angle->setSuffix("\260");

}


int
GPlatesQtWidgets::ConfigureVelocityLegendOverlayDialog::exec(
		GPlatesGui::VelocityLegendOverlaySettings &settings)
{
	populate(settings);
	int dialog_code = QDialog::exec();
	if (dialog_code == QDialog::Accepted)
	{
		save(settings);
	}
	return dialog_code;
}

void
GPlatesQtWidgets::ConfigureVelocityLegendOverlayDialog::handle_radio_buttons_checked()
{
	if (radio_button_arrow_length->isChecked())
	{
		spinbox_length->setFocus();
	}
	else
	{
		spinbox_scale->setFocus();
	}
}

void
GPlatesQtWidgets::ConfigureVelocityLegendOverlayDialog::populate(
		const GPlatesGui::VelocityLegendOverlaySettings &settings)
{
	velocity_legend_overlay_groupbox->setChecked(settings.is_enabled());

	d_scale_text_font_button->set_font(settings.get_scale_text_font());
	d_scale_text_colour_button->set_colour(settings.get_scale_text_colour());
	d_arrow_colour_button->set_colour(settings.get_arrow_colour());
	d_background_colour_button->set_colour(settings.get_background_colour());
	anchor_combobox->setCurrentIndex(static_cast<int>(settings.get_anchor()));
	horizontal_offset_spinbox->setValue(settings.get_x_offset());
	vertical_offset_spinbox->setValue(settings.get_y_offset());
	spinbox_length->setValue(settings.get_arrow_length());
	spinbox_angle->setValue(settings.get_arrow_angle());
	spinbox_scale->setValue(settings.get_arrow_scale());
	checkbox_show_background->setChecked(settings.background_enabled());
	radio_button_arrow_length->setChecked(settings.get_arrow_length_type() ==
										GPlatesGui::VelocityLegendOverlaySettings::MAXIMUM_ARROW_LENGTH);
	d_visual_layers_combo_box->set_selected_visual_layer(
			settings.get_selected_velocity_layer());
}


void
GPlatesQtWidgets::ConfigureVelocityLegendOverlayDialog::save(
		GPlatesGui::VelocityLegendOverlaySettings &settings)
{
	settings.set_enabled(velocity_legend_overlay_groupbox->isChecked());

	settings.set_scale_text_font(d_scale_text_font_button->get_font());
	settings.set_scale_text_colour(d_scale_text_colour_button->get_colour());
	settings.set_arrow_colour(d_arrow_colour_button->get_colour());
	settings.set_background_colour(d_background_colour_button->get_colour());
	settings.set_anchor(static_cast<GPlatesGui::VelocityLegendOverlaySettings::Anchor>(anchor_combobox->currentIndex()));
	settings.set_x_offset(horizontal_offset_spinbox->value());
	settings.set_y_offset(vertical_offset_spinbox->value());
	settings.set_arrow_length(spinbox_length->value());
	settings.set_arrow_angle(spinbox_angle->value());
	settings.set_arrow_scale(spinbox_scale->value());
	settings.set_background_enabled(checkbox_show_background->isChecked());
	settings.set_arrow_length_type(radio_button_arrow_length->isChecked() ?
									   GPlatesGui::VelocityLegendOverlaySettings::MAXIMUM_ARROW_LENGTH :
									   GPlatesGui::VelocityLegendOverlaySettings::DYNAMIC_ARROW_LENGTH);
	settings.set_selected_velocity_layer(d_visual_layers_combo_box->get_selected_visual_layer());
}

GPlatesQtWidgets::ColourButton::ColourButton(
		QWidget *parent_) :
	QToolButton(parent_)
{
	set_colour(GPlatesGui::Colour::get_white());

	QObject::connect(
			this,
			SIGNAL(clicked()),
			this,
			SLOT(handle_clicked()));
}


void
GPlatesQtWidgets::ColourButton::set_colour(
		const GPlatesGui::Colour &colour)
{
	if (d_colour == colour)
	{
		return;
	}

	d_colour = colour;

	// Set tooltip to display R, G and B of colour.
	GPlatesGui::rgba8_t rgba = GPlatesGui::Colour::to_rgba8(colour);
	static const char *TOOLTIP_TEMPLATE = QT_TR_NOOP("(%1, %2, %3)");
	QString tooltip = QObject::tr(TOOLTIP_TEMPLATE).arg(rgba.red).arg(rgba.green).arg(rgba.blue);
	setToolTip(tooltip);

	// Create an icon to display the colour.
	QPixmap pixmap(iconSize());
	pixmap.fill(colour);
	setIcon(QIcon(pixmap));

	Q_EMIT colour_changed(*this);
}


void
GPlatesQtWidgets::ColourButton::handle_clicked()
{
	QColorDialog dialog;
	dialog.setOption(QColorDialog::ShowAlphaChannel);
	dialog.setCurrentColor(d_colour);
	if (dialog.exec() == QDialog::Accepted)
	{
		set_colour(dialog.currentColor());
	}
}

