/* $Id$ */

/**
 * \file An adapter between the @a GPlatesQtMsgHandler and the LogModel Qt Model.
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

#include <QString>

#include "LogToModelHandler.h"

#include "app-logic/LogModel.h"



GPlatesAppLogic::LogToModelHandler::LogToModelHandler(
		GPlatesAppLogic::LogModel &_model):
	d_log_model_ptr(&_model)
{  }


GPlatesAppLogic::LogToModelHandler::~LogToModelHandler()
{  }


void
GPlatesAppLogic::LogToModelHandler::handle_qt_message(
		QtMsgType msg_type,
		const char *msg)
{
	d_log_model_ptr->append(LogModel::LogEntry(QString(msg), LogModel::LogEntry::Severity(msg_type)));
}

