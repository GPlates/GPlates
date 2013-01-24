/* $Id$ */

/**
 * \file Base for main GPlates dialogs, to be managed by gui/Dialogs.h
 *
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
 
#ifndef GPLATES_QTWIDGETS_GPLATESDIALOG_H
#define GPLATES_QTWIDGETS_GPLATESDIALOG_H

#include <QDialog>
#include <QtGlobal>
#include <boost/noncopyable.hpp>


namespace GPlatesQtWidgets
{
	/**
	 * Base class to be used in place of a plain QDialog for @em major GPlates dialogs.
	 *
	 * By inheriting from GPlatesDialog instead of QDialog, a few extra utility methods will
	 * be available to help manage the opening and closing of the dialog in a way that works
	 * around some odd behaviour across different platforms.
	 * Your class should still use the Q_OBJECT macro as usual.
	 * 
	 * For those dialogs that behave a bit more like a "Main sub-window", this class will
	 * also provide a QAction for an easy hide/show menu item.
	 *
	 * GPlatesGui::Dialogs should be used to manage instances of specific GPlatesDialogs, to
	 * avoid further ViewportWindow clutter.
	 *
	 * TODO: We may also want to add a special Cmd-W handler for OSX, as it seems Qt only provides
	 * 'Esc' as a standard means of reject()ing the dialog.
	 */
	class GPlatesDialog:
			public QDialog,
			private boost::noncopyable
	{
		Q_OBJECT
	public:
	
		/**
		 * @param _parent - Parent widget for the dialog, usually the main window. Dialogs should
		 *                  @em always have a parent, otherwise they pop up in the middle of the screen.
		 * @param _flags  - Qt windw flags, usually Qt::Window or Qt::Dialog.
		 *                  See http://doc.trolltech.com/4.6/qt.html#WindowType-enum for more details.
		 */
		explicit
		GPlatesDialog(
				QWidget *_parent,
				Qt::WindowFlags flags = Qt::Window);
	
		virtual
		~GPlatesDialog()
		{  }
	
	public Q_SLOTS:
	
		/**
		 * If the dialog is currently hidden, show it and ask the WM to raise it to the top.
		 * If the dialog is already shown, similarly encourage the WM to bring it to the front.
		 *
		 * TODO: Alternative handler for modal windows.
		 */
		void
		pop_up();
	};

}

#endif // GPLATES_QTWIDGETS_GPLATESDIALOG_H


