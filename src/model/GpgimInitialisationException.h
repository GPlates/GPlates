/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_GPGIMINITIALISATIONEXCEPTION_H
#define GPLATES_MODEL_GPGIMINITIALISATIONEXCEPTION_H

#include <QtGlobal>
#include <QString>

#include "global/GPlatesException.h"


namespace GPlatesModel
{
	/**
	 * An exception during initialisation of the GPGIM (reading/parsing GPGIM XML file).
	 */
	class GpgimInitialisationException :
			public GPlatesGlobal::Exception
	{
	public:

		/**
		 * @param msg is a description of the conditions
		 * in which the problem occurs.
		 */
		GpgimInitialisationException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const QString &gpgim_filename,
				const qint64 &line_number,
				const QString &msg) :
			GPlatesGlobal::Exception(exception_source),
			d_gpgim_filename(gpgim_filename),
			d_line_number(line_number),
			d_msg(msg)
		{  }

		~GpgimInitialisationException() throw() { }

	protected:

		virtual const char *
		exception_name() const
		{

			return "GpgimInitialisationException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const;

	private:

		QString d_gpgim_filename;
		int d_line_number;
		QString d_msg;

	};
}

#endif // GPLATES_MODEL_GPGIMINITIALISATIONEXCEPTION_H
