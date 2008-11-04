/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_QTWIDGETS_ZOOMSLIDERWIDGET_H
#define GPLATES_QTWIDGETS_ZOOMSLIDERWIDGET_H

#ifdef HAVE_PYTHON
// We need to include this _before_ any Qt headers get included because
// of a moc preprocessing problems with a feature called 'slots' in the
// python header file object.h
# include <boost/python.hpp>
#endif

#include <QWidget>
#include <QSlider>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QDebug>

#include "gui/ViewportZoom.h"

namespace GPlatesQtWidgets
{
	/**
	 * Trivial widget with a slider and two icons that responds to and emits
	 * zoom events. This is implemented in code in a separate class because
	 * this slider now needs to be inserted very carefully between two other
	 * widgets which are also set up via code rather than Qt Designer.
	 * 
	 * This is all done so that we can put a resize grip between the GlobeView
	 * and the TaskPanel, and have it (hopefully) resize in a natural way.
	 */
	class ZoomSliderWidget:
			public QWidget
	{
		Q_OBJECT

	public:
		explicit
		ZoomSliderWidget(
				const GPlatesGui::ViewportZoom &vzoom,
				QWidget *parent_ = NULL):
			QWidget(parent_),
			d_slider_zoom(new QSlider(this))
		{
			// Set our own properties.
			setFocusPolicy(Qt::NoFocus);
			setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

			// Set up the widgets, as though the Designer had created them.
			QVBoxLayout *vbox = new QVBoxLayout(this);
			vbox->setContentsMargins(0, 0, 0, 0);
			vbox->setSpacing(2);
			
			QLabel *label_zoom_max = new QLabel(this);
			label_zoom_max->setPixmap(QPixmap(QString::fromUtf8(":/gnome_zoom_in_16.png")));
			vbox->addWidget(label_zoom_max);

			d_slider_zoom->setSingleStep(1);
			d_slider_zoom->setOrientation(Qt::Vertical);
			d_slider_zoom->setTickPosition(QSlider::NoTicks);
			d_slider_zoom->setFocusPolicy(Qt::WheelFocus);
			vbox->addWidget(d_slider_zoom);

			QLabel *label_zoom_min = new QLabel(this);
			label_zoom_min->setPixmap(QPixmap(QString::fromUtf8(":/gnome_zoom_out_16.png")));
			vbox->addWidget(label_zoom_min);

			// Set up the zoom slider to use appropriate range and current zoom level.
			d_slider_zoom->setRange(vzoom.s_min_zoom_level, vzoom.s_max_zoom_level);
			d_slider_zoom->setValue(vzoom.s_initial_zoom_level);

			// This could go straight to the ViewportZoom - but we're not sure if that
			// will change when we have multiple views.
			QObject::connect(d_slider_zoom, SIGNAL(sliderMoved(int)),
					this, SLOT(handle_slider_moved(int)));
		}


		/**
		 * In response to a zoom event, calling this method will set the slider
		 * to the new zoom level (i.e. canvas.viewport_zoom().zoom_level()).
		 *
		 * Note: I'm being cautious, and not making this a 'zoom has changed' signal
		 * listener. It instead relies on the existing slot in ReconstructionViewWidget
		 * to update the ZoomSliderWidget via this setter. My reasoning is,
		 * once the Map Canvas stuff is merged in, we may have two entirely different
		 * ViewportZoom instances/derivations, and so this class should not cache
		 * that ViewportZoom as a member. Let ReconstructionViewWidget deal with
		 * the indirection.
		 */
		void
		set_zoom_value(
				int new_zoom_level)
		{
			d_slider_zoom->setValue(new_zoom_level);
		}
	
	signals:
		
		/**
		 * Emitted when the slider has been changed to a new zoom level.
		 * Does not modify a ViewportZoom for the reasons stated in set_zoom_value().
		 */
		void
		slider_moved(
				int slider_position);

	private slots:
		
		void
		handle_slider_moved(
				int slider_position)
		{
			qDebug() << "slider_moved"<<slider_position;
			emit slider_moved(slider_position);
		}
	
	private:
	
		QSlider *d_slider_zoom;

	};
}

#endif  // GPLATES_QTWIDGETS_ZOOMSLIDERWIDGET_H
