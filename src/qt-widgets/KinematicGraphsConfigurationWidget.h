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

#ifndef GPLATES_QTWIDGETS_KINEMATICGRAPHSCONFIGURATIONWIDGET_H
#define GPLATES_QTWIDGETS_KINEMATICGRAPHSCONFIGURATIONWIDGET_H

#include <QWidget>

#include "gui/ConfigGuiUtils.h"

#include "ui_KinematicGraphsConfigurationWidgetUi.h"

namespace GPlatesQtWidgets
{

	class KinematicGraphsConfigurationWidget :
			public QWidget,
			protected Ui_KinematicGraphsConfigurationWidget
	{
		Q_OBJECT

	public:

		enum VelocityMethod{
			T_TO_T_MINUS_DT = 0,
			T_PLUS_DT_TO_T,
			T_PLUS_MINUS_HALF_DT
		};

		static const GPlatesGui::ConfigGuiUtils::ConfigButtonGroupAdapter::button_enum_to_description_map_type &
		build_velocity_method_description_map()
		{
			static GPlatesGui::ConfigGuiUtils::ConfigButtonGroupAdapter::button_enum_to_description_map_type map;
			map[T_PLUS_DT_TO_T] = "(T+dt)_to_T";
			map[T_TO_T_MINUS_DT] = "T_to_(T-dt)";
			map[T_PLUS_MINUS_HALF_DT] = "(T+dt/2)_to_(T-dt/2)";
			return map;
		}


		explicit
		KinematicGraphsConfigurationWidget(QWidget *parent = 0);

		~KinematicGraphsConfigurationWidget();

		QDoubleSpinBox *
		delta_time_spinbox()
		{
			return spinbox_dt;
		}

		QDoubleSpinBox *
		velocity_yellow_spinbox()
		{
			return spinbox_yellow;
		}

		QDoubleSpinBox *
		velocity_red_spinbox()
		{
			return spinbox_red;
		}

		QButtonGroup *
		velocity_method_button_group()
		{
			return button_group_velocity_method;
		}


		double delta_time()
		{
			return spinbox_dt->value();
		}

		void
		set_delta_time(
				double delta_time_)
		{
			spinbox_dt->setValue(delta_time_);
		}

		double yellow_velocity_threshold()
		{
			return spinbox_yellow->value();
		}

		void
		set_yellow_velocity_threshold(
				double yellow)
		{
			spinbox_yellow->setValue(yellow);
		}

		double red_velocity_threshold()
		{
			return spinbox_red->value();
		}

		void
		set_red_velocity_threshold(
				double red)
		{
			spinbox_red->setValue(red);
		}

		VelocityMethod
		velocity_method();

		void
		set_velocity_method(
				const VelocityMethod &method)
		{
			d_velocity_method = method;
			QAbstractButton *button = button_group_velocity_method->button(method);

			if (button)
			{
				button->setChecked(true);
			}
		}

	Q_SIGNALS:

		/**
		 * @brief configuration_changed - emitted when the dt spinbox has changed.
		 *
		 * This lets parent dialogs react accordingly e.g. enabling/disabling the Apply button.
		 *
		 * @param valid - true if current configration is valid.
		 */
		void
		configuration_changed(
				bool valid);

	private Q_SLOTS:

		void
		handle_velocity_method_changed();

		void
		handle_delta_time_changed();

		void
		handle_velocity_yellow_changed();

		void
		handle_velocity_red_changed();

	private:

		VelocityMethod d_velocity_method;

		/**
		 * @brief d_spin_box_palette - the palette used in the delta_time spinboxe. Stored so that we can
		 * restore the original palette after changing to a warning palette.
		 */
		QPalette d_spin_box_palette;
	};

}

#endif // GPLATES_QTWIDGETS_KINEMATICGRAPHSCONFIGURATIONWIDGET_H
