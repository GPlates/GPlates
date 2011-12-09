/* $Id$ */

/**
 * \file A Widget class to accompany GPlatesGui::ConfigValueDelegate.
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

#ifndef GPLATES_QTWIDGETS_CONFIGVALUEEDITORWIDGET_H
#define GPLATES_QTWIDGETS_CONFIGVALUEEDITORWIDGET_H

#include <QWidget>


namespace GPlatesQtWidgets
{
	class ConfigValueEditorWidget :
			public QWidget
	{
		Q_OBJECT
		
	public:
		explicit
		ConfigValueEditorWidget(
				QWidget *parent_ = NULL);
		
		virtual
		~ConfigValueEditorWidget()
		{  }
	
		
		/**
		 * Has the user clicked the reset button on this editor?
		 */
		bool
		wants_reset()
		{
			return d_wants_reset;
		}
	
	signals:
		
		/**
		 * This widget wants to reset to the default value and close the editor, please.
		 * @a editor is set to 'this', to support a connection to ConfigValueDelegate::closeEditor().
		 *
		 * Note that as far as doing the actual reset is concerned, ConfigValueDelegate does it when
		 * the Qt Model View system asks for a setModelData().
		 */
		void
		reset_requested(
				QWidget *editor);
	
	private slots:
	
		/**
		 * Reset button has been clicked(), re-emit as our own custom signal.
		 */
		void
		handle_reset();
	
	private:
	
		bool d_wants_reset;
		
	};
}



#endif //GPLATES_QTWIDGETS_CONFIGVALUEEDITORWIDGET_H
