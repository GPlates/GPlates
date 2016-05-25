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
 
#ifndef GPLATES_APP_LOGIC_LAYERTASKPARAMS_H
#define GPLATES_APP_LOGIC_LAYERTASKPARAMS_H

#include <QObject>


namespace GPlatesAppLogic
{
	/**
	 * LayerTaskParams can be used to store additional parameters and
	 * configuration options needed by a layer task to do its job.
	 *
	 * If a layer task does not need additional parameters, it may simply store
	 * an instance of LayerTaskParams. If a layer task wishes to store
	 * additional parameters, it can, instead, store an instance of a subclass
	 * of LayerTaskParams.
	 */
	class LayerTaskParams :
			public QObject
	{
		Q_OBJECT

	public:

		virtual
		~LayerTaskParams()
		{  }

	Q_SIGNALS:

		/**
		 * Emitted when any aspect of the parameters has been modified.
		 */
		void
		modified(
				GPlatesAppLogic::LayerTaskParams &layer_task_params);

	protected:

		/**
		 * Subclasses should call this method to cause the @a modified() signal to be emitted.
		 */
		void
		emit_modified()
		{
			Q_EMIT modified(*this);
		}
	};
}

#endif // GPLATES_APP_LOGIC_LAYERTASKPARAMS_H
