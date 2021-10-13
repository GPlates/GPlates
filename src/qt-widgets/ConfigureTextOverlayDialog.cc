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
 
#include "ConfigureTextOverlayDialog.h"

#include "ChooseColourButton.h"
#include "ChooseFontButton.h"
#include "QtWidgetUtils.h"

#include "gui/TextOverlaySettings.h"


GPlatesQtWidgets::ConfigureTextOverlayDialog::ConfigureTextOverlayDialog(
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_colour_button(new ChooseColourButton(this)),
	d_font_button(new ChooseFontButton(this))
{
	setupUi(this);
	QtWidgetUtils::add_widget_to_placeholder(
			d_colour_button,
			colour_button_placeholder_widget);
	colour_label->setBuddy(d_colour_button);
	QtWidgetUtils::add_widget_to_placeholder(
			d_font_button,
			font_button_placeholder_widget);
	font_label->setBuddy(d_font_button);

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

	QtWidgetUtils::resize_based_on_size_hint(this);
}


int
GPlatesQtWidgets::ConfigureTextOverlayDialog::exec(
		GPlatesGui::TextOverlaySettings &settings)
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
GPlatesQtWidgets::ConfigureTextOverlayDialog::populate(
		const GPlatesGui::TextOverlaySettings &settings)
{
	text_overlay_groupbox->setChecked(settings.is_enabled());

	text_lineedit->setText(settings.get_text());
	d_font_button->set_font(settings.get_font());
	d_colour_button->set_colour(settings.get_colour());
	anchor_combobox->setCurrentIndex(static_cast<int>(settings.get_anchor()));
	horizontal_offset_spinbox->setValue(settings.get_x_offset());
	vertical_offset_spinbox->setValue(settings.get_y_offset());
	shadow_checkbox->setChecked(settings.has_shadow());
}


void
GPlatesQtWidgets::ConfigureTextOverlayDialog::save(
		GPlatesGui::TextOverlaySettings &settings)
{
	settings.set_enabled(text_overlay_groupbox->isChecked());

	settings.set_text(text_lineedit->text());
	settings.set_font(d_font_button->get_font());
	settings.set_colour(d_colour_button->get_colour());
	settings.set_anchor(static_cast<GPlatesGui::TextOverlaySettings::Anchor>(anchor_combobox->currentIndex()));
	settings.set_x_offset(horizontal_offset_spinbox->value());
	settings.set_y_offset(vertical_offset_spinbox->value());
	settings.set_shadow(shadow_checkbox->isChecked());
}

