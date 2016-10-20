/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include <QLabel>
#include <QVBoxLayout>

#include "ChooseBuiltinPaletteDialog.h"

#include "QtWidgetUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/BuiltinColourPalettes.h"


GPlatesQtWidgets::ChooseBuiltinPaletteDialog::ChooseBuiltinPaletteDialog(
		const GPlatesGui::BuiltinColourPaletteType::Parameters &builtin_parameters,
		QWidget *parent_):
	GPlatesDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_builtin_parameters(builtin_parameters),
	// ColorBrewer sequential multi-hue palettes...
	d_BuGn_button(new ColourScaleButton(this)),
	d_BuPu_button(new ColourScaleButton(this)),
	d_GnBu_button(new ColourScaleButton(this)),
	d_OrRd_button(new ColourScaleButton(this)),
	d_PuBu_button(new ColourScaleButton(this)),
	d_PuBuGn_button(new ColourScaleButton(this)),
	d_PuRd_button(new ColourScaleButton(this)),
	d_RdPu_button(new ColourScaleButton(this)),
	d_YlGn_button(new ColourScaleButton(this)),
	d_YlGnBu_button(new ColourScaleButton(this)),
	d_YlOrBr_button(new ColourScaleButton(this)),
	d_YlOrRd_button(new ColourScaleButton(this)),
	// ColorBrewer sequential single hue palettes...
	d_Blues_button(new ColourScaleButton(this)),
	d_Greens_button(new ColourScaleButton(this)),
	d_Greys_button(new ColourScaleButton(this)),
	d_Oranges_button(new ColourScaleButton(this)),
	d_Purples_button(new ColourScaleButton(this)),
	d_Reds_button(new ColourScaleButton(this)),
	// ColorBrewer diverging palettes...
	d_BrBG_button(new ColourScaleButton(this)),
	d_PiYG_button(new ColourScaleButton(this)),
	d_PRGn_button(new ColourScaleButton(this)),
	d_PuOr_button(new ColourScaleButton(this)),
	d_RdBu_button(new ColourScaleButton(this)),
	d_RdGy_button(new ColourScaleButton(this)),
	d_RdYlBu_button(new ColourScaleButton(this)),
	d_RdYlGn_button(new ColourScaleButton(this)),
	d_Spectral_button(new ColourScaleButton(this))
{
	setupUi(this);

	// ColorBrewer sequential multi-hue palettes.
	add_colour_scale_button(d_BuGn_button, BuGn_placeholder);
	add_colour_scale_button(d_BuPu_button, BuPu_placeholder);
	add_colour_scale_button(d_GnBu_button, GnBu_placeholder);
	add_colour_scale_button(d_OrRd_button, OrRd_placeholder);
	add_colour_scale_button(d_PuBu_button, PuBu_placeholder);
	add_colour_scale_button(d_PuBuGn_button, PuBuGn_placeholder);
	add_colour_scale_button(d_PuRd_button, PuRd_placeholder);
	add_colour_scale_button(d_RdPu_button, RdPu_placeholder);
	add_colour_scale_button(d_YlGn_button, YlGn_placeholder);
	add_colour_scale_button(d_YlGnBu_button, YlGnBu_placeholder);
	add_colour_scale_button(d_YlOrBr_button, YlOrBr_placeholder);
	add_colour_scale_button(d_YlOrRd_button, YlOrRd_placeholder);

	// ColorBrewer sequential single hue palettes.
	add_colour_scale_button(d_Blues_button, Blues_placeholder);
	add_colour_scale_button(d_Greens_button, Greens_placeholder);
	add_colour_scale_button(d_Greys_button, Greys_placeholder);
	add_colour_scale_button(d_Oranges_button, Oranges_placeholder);
	add_colour_scale_button(d_Purples_button, Purples_placeholder);
	add_colour_scale_button(d_Reds_button, Reds_placeholder);

	// ColorBrewer diverging palettes.
	add_colour_scale_button(d_BrBG_button, BrBG_placeholder);
	add_colour_scale_button(d_PiYG_button, PiYG_placeholder);
	add_colour_scale_button(d_PRGn_button, PRGn_placeholder);
	add_colour_scale_button(d_PuOr_button, PuOr_placeholder);
	add_colour_scale_button(d_RdBu_button, RdBu_placeholder);
	add_colour_scale_button(d_RdGy_button, RdGy_placeholder);
	add_colour_scale_button(d_RdYlBu_button, RdYlBu_placeholder);
	add_colour_scale_button(d_RdYlGn_button, RdYlGn_placeholder);
	add_colour_scale_button(d_Spectral_button, Spectral_placeholder);

	colorbrewer_sequential_classes_spinbox->setRange(
			GPlatesGui::BuiltinColourPalettes::ThreeSequentialClasses,
			GPlatesGui::BuiltinColourPalettes::NineSequentialClasses);
	colorbrewer_sequential_classes_spinbox->setSingleStep(1);
	colorbrewer_sequential_classes_spinbox->setValue(d_builtin_parameters.colorbrewer_sequential_classes);
	QObject::connect(
			colorbrewer_sequential_classes_spinbox, SIGNAL(valueChanged(int)),
			this, SLOT(handle_colorbrewer_sequential_classes_changed(int)));

	colorbrewer_diverging_classes_spinbox->setRange(
			GPlatesGui::BuiltinColourPalettes::ThreeDivergingClasses,
			GPlatesGui::BuiltinColourPalettes::ElevenDivergingClasses);
	colorbrewer_diverging_classes_spinbox->setSingleStep(1);
	colorbrewer_diverging_classes_spinbox->setValue(d_builtin_parameters.colorbrewer_diverging_classes);
	QObject::connect(
			colorbrewer_diverging_classes_spinbox, SIGNAL(valueChanged(int)),
			this, SLOT(handle_colorbrewer_diverging_classes_changed(int)));

	colorbrewer_discrete_checkbox->setChecked(!d_builtin_parameters.colorbrewer_continuous);
	QObject::connect(
			colorbrewer_discrete_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(handle_colorbrewer_discrete_check_box_changed(int)));

	colorbrewer_invert_checkbox->setChecked(d_builtin_parameters.colorbrewer_inverted);
	QObject::connect(
			colorbrewer_invert_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(handle_colorbrewer_invert_check_box_changed(int)));

	QtWidgetUtils::resize_based_on_size_hint(this);
}


void
GPlatesQtWidgets::ChooseBuiltinPaletteDialog::add_colour_scale_button(
		ColourScaleButton *colour_scale_button,
		QWidget *colour_scale_button_placeholder)
{
	const GPlatesGui::BuiltinColourPaletteType builtin_colour_palette_type =
			get_builtin_colour_palette_type(colour_scale_button);

	// Replace each placeholder with a colour scale button above a text label.
	QVBoxLayout *layout = new QVBoxLayout(colour_scale_button_placeholder);
	layout->addWidget(colour_scale_button, 0/*stretch*/, Qt::AlignHCenter);
	layout->addWidget(new QLabel(builtin_colour_palette_type.get_palette_name()), 0/*stretch*/, Qt::AlignHCenter);
	layout->setSpacing(1);
	layout->setContentsMargins(0, 0, 0, 0);

	colour_scale_button->populate(builtin_colour_palette_type.create_palette());

	QObject::connect(
			colour_scale_button, SIGNAL(clicked(bool)),
			this, SLOT(handle_colour_scale_button_clicked(bool)));
}


GPlatesGui::BuiltinColourPaletteType
GPlatesQtWidgets::ChooseBuiltinPaletteDialog::get_builtin_colour_palette_type(
		ColourScaleButton *colour_scale_button)
{
	// ColorBrewer sequential multi-hue palettes.
	if (colour_scale_button == d_BuGn_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::BuGn);
	}
	if (colour_scale_button == d_BuPu_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::BuPu);
	}
	if (colour_scale_button == d_GnBu_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::GnBu);
	}
	if (colour_scale_button == d_OrRd_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::OrRd);
	}
	if (colour_scale_button == d_PuBu_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::PuBu);
	}
	if (colour_scale_button == d_PuBuGn_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::PuBuGn);
	}
	if (colour_scale_button == d_PuRd_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::PuRd);
	}
	if (colour_scale_button == d_RdPu_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::RdPu);
	}
	if (colour_scale_button == d_YlGn_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::YlGn);
	}
	if (colour_scale_button == d_YlGnBu_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::YlGnBu);
	}
	if (colour_scale_button == d_YlOrBr_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::YlOrBr);
	}
	if (colour_scale_button == d_YlOrRd_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::YlOrRd);
	}

	// ColorBrewer sequential single hue palettes.
	if (colour_scale_button == d_Blues_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::Blues);
	}
	if (colour_scale_button == d_Greens_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::Greens);
	}
	if (colour_scale_button == d_Greys_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::Greys);
	}
	if (colour_scale_button == d_Oranges_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::Oranges);
	}
	if (colour_scale_button == d_Purples_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::Purples);
	}
	if (colour_scale_button == d_Reds_button)
	{
		return create_colorbrewer_sequential_palette_type(GPlatesGui::BuiltinColourPalettes::Reds);
	}

	// ColorBrewer diverging palettes.
	if (colour_scale_button == d_BrBG_button)
	{
		return create_colorbrewer_diverging_palette_type(GPlatesGui::BuiltinColourPalettes::BrBG);
	}
	if (colour_scale_button == d_PiYG_button)
	{
		return create_colorbrewer_diverging_palette_type(GPlatesGui::BuiltinColourPalettes::PiYG);
	}
	if (colour_scale_button == d_PRGn_button)
	{
		return create_colorbrewer_diverging_palette_type(GPlatesGui::BuiltinColourPalettes::PRGn);
	}
	if (colour_scale_button == d_PuOr_button)
	{
		return create_colorbrewer_diverging_palette_type(GPlatesGui::BuiltinColourPalettes::PuOr);
	}
	if (colour_scale_button == d_RdBu_button)
	{
		return create_colorbrewer_diverging_palette_type(GPlatesGui::BuiltinColourPalettes::RdBu);
	}
	if (colour_scale_button == d_RdGy_button)
	{
		return create_colorbrewer_diverging_palette_type(GPlatesGui::BuiltinColourPalettes::RdGy);
	}
	if (colour_scale_button == d_RdYlBu_button)
	{
		return create_colorbrewer_diverging_palette_type(GPlatesGui::BuiltinColourPalettes::RdYlBu);
	}
	if (colour_scale_button == d_RdYlGn_button)
	{
		return create_colorbrewer_diverging_palette_type(GPlatesGui::BuiltinColourPalettes::RdYlGn);
	}
	if (colour_scale_button == d_Spectral_button)
	{
		return create_colorbrewer_diverging_palette_type(GPlatesGui::BuiltinColourPalettes::Spectral);
	}

	// Shouldn't be able to get here.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			colour_scale_button,
			GPLATES_ASSERTION_SOURCE);
	return GPlatesGui::BuiltinColourPaletteType(GPlatesGui::BuiltinColourPaletteType::AGE_PALETTE/*arbitrary*/);
}


GPlatesGui::BuiltinColourPaletteType
GPlatesQtWidgets::ChooseBuiltinPaletteDialog::create_colorbrewer_sequential_palette_type(
		GPlatesGui::BuiltinColourPalettes::ColorBrewerSequentialType sequential_type)
{
	return GPlatesGui::BuiltinColourPaletteType(sequential_type, d_builtin_parameters);
}


GPlatesGui::BuiltinColourPaletteType
GPlatesQtWidgets::ChooseBuiltinPaletteDialog::create_colorbrewer_diverging_palette_type(
		GPlatesGui::BuiltinColourPalettes::ColorBrewerDivergingType diverging_type)
{
	return GPlatesGui::BuiltinColourPaletteType(diverging_type, d_builtin_parameters);
}


void
GPlatesQtWidgets::ChooseBuiltinPaletteDialog::re_populate_colorbrewer_buttons()
{
	d_BuGn_button->populate(get_builtin_colour_palette_type(d_BuGn_button).create_palette());
	d_BuPu_button->populate(get_builtin_colour_palette_type(d_BuPu_button).create_palette());
	d_GnBu_button->populate(get_builtin_colour_palette_type(d_GnBu_button).create_palette());
	d_OrRd_button->populate(get_builtin_colour_palette_type(d_OrRd_button).create_palette());
	d_PuBu_button->populate(get_builtin_colour_palette_type(d_PuBu_button).create_palette());
	d_PuBuGn_button->populate(get_builtin_colour_palette_type(d_PuBuGn_button).create_palette());
	d_PuRd_button->populate(get_builtin_colour_palette_type(d_PuRd_button).create_palette());
	d_RdPu_button->populate(get_builtin_colour_palette_type(d_RdPu_button).create_palette());
	d_YlGn_button->populate(get_builtin_colour_palette_type(d_YlGn_button).create_palette());
	d_YlGnBu_button->populate(get_builtin_colour_palette_type(d_YlGnBu_button).create_palette());
	d_YlOrBr_button->populate(get_builtin_colour_palette_type(d_YlOrBr_button).create_palette());
	d_YlOrRd_button->populate(get_builtin_colour_palette_type(d_YlOrRd_button).create_palette());

	// ColorBrewer sequential single hue palettes.
	d_Blues_button->populate(get_builtin_colour_palette_type(d_Blues_button).create_palette());
	d_Greens_button->populate(get_builtin_colour_palette_type(d_Greens_button).create_palette());
	d_Greys_button->populate(get_builtin_colour_palette_type(d_Greys_button).create_palette());
	d_Oranges_button->populate(get_builtin_colour_palette_type(d_Oranges_button).create_palette());
	d_Purples_button->populate(get_builtin_colour_palette_type(d_Purples_button).create_palette());
	d_Reds_button->populate(get_builtin_colour_palette_type(d_Reds_button).create_palette());

	// ColorBrewer diverging palettes.
	d_BrBG_button->populate(get_builtin_colour_palette_type(d_BrBG_button).create_palette());
	d_PiYG_button->populate(get_builtin_colour_palette_type(d_PiYG_button).create_palette());
	d_PRGn_button->populate(get_builtin_colour_palette_type(d_PRGn_button).create_palette());
	d_PuOr_button->populate(get_builtin_colour_palette_type(d_PuOr_button).create_palette());
	d_RdBu_button->populate(get_builtin_colour_palette_type(d_RdBu_button).create_palette());
	d_RdGy_button->populate(get_builtin_colour_palette_type(d_RdGy_button).create_palette());
	d_RdYlBu_button->populate(get_builtin_colour_palette_type(d_RdYlBu_button).create_palette());
	d_RdYlGn_button->populate(get_builtin_colour_palette_type(d_RdYlGn_button).create_palette());
	d_Spectral_button->populate(get_builtin_colour_palette_type(d_Spectral_button).create_palette());
}


void
GPlatesQtWidgets::ChooseBuiltinPaletteDialog::handle_colorbrewer_sequential_classes_changed(
		int value)
{
	d_builtin_parameters.colorbrewer_sequential_classes =
			static_cast<GPlatesGui::BuiltinColourPalettes::ColorBrewerSequentialClasses>(value);

	// Redraw the ColorBrewer buttons since number of classes changed.
	re_populate_colorbrewer_buttons();

	Q_EMIT builtin_parameters_changed(d_builtin_parameters);
}


void
GPlatesQtWidgets::ChooseBuiltinPaletteDialog::handle_colorbrewer_diverging_classes_changed(
		int value)
{
	d_builtin_parameters.colorbrewer_diverging_classes =
			static_cast<GPlatesGui::BuiltinColourPalettes::ColorBrewerDivergingClasses>(value);

	// Redraw the ColorBrewer buttons since number of classes changed.
	re_populate_colorbrewer_buttons();

	Q_EMIT builtin_parameters_changed(d_builtin_parameters);
}


void
GPlatesQtWidgets::ChooseBuiltinPaletteDialog::handle_colorbrewer_discrete_check_box_changed(
		int state)
{
	d_builtin_parameters.colorbrewer_continuous = (state != Qt::Checked);

	// Redraw the ColorBrewer buttons since transitioning from discrete to continuous (or vice versa).
	re_populate_colorbrewer_buttons();

	Q_EMIT builtin_parameters_changed(d_builtin_parameters);
}


void
GPlatesQtWidgets::ChooseBuiltinPaletteDialog::handle_colorbrewer_invert_check_box_changed(
		int state)
{
	d_builtin_parameters.colorbrewer_inverted = (state == Qt::Checked);

	// Redraw the ColorBrewer buttons since inverting colours.
	re_populate_colorbrewer_buttons();

	Q_EMIT builtin_parameters_changed(d_builtin_parameters);
}


void
GPlatesQtWidgets::ChooseBuiltinPaletteDialog::handle_colour_scale_button_clicked(
		bool /*checked*/)
{
	// Get the QObject that triggered this slot.
	QObject *signal_sender = sender();
	// Return early in case this slot not activated by a signal - shouldn't happen.
	if (!signal_sender)
	{
		return;
	}

	// Cast to a ColourScaleButton since only ColourScaleButton objects trigger this slot.
	ColourScaleButton *colour_scale_button = qobject_cast<ColourScaleButton *>(signal_sender);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			colour_scale_button,
			GPLATES_ASSERTION_SOURCE);

	const GPlatesGui::BuiltinColourPaletteType builtin_colour_palette_type =
			get_builtin_colour_palette_type(colour_scale_button);

	Q_EMIT builtin_colour_palette_selected(builtin_colour_palette_type);
}
