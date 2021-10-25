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


#ifndef HELLINGERCONFIGURATIONWIDGET_H
#define HELLINGERCONFIGURATIONWIDGET_H

#include <QWidget>

#include "gui/Colour.h"
#include "ui_HellingerConfigurationWidgetUi.h"
//#include "HellingerDialog.h"

namespace GPlatesQtWidgets
{

class HellingerConfigurationWidget :
	public QWidget,
	protected Ui_HellingerConfigurationWidget
{
	Q_OBJECT

public:

	enum HellingerColour{
		BLACK = 0, /*force 0 to match index of combo box */
		WHITE,
		RED,
		GREEN,
		BLUE,
		GREY,
		SILVER,
		MAROON,
		PURPLE,
		FUCHSIA,
		LIME,
		OLIVE,
		YELLOW,
		NAVY,
		TEAL,
		AQUA
	};


	static
	GPlatesGui::Colour
	get_colour_from_hellinger_colour(
			const HellingerColour &hellinger_colour);


	typedef QMap<HellingerColour,QString> colour_description_map_type;

	static const colour_description_map_type &
	build_colour_description_map()
	{
		static colour_description_map_type map;
		map[BLACK] = "Black";
		map[WHITE] = "White";
		map[RED] = "Red";
		map[GREEN] = "Green";
		map[BLUE] = "Blue";
		map[GREY] = "Grey";
		map[SILVER] = "Silver";
		map[MAROON] = "Maroon";
		map[PURPLE] = "Purple";
		map[FUCHSIA] = "Fuchsia";
		map[LIME] = "Lime";
		map[OLIVE] = "Olive";
		map[YELLOW] = "Yellow";
		map[NAVY] = "Navy";
		map[TEAL] = "Teal";
		map[AQUA] = "Aqua";
		return map;
	}


	explicit
	HellingerConfigurationWidget(QWidget *parent = 0);

	~HellingerConfigurationWidget();

	HellingerColour
	best_fit_pole_colour() const;

	HellingerColour
	ellipse_colour() const;

	int
	ellipse_line_thickness() const;

	HellingerColour
	initial_estimate_pole_colour() const;

	float
	pole_arrow_height() const;

	float
	pole_arrow_radius() const;

	void
	set_ellipse_line_thickness(int thickness)
	{
		spinbox_ellipse_thickness->setValue(thickness);
	}

	void
	set_best_fit_pole_colour(
			HellingerColour &colour);

	void
	set_ellipse_colour(
			HellingerColour &colour);

	void
	set_initial_estimate_pole_colour(
			HellingerColour &colour);

	void
	set_pole_arrow_height(
			const float &height);

	void
	set_pole_arrow_radius(
			const float &radius);


Q_SIGNALS:

	/**
	 * @brief configuration_changed
	 *
	 * This lets parent dialogs react accordingly e.g. enabling/disabling the Apply button.
	 *
	 * @param valid - true if current configration is valid.
	 */
	void
	configuration_changed(
			bool valid);


private:



	void
	initialise_widget();

};

}
#endif // HELLINGERCONFIGURATIONWIDGET_H
