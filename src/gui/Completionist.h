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

#ifndef GPLATES_GUI_COMPLETIONIST_H
#define GPLATES_GUI_COMPLETIONIST_H

#include <QLineEdit>
#include <QAbstractItemModel>
#include <QMap>
#include <QSharedPointer>

#include "utils/Singleton.h"


namespace GPlatesGui
{
	
	/**
	 * GUI class to load and hold assorted lists of completion terms for tab completion or
	 * find-as-you-type functionality on QLineEdits and to generate appropriate Qt Models and
	 * QCompleter objects behind the scenes so that all you really need to do is tell this class
	 * to attach find-as-you-type functionality to a QLineEdit and what dictionary of terms to use.
	 *
	 * TODO: Time is limited, so this is pretty basic at the moment. You may want to do some intelligent
	 * store down at the model or fileio layer for the actual XML DOM timescale data, if planned
	 * functionality is to include the ability for GPlates to map between them, etc. For now, this
	 * exists purely as a typing aid. --jclark 20150316
	 */
	class Completionist :
			public GPlatesUtils::Singleton<Completionist>
	{
		/**
		 * It should not be possible for client code to create an instance of this class.
		 * This declares a protected constructor and defines it with an empty body.
		 *
		 * You can access the instance via ::instance().
		 */
		GPLATES_SINGLETON_CONSTRUCTOR_DEF(Completionist)
		
	public:
		~Completionist()
		{  }

		/**
		 * Creates a QCompleter object suitable for completion with the specified dictionary
		 * of terms, and installs it on the given QLineEdit using ->setCompleter().
		 *
		 * this implies doing ->setWidget() on the QCompleter object - only one completer can be
		 * set on a widget, and one completer can only handle one widget. Accordingly, the QCompleter
		 * object created by this function is parented to the given widget so that it will be
		 * cleaned up appropriately.
		 *
		 * The Qt Models used by the completer objects behind the scenes should be shareable
		 * between instances without any problems; the Completionist class will hold on to those.
		 *
		 * TODO: Currently has no way to specify which dictionary you're interested in; always
		 * installs the ICC2012.xml built-in resource.
		 */
		void
		install_completer(
				QLineEdit &widget);

	private:

		/**
		 * Instantiate or fetch a previously instantiated QAbstractItemModel for use by QCompleter.
		 *
		 * May return NULL if no such source of completion text exists.
		 *
		 * TODO: Rather than specify @a name as a path to a resource, possibly have GPlates load
		 * a bunch of those resources at a lower level and refer to them here by name.
		 */
		QAbstractItemModel *
		get_model_for_dictionary(
				const QString &name);

		/**
		 * Holds the constructed Qt models corresponding to our dictionaries for use and re-use
		 * by QCompleter objects.
		 */
		QMap<QString, QSharedPointer<QAbstractItemModel> > d_models;
	};
}

#endif // GPLATES_GUI_COMPLETIONIST_H
