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
 
#ifndef GPLATES_QTWIDGETS_DOCKWIDGET_H
#define GPLATES_QTWIDGETS_DOCKWIDGET_H

#include <QAction>
#include <QDockWidget>
#include <QPointer>

namespace GPlatesGui
{
	class DockState;
}


namespace GPlatesQtWidgets
{
	class ViewportWindow;

	/**
	 * A wrapper around QDockWidget that adds extra bookkeeping actions that we would otherwise
	 * have to add to each dock we create.
	 */
	class DockWidget:
			public QDockWidget
	{
		Q_OBJECT
		
	public:
		explicit
		DockWidget(
				const QString &title,
				GPlatesGui::DockState &dock_state,
				ViewportWindow &main_window);

	signals:

		void
		location_changed(
				GPlatesQtWidgets::DockWidget &self,
				Qt::DockWidgetArea area,
				bool floating);

	public slots:

		void
		dock_at_top();

		void
		dock_at_bottom();

		void
		dock_at_left();

		void
		dock_at_right();

		void
		tabify_at_top();

		void
		tabify_at_bottom();

		void
		tabify_at_left();

		void
		tabify_at_right();

	private slots:

		void
		handle_floating_change(
				bool floating);

		void
		handle_location_change(
				Qt::DockWidgetArea area);

		/**
		 * Hides 'tabify' menu items as appropriate.
		 */
		void
		hide_menu_items_as_appropriate();

	private:

		/**
		 * Creates the context menu necessary to help users wrangle their docks into shape.
		 */
		void
		set_up_context_menu();


		/**
		 * DockState keeps track of which dock is currently where.
		 */
		QPointer<GPlatesGui::DockState> d_dock_state_ptr;

		/**
		 * Guarded pointer back to our main window, to call dock management methods on.
		 */
		QPointer<ViewportWindow> d_viewport_window_ptr;

		/**
		 * The various context menu actions.
		 */
		QPointer<QAction> d_action_Dock_At_Top;
		QPointer<QAction> d_action_Dock_At_Bottom;
		QPointer<QAction> d_action_Dock_At_Left;
		QPointer<QAction> d_action_Dock_At_Right;
		QPointer<QAction> d_action_Tabify_At_Top;
		QPointer<QAction> d_action_Tabify_At_Bottom;
		QPointer<QAction> d_action_Tabify_At_Left;
		QPointer<QAction> d_action_Tabify_At_Right;

	};
}

#endif  // GPLATES_QTWIDGETS_DOCKWIDGET_H
