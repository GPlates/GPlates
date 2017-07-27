/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
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

#include <cmath>
#include <utility>
#include <boost/optional.hpp>
#include <boost/numeric/conversion/converter.hpp>
#include <boost/foreach.hpp>
#include <QPainter>
#include <QBrush>
#include <QResizeEvent>
#include <QFontMetrics>
#include <QLocale>
#include <QPalette>
#include <QMenu>
#include <QContextMenuEvent>
#include <QMessageBox>
#include <QDebug>

#include "ColourScaleWidget.h"
#include "ViewportWindow.h"
#include "VisualLayersDialog.h"

#include "QtWidgetUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"
#include "gui/Dialogs.h"


namespace
{
	GPlatesQtWidgets::SaveFileDialog::filter_list_type
	get_file_dialog_filters()
	{
		using namespace GPlatesQtWidgets;

		SaveFileDialog::filter_list_type filters;
		filters.push_back(FileDialogFilter(ColourScaleWidget::tr("PNG image"), "png"));
		filters.push_back(FileDialogFilter(ColourScaleWidget::tr("All files"), "*"));
		return filters;
	}
}


GPlatesQtWidgets::ColourScaleWidget::ColourScaleWidget(
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	QWidget(parent_),
	d_viewport_window(viewport_window),
	d_curr_colour_palette(GPlatesGui::RasterColourPalette::create()),
	d_save_file_dialog(
			this,
			tr("Save Image As"),
			get_file_dialog_filters(),
			view_state)
{
	setMinimumHeight(MINIMUM_HEIGHT);

	QAction *save_action = new QAction(tr("&Save Image As..."), this);
	d_right_click_actions.push_back(save_action);
}


bool
GPlatesQtWidgets::ColourScaleWidget::populate(
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette,
		boost::optional<double> use_log_scale)
{
	d_curr_colour_palette = colour_palette;
	d_use_log_scale = use_log_scale;

	return regenerate_contents();
}


void
GPlatesQtWidgets::ColourScaleWidget::paintEvent(
		QPaintEvent *ev)
{
	QPainter painter(this);
	QFontMetrics font_metrics(font());
	int vertical_margin = font_metrics.ascent() / 2;

	// Paint the background.
	QPalette this_palette = palette();
	painter.fillRect(
			0,
			0,
			width(),
			height(),
			QBrush(this_palette.color(QPalette::Window)));

	// Draw the colour scale.
	painter.drawPixmap(
			LEFT_MARGIN,
			vertical_margin,
			isEnabled() ? d_colour_scale_pixmap : d_disabled_colour_scale_pixmap);

	// Draw a border around the colour scale.
	QPen border_pen(Qt::gray);
	border_pen.setWidth(1);
	painter.setPen(border_pen);
	painter.drawRect(
			LEFT_MARGIN - 1,
			vertical_margin - 1,
			d_colour_scale_pixmap.width() + 1,
			d_colour_scale_pixmap.height() + 1);

	// Draw the annotations.
	QPen annotation_pen(this_palette.color(isEnabled() ? QPalette::Active : QPalette::Disabled, QPalette::WindowText));
	annotation_pen.setWidth(1);
	BOOST_FOREACH(const GPlatesGui::ColourScale::annotation_type &annotation, d_annotations)
	{
		int curr_y = annotation.first + vertical_margin;
		painter.setPen(border_pen);
		painter.drawLine(
				LEFT_MARGIN + COLOUR_SCALE_WIDTH,
				curr_y,
				LEFT_MARGIN + COLOUR_SCALE_WIDTH + TICK_LENGTH,
				curr_y);
		painter.setPen(annotation_pen);
		painter.drawText(
				LEFT_MARGIN + COLOUR_SCALE_WIDTH + INTERNAL_SPACING,
				curr_y + font_metrics.ascent() / 2 /* vertically centre text on tick mark */,
				annotation.second);
	}
}


void
GPlatesQtWidgets::ColourScaleWidget::resizeEvent(
		QResizeEvent *ev)
{
	// The colour scale only needs to be regenerated if the height changes - the
	// width of the widget doesn't affect how it looks.
	if (ev->oldSize().height() != height())
	{
		regenerate_contents();
	}

	QWidget::resizeEvent(ev);
}


void
GPlatesQtWidgets::ColourScaleWidget::contextMenuEvent(
		QContextMenuEvent *ev)
{
	QAction *triggered_action = QMenu::exec(d_right_click_actions, ev->globalPos());
	if (triggered_action == d_right_click_actions.first())
	{
		boost::optional<QString> file_name = d_save_file_dialog.get_file_name();
		if (file_name)
		{
			// Grab an image of this widget and save it to disk.
			QPixmap widget_pixmap = QPixmap::grabWidget(this, geometry());
			bool success = widget_pixmap.save(*file_name);
			if (!success)
			{
				QMessageBox::critical(
						&d_viewport_window->dialogs().visual_layers_dialog(),
						tr("Save Image As"),
						tr("GPlates could not save to the chosen file. Please choose another location."));
			}
		}

		ev->accept();
	}
}


bool
GPlatesQtWidgets::ColourScaleWidget::regenerate_contents()
{
	QFontMetrics font_metrics(font());
	int vertical_margin = font_metrics.ascent() / 2;
	int font_height = font_metrics.height();
	int annotation_height = font_height + ANNOTATION_LINE_SPACING;

	int pixmap_width = COLOUR_SCALE_WIDTH;
	int pixmap_height = size().height() - 2 * vertical_margin;

	if (!GPlatesGui::ColourScale::generate(
			d_curr_colour_palette,
			d_colour_scale_pixmap,
			d_disabled_colour_scale_pixmap,
			pixmap_width,
			pixmap_height,
			d_use_log_scale,
			GPlatesGui::ColourScale::Annotations(d_annotations, annotation_height)))
	{
		return false;
	}

	update();

	return true;
}
