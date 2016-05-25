/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include <limits>
#include <QDir>
#include <QDoubleValidator>
#include <QFileInfo>
#include <QPalette>

#include "RemappedColourPaletteWidget.h"

#include "ColourScaleWidget.h"
#include "FriendlyLineEdit.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "gui/RasterColourPalette.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::RemappedColourPaletteWidget::RemappedColourPaletteWidget(
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_,
		QWidget *extra_widget) :
	QWidget(parent_),
	d_palette_filename_lineedit(
			new FriendlyLineEdit(
				QString(),
				tr("Default Palette"),
				this)),
	d_colour_scale_widget(
			new ColourScaleWidget(
				view_state,
				viewport_window,
				this))
{
	setupUi(this);


	if (extra_widget)
	{
		QtWidgetUtils::add_widget_to_placeholder(extra_widget, extra_placeholder_widget);
	}
	else
	{
		// Remove the extra placeholder - it's only used if clients add extra widgets to us.
		extra_placeholder_widget->setVisible(false);
	}


	select_palette_filename_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			select_palette_filename_button, SIGNAL(clicked()),
			this, SLOT(handle_select_palette_filename_button_clicked()));
	use_default_palette_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			use_default_palette_button, SIGNAL(clicked()),
			this, SLOT(handle_use_default_palette_button_clicked()));

	d_palette_filename_lineedit->setReadOnly(true);
	QtWidgetUtils::add_widget_to_placeholder(
			d_palette_filename_lineedit,
			palette_filename_placeholder_widget);

	QtWidgetUtils::add_widget_to_placeholder(
			d_colour_scale_widget,
			colour_scale_placeholder_widget);
	QPalette colour_scale_palette = d_colour_scale_widget->palette();
	colour_scale_palette.setColor(QPalette::Window, Qt::white);
	d_colour_scale_widget->setPalette(colour_scale_palette);

	range_check_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			range_check_box, SIGNAL(stateChanged(int)),
			this, SLOT(handle_range_check_box_changed(int)));

	min_line_edit->setCursor(QCursor(Qt::ArrowCursor));
	// Text must be numeric 'double'.
	min_line_edit->setValidator(
			// The parentheses around min/max are to prevent the windows min/max macros
			// from stuffing numeric_limits' min/max...
			new QDoubleValidator(
					-(std::numeric_limits<double>::max)(),
					(std::numeric_limits<double>::max)(),
					6,
					min_line_edit));
	QObject::connect(
			min_line_edit, SIGNAL(editingFinished()),
			this, SLOT(handle_min_line_editing_finished()));

	max_line_edit->setCursor(QCursor(Qt::ArrowCursor));
	// Text must be numeric 'double'.
	max_line_edit->setValidator(
			// The parentheses around min/max are to prevent the windows min/max macros
			// from stuffing numeric_limits' min/max...
			new QDoubleValidator(
					-(std::numeric_limits<double>::max)(),
					(std::numeric_limits<double>::max)(),
					6,
					max_line_edit));
	QObject::connect(
			max_line_edit, SIGNAL(editingFinished()),
			this, SLOT(handle_max_line_editing_finished()));

	range_restore_min_max_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			range_restore_min_max_button, SIGNAL(clicked()),
			this, SLOT(handle_range_restore_min_max_button_clicked()));
	range_restore_mean_deviation_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			range_restore_mean_deviation_button, SIGNAL(clicked()),
			this, SLOT(handle_range_restore_mean_deviation_button_clicked()));
	range_restore_mean_deviation_spin_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			range_restore_mean_deviation_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_range_restore_mean_deviation_spinbox_changed(double)));
}


void
GPlatesQtWidgets::RemappedColourPaletteWidget::set_parameters(
		const GPlatesPresentation::RemappedColourPaletteParameters &parameters)
{
	// Load the colour palette into the colour scale widget.
	const bool show_scalar_colour_scale = d_colour_scale_widget->populate(parameters.get_colour_palette());
	colour_scale_placeholder_widget->setVisible(show_scalar_colour_scale);

	// Populate the palette filename.
	d_palette_filename_lineedit->setText(parameters.get_colour_palette_filename());

	// Set the palette range check box.
	QObject::disconnect(
			range_check_box, SIGNAL(stateChanged(int)),
			this, SLOT(handle_range_check_box_changed(int)));
	if (parameters.is_palette_range_mapped())
	{
		range_check_box->setChecked(true);
	}
	else
	{
		range_check_box->setChecked(false);

		// If the colour palette is integer (categorical) then we don't want the user to be
		// able to select remapping (because integer palettes cannot be remapped) so
		// we hide the entire range group box.
		range_mapping_group_box->setVisible(
				GPlatesGui::RasterColourPaletteType::get_type(*parameters.get_colour_palette()) ==
						GPlatesGui::RasterColourPaletteType::DOUBLE);
	}
	QObject::connect(
			range_check_box, SIGNAL(stateChanged(int)),
			this, SLOT(handle_range_check_box_changed(int)));

	// Set the scalar colour palette range for when it is explicitly mapped by the user controls.
	QObject::disconnect(
			min_line_edit, SIGNAL(editingFinished()),
			this, SLOT(handle_min_line_editing_finished()));
	QObject::disconnect(
			max_line_edit, SIGNAL(editingFinished()),
			this, SLOT(handle_max_line_editing_finished()));
	if (parameters.is_palette_range_mapped())
	{
		min_line_edit->setText(QString::number(parameters.get_palette_range().first, 'f'));
		max_line_edit->setText(QString::number(parameters.get_palette_range().second, 'f'));
	}
	QObject::connect(
			min_line_edit, SIGNAL(editingFinished()),
			this, SLOT(handle_min_line_editing_finished()));
	QObject::connect(
			max_line_edit, SIGNAL(editingFinished()),
			this, SLOT(handle_max_line_editing_finished()));

	QObject::disconnect(
			range_restore_mean_deviation_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_range_restore_mean_deviation_spinbox_changed(double)));
	range_restore_mean_deviation_spin_box->setValue(parameters.get_deviation_from_mean());
	QObject::connect(
			range_restore_mean_deviation_spin_box, SIGNAL(valueChanged(double)),
			this, SLOT(handle_range_restore_mean_deviation_spinbox_changed(double)));

	// Finally we need to show/hide the range widget.
	if (parameters.is_palette_range_mapped())
	{
		range_widget->show();
	}
	else
	{
		range_widget->hide();
	}
}


void
GPlatesQtWidgets::RemappedColourPaletteWidget::handle_select_palette_filename_button_clicked()
{
	Q_EMIT select_palette_filename_button_clicked();
}


void
GPlatesQtWidgets::RemappedColourPaletteWidget::handle_use_default_palette_button_clicked()
{
	Q_EMIT use_default_palette_button_clicked();
}


void
GPlatesQtWidgets::RemappedColourPaletteWidget::handle_range_check_box_changed(
		int state)
{
	Q_EMIT range_check_box_changed(state);
}


void
GPlatesQtWidgets::RemappedColourPaletteWidget::handle_min_line_editing_finished()
{
	Q_EMIT min_line_editing_finished(min_line_edit->text().toDouble());
}


void
GPlatesQtWidgets::RemappedColourPaletteWidget::handle_max_line_editing_finished()
{
	Q_EMIT max_line_editing_finished(max_line_edit->text().toDouble());
}


void
GPlatesQtWidgets::RemappedColourPaletteWidget::handle_range_restore_min_max_button_clicked()
{
	Q_EMIT range_restore_min_max_button_clicked();
}


void
GPlatesQtWidgets::RemappedColourPaletteWidget::handle_range_restore_mean_deviation_button_clicked()
{
	Q_EMIT range_restore_mean_deviation_button_clicked();
}


void
GPlatesQtWidgets::RemappedColourPaletteWidget::handle_range_restore_mean_deviation_spinbox_changed(
		double value)
{
	Q_EMIT range_restore_mean_deviation_spinbox_changed(value);
}
