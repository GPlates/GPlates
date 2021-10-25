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

#ifndef GPLATES_GUI_CUSTOMCOMPLETER_H
#define GPLATES_GUI_CUSTOMCOMPLETER_H

#include <QCompleter>


namespace GPlatesGui
{
	/**
	 * We subclass QCompleter to get at the two protected virtual methods, splitPath and pathFromIndex.
	 * And figure out what they actually *do*.
	 * Also to maybe gently encourage it to use EditRole like I told it to, damnit.
	 */
	class CustomCompleter :
			public QCompleter
	{
		Q_OBJECT
	public:
		explicit
		CustomCompleter(
				QObject *parent_ = NULL);
		
		void
		set_custom_popup();
		
	protected:
		/**
		 * Seems to only get called as the user is typing, and then only to split up what they typed, not the model data.
		 */
		virtual
		QStringList
		splitPath(
				const QString &path) const;
		
		/**
		 * Seems to only get called once some entry is selected to generate the final text that gets inserted.
		 */
		virtual
		QString
		pathFromIndex(
				const QModelIndex &idx) const;
	};
	
}

#endif // GPLATES_GUI_CUSTOMCOMPLETER_H
