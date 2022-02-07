/**
 * \file
 * $Revision: 12148 $
 * $Date: 2011-08-18 14:01:47 +0200 (Thu, 18 Aug 2011) $
 *
 * Copyright (C) 2015 Geological Survey of Norway
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

#include "HellingerConfigurationWidget.h"



GPlatesGui::Colour
GPlatesQtWidgets::HellingerConfigurationWidget::get_colour_from_hellinger_colour(
		const GPlatesQtWidgets::HellingerConfigurationWidget::HellingerColour &hellinger_colour)
{
	switch(hellinger_colour)
	{
	case GPlatesQtWidgets::HellingerConfigurationWidget::BLACK:
		return GPlatesGui::Colour::get_black();
	case GPlatesQtWidgets::HellingerConfigurationWidget::WHITE:
		return GPlatesGui::Colour::get_white();
	case GPlatesQtWidgets::HellingerConfigurationWidget::RED:
		return GPlatesGui::Colour::get_red();
	case GPlatesQtWidgets::HellingerConfigurationWidget::GREEN:
		return GPlatesGui::Colour::get_green();

	case GPlatesQtWidgets::HellingerConfigurationWidget::BLUE:
		return GPlatesGui::Colour::get_blue();
	case GPlatesQtWidgets::HellingerConfigurationWidget::GREY:
		return GPlatesGui::Colour::get_grey();
	case GPlatesQtWidgets::HellingerConfigurationWidget::SILVER:
		return GPlatesGui::Colour::get_silver();
	case GPlatesQtWidgets::HellingerConfigurationWidget::MAROON:
		return GPlatesGui::Colour::get_maroon();

	case GPlatesQtWidgets::HellingerConfigurationWidget::PURPLE:
		return GPlatesGui::Colour::get_purple();
	case GPlatesQtWidgets::HellingerConfigurationWidget::FUCHSIA:
		return GPlatesGui::Colour::get_fuchsia();
	case GPlatesQtWidgets::HellingerConfigurationWidget::LIME:
		return GPlatesGui::Colour::get_lime();
	case GPlatesQtWidgets::HellingerConfigurationWidget::OLIVE:
		return GPlatesGui::Colour::get_olive();

	case GPlatesQtWidgets::HellingerConfigurationWidget::YELLOW:
		return GPlatesGui::Colour::get_yellow();
	case GPlatesQtWidgets::HellingerConfigurationWidget::NAVY:
		return GPlatesGui::Colour::get_navy();
	case GPlatesQtWidgets::HellingerConfigurationWidget::TEAL:
		return GPlatesGui::Colour::get_teal();
	case GPlatesQtWidgets::HellingerConfigurationWidget::AQUA:
		return GPlatesGui::Colour::get_aqua();
	}

	return GPlatesGui::Colour::get_red();
}

GPlatesQtWidgets::HellingerConfigurationWidget::HellingerConfigurationWidget(
		QWidget *parent_) :
	QWidget(parent_)
{
	setupUi(this);

	initialise_widget();
}

GPlatesQtWidgets::HellingerConfigurationWidget::~HellingerConfigurationWidget()
{
}

GPlatesQtWidgets::HellingerConfigurationWidget::HellingerColour
GPlatesQtWidgets::HellingerConfigurationWidget::best_fit_pole_colour() const
{
	int index = combo_best_fit_pole_colour->currentIndex();

	return static_cast<HellingerColour>(index);
}

GPlatesQtWidgets::HellingerConfigurationWidget::HellingerColour
GPlatesQtWidgets::HellingerConfigurationWidget::ellipse_colour() const
{
	int index = combo_ellipse_colour->currentIndex();

	return static_cast<HellingerColour>(index);
}

int
GPlatesQtWidgets::HellingerConfigurationWidget::ellipse_line_thickness() const
{
	return spinbox_ellipse_thickness->value();
}

GPlatesQtWidgets::HellingerConfigurationWidget::HellingerColour
GPlatesQtWidgets::HellingerConfigurationWidget::initial_estimate_pole_colour() const
{
	int index = combo_initial_estimate_pole_colour->currentIndex();

	return static_cast<HellingerColour>(index);
}

float
GPlatesQtWidgets::HellingerConfigurationWidget::pole_arrow_height() const
{
	return spinbox_arrow_height->value();
}

float
GPlatesQtWidgets::HellingerConfigurationWidget::pole_arrow_radius() const
{
	return spinbox_arrow_radius->value();
}

void
GPlatesQtWidgets::HellingerConfigurationWidget::set_best_fit_pole_colour(
		GPlatesQtWidgets::HellingerConfigurationWidget::HellingerColour &colour)
{
	combo_best_fit_pole_colour->setCurrentIndex(colour);
}

void
GPlatesQtWidgets::HellingerConfigurationWidget::set_ellipse_colour(
		GPlatesQtWidgets::HellingerConfigurationWidget::HellingerColour &colour)
{
	combo_ellipse_colour->setCurrentIndex(colour);
}

void
GPlatesQtWidgets::HellingerConfigurationWidget::set_initial_estimate_pole_colour(
		GPlatesQtWidgets::HellingerConfigurationWidget::HellingerColour &colour)
{
	combo_initial_estimate_pole_colour->setCurrentIndex(colour);
}

void
GPlatesQtWidgets::HellingerConfigurationWidget::set_pole_arrow_height(
		const float &height_)
{
	spinbox_arrow_height->setValue(height_);
}

void
GPlatesQtWidgets::HellingerConfigurationWidget::set_pole_arrow_radius(
		const float &radius)
{
	spinbox_arrow_radius->setValue(radius);
}


void
GPlatesQtWidgets::HellingerConfigurationWidget::initialise_widget()
{
	static const colour_description_map_type
			map = build_colour_description_map();

	Q_FOREACH(QString colour_string, map)
	{
		combo_best_fit_pole_colour->addItem(colour_string);
		combo_ellipse_colour->addItem(colour_string);
		combo_initial_estimate_pole_colour->addItem(colour_string);
	}

}
