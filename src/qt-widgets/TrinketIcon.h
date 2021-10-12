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
 
#ifndef GPLATES_QTWIDGETS_TRINKETICON_H
#define GPLATES_QTWIDGETS_TRINKETICON_H

#include <boost/function.hpp>
#include <QLabel>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QMouseEvent>


namespace GPlatesQtWidgets
{
	/**
	 * This widget is a subclass of QLabel specialising in displaying the icons in the
	 * status bar, adding a thin veneer of interactivity to the otherwise static QLabel class.
	 * These icons are in turn managed by GPlatesGui::TrinketArea.
	 *
	 * It might be possible to adapt this class to be useful in the ManageFeatureCollectionsDialog,
	 * to help display reconstruciton-workflows in an interactive way.
	 */
	class TrinketIcon: 
			public QLabel
	{
		Q_OBJECT
		Q_PROPERTY(bool clickable READ clickable WRITE set_clickable)
		
	public:

		/**
		 * Typedef for the callback function object which you can set for the on-click event.
		 * Doing it as a boost::function is a little cleaner in this case, rather than having
		 * dozens of different signal/slot connections everywhere.
		 */
		typedef boost::function<void ()> clicked_callback_function_type;

	
		explicit
		TrinketIcon(
				const QIcon &icon,
				const QString &tooltip,
				QWidget *parent_ = NULL);

		virtual
		~TrinketIcon()
		{  }

		void
		setIcon(
				const QIcon &icon);

		void
		set_clickable(
				bool is_clickable)
		{
			d_clickable = is_clickable;
		}
		
		bool
		clickable()
		{
			return d_clickable;
		}

		clicked_callback_function_type
		clicked_callback_function()
		{
			return d_clicked_callback;
		}

		void
		set_clicked_callback_function(
				clicked_callback_function_type f)
		{
			d_clicked_callback = f;
		}

	signals:

		void
		clicked(
				GPlatesQtWidgets::TrinketIcon *self,
				QMouseEvent *ev);

	protected:

		void
		mousePressEvent(
				QMouseEvent *ev);

		void
		mouseMoveEvent(
				QMouseEvent *ev);

		void
		mouseReleaseEvent(
				QMouseEvent *ev);

	private:
	
		/**
		 * Can the user click on this icon to interact with it?
		 */
		bool d_clickable;

		/**
		 * What do we do when clicked?
		 */
		clicked_callback_function_type d_clicked_callback;


		/**
		 * Pixmap for the icon. QPixmap uses pimpl idiom, it is fine to assign to this member.
		 */
		QPixmap d_pixmap_normal;
		QPixmap d_pixmap_clicking;
		
	};
}


#endif	// GPLATES_QTWIDGETS_TRINKETICON_H
