/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009, 2011 The University of Sydney, Australia
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

#include <QWidget>
#include <QSlider>
#include <QPixmap>
#include <QLabel>


namespace GPlatesGui
{
	class ViewportZoom;
}

namespace GPlatesQtWidgets
{
	/**
	 * Trivial widget with a slider and two icons that responds to and changes
	 * the viewport zoom. This is implemented in code in a separate class because
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
				GPlatesGui::ViewportZoom &vzoom,
				QWidget *parent_ = NULL);

	private Q_SLOTS:
		
		void
		handle_slider_moved(
				int slider_position);

		void
		handle_zoom_changed();

	private:

		class ZoomSlider :
				public QSlider
		{
		public:

			ZoomSlider(
					QWidget *parent_) :
				QSlider(parent_)
			{  }

			/**
			 * Exposes the protected function @a setRepeatAction.
			 */
			void
			set_repeat_action(
					QAbstractSlider::SliderAction action)
			{
				setRepeatAction(action);
			}
		};

		class ZoomIcon :
				public QLabel
		{
		public:

			ZoomIcon(
					const QPixmap &icon,
					ZoomSlider *zoom_slider,
					QAbstractSlider::SliderAction action,
					QWidget *parent_) :
				QLabel(parent_),
				d_zoom_slider(zoom_slider),
				d_action(action)
			{
				setPixmap(icon);
				setCursor(QCursor(Qt::PointingHandCursor));
			}

		protected:

			virtual
			void
			mousePressEvent(
					QMouseEvent *ev)
			{
				d_zoom_slider->triggerAction(d_action);
				d_zoom_slider->set_repeat_action(d_action);
			}

			virtual
			void
			mouseReleaseEvent(
					QMouseEvent *ev)
			{
				d_zoom_slider->set_repeat_action(QAbstractSlider::SliderNoAction);
			}

		private:

			ZoomSlider *d_zoom_slider;
			QAbstractSlider::SliderAction d_action;
		};

		void
		set_up_ui();

		void
		set_up_signals_and_slots();

		/**
		 * This is a pointer to the viewport zoom we are using to control
		 * the current zoom level (and react to zoom events not caused
		 * by us so we can update our slider).
		 */
		GPlatesGui::ViewportZoom *d_viewport_zoom_ptr;

		/**
		 * This is our slider widget that we get events from.
		 */
		ZoomSlider *d_slider_zoom;

		/**
		 * A necessary work-around to using QSlider::setValue() while tracking
		 * is enabled; we don't want the programmatic modification of the slider
		 * to cause zoom level changes, because the slider ticks by zoom level,
		 * which may not be exactly the same as the current zoom percentage.
		 *
		 * The subtle interaction of signals and slots in this fashion was causing
		 * a bug that meant it was (mostly) impossible for the user to set a
		 * specific zoom percentage with the spinbox, because the slider would
		 * react to the change and immediately clamp it's own value to a zoom
		 * level, which would then be propagated back to ViewportZoom and change
		 * the spinbox.
		 *
		 * This kind of thing is not as big a problem for (say) the Animation
		 * slider, as the slider has enough granularity in it's "ticks".
		 */
		bool d_suppress_zoom_change_event;

	};
}

#endif  // GPLATES_QTWIDGETS_ZOOMSLIDERWIDGET_H
