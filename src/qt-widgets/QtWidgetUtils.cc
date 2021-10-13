/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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
#include <QApplication>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QRect>
#include <QSize>
#include <QColorDialog>
#include <QPainter>
#include <QPen>
#include <QBrush>

#include "QtWidgetUtils.h"


void
GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
		QWidget *widget,
		QWidget *placeholder)
{
	QHBoxLayout *layout = new QHBoxLayout(placeholder);
	layout->addWidget(widget);
	layout->setContentsMargins(0, 0, 0, 0);
}


void
GPlatesQtWidgets::QtWidgetUtils::reposition_to_side_of_parent(
		QDialog *dialog)
{
	QWidget *par = dialog->parentWidget();
	if (par)
	{
		QRect frame_geometry = dialog->frameGeometry();
		int new_x = par->pos().x() + par->frameGeometry().width();
		int new_y = par->pos().y() + (par->frameGeometry().height() - frame_geometry.height()) / 2;

		// Ensure the dialog is not off-screen.
		QDesktopWidget *desktop = QApplication::desktop();
		QRect screen_geometry = desktop->screenGeometry(par);
		if (new_x + frame_geometry.width() > screen_geometry.right())
		{
			new_x = screen_geometry.right() - frame_geometry.width();
		}
		if (new_y + frame_geometry.height() > screen_geometry.bottom())
		{
			new_y = screen_geometry.bottom() - frame_geometry.height();
		}

		dialog->move(new_x, new_y);
	}
}


void
GPlatesQtWidgets::QtWidgetUtils::pop_up_dialog(
		QWidget *dialog)
{
	dialog->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	dialog->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	dialog->raise();
}


void
GPlatesQtWidgets::QtWidgetUtils::resize_based_on_size_hint(
		QDialog *dialog)
{
	QSize size_hint = dialog->sizeHint();
	dialog->resize(
			(std::max)(dialog->width(), size_hint.width()),
			size_hint.height());
}


boost::optional<GPlatesGui::Colour>
GPlatesQtWidgets::QtWidgetUtils::get_colour_with_alpha(
		const GPlatesGui::Colour &initial,
		QWidget *parent)
{
	// Qt 4.5 revamped the QColorDialog, changing the mechanism by which the dialog
	// is to be invoked if you are interested in getting an alpha value.
	// QColorDialog::getRgba() is deprecated; note also that the dialog when
	// invoked using this function resets the alpha value to 255.
#if QT_VERSION >= 0x040500
	QColor new_colour = QColorDialog::getColor(
			initial,
			parent,
			"" /* use default title */,
			QColorDialog::ShowAlphaChannel);
	if (new_colour.isValid())
	{
		return GPlatesGui::Colour(new_colour);
	}
	return boost::none;
#else
	QRgb curr_colour = GPlatesGui::Colour::to_qrgb(initial);
	bool ok;
	QRgb result = QColorDialog::getRgba(curr_colour, &ok, parent);
	if (ok)
	{
		return GPlatesGui::Colour::from_qrgb(result);
	}
	return boost::none;
#endif
}


bool
GPlatesQtWidgets::QtWidgetUtils::is_control_c(
		QKeyEvent *key_event)
{
	return key_event->key() == Qt::Key_C &&
#if defined(Q_WS_MAC)
		key_event->modifiers() == Qt::MetaModifier;
#else
		key_event->modifiers() == Qt::ControlModifier;
#endif
}


QPixmap
GPlatesQtWidgets::QtWidgetUtils::create_transparent_checkerboard(
		int width,
		int height,
		int grid_size)
{
	// First we create a tile with 2 rows and 2 columns of checkerboard.
	QPixmap tile(grid_size * 2, grid_size * 2);
	QPainter tile_painter(&tile);
	tile_painter.setPen(QPen(Qt::NoPen));
	tile_painter.setBrush(QBrush(Qt::lightGray));
	tile_painter.drawRect(0, 0, grid_size, grid_size);
	tile_painter.drawRect(grid_size, grid_size, grid_size, grid_size);
	tile_painter.setBrush(QBrush(Qt::darkGray));
	tile_painter.drawRect(0, grid_size, grid_size, grid_size);
	tile_painter.drawRect(grid_size, 0, grid_size, grid_size);

	// Create the final pixmap by tiling the tile over it.
	QPixmap checkerboard(width, height);
	QPainter checkerboard_painter(&checkerboard);
	checkerboard_painter.drawTiledPixmap(0, 0, width, height, tile);

	return checkerboard;
}

