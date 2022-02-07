/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_TRANSCRIBEQUALIFIEDXMLNAME_H
#define GPLATES_MODEL_TRANSCRIBEQUALIFIEDXMLNAME_H

#include "QualifiedXmlName.h"

#include "scribe/Scribe.h"


namespace GPlatesModel
{
	//
	// The transcribe implementation of QualifiedXmlName is placed in a separate header
	// that only needs to be included when transcribing.
	// This avoids "QualifiedXmlName.h" having to include the heavyweight "Scribe.h"
	// for regular code (non-transcribe) paths that don't need it.
	//

	template <typename SingletonType>
	GPlatesScribe::TranscribeResult
	QualifiedXmlName<SingletonType>::transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject< QualifiedXmlName<SingletonType> > &qualified_xml_name)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, qualified_xml_name->d_namespace->qstring(), "namespace");
			scribe.save(TRANSCRIBE_SOURCE, qualified_xml_name->d_namespace_alias->qstring(), "namespace_alias");
			scribe.save(TRANSCRIBE_SOURCE, qualified_xml_name->d_name->qstring(), "name");
		}
		else // loading...
		{
			QString namespace_uri, namespace_alias, name;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, namespace_uri, "namespace") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, namespace_alias, "namespace_alias") ||
				!scribe.transcribe(TRANSCRIBE_SOURCE, name, "name"))
			{
				return scribe.get_transcribe_result();
			}

			qualified_xml_name.construct_object(namespace_uri, namespace_alias, name);
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}


	template <typename SingletonType>
	GPlatesScribe::TranscribeResult
	QualifiedXmlName<SingletonType>::transcribe(
			GPlatesScribe::Scribe &scribe,
			bool transcribed_construct_data)
	{
		// If not already transcribed in 'transcribe_construct_data()'.
		if (!transcribed_construct_data)
		{
			if (scribe.is_saving())
			{
				scribe.save(TRANSCRIBE_SOURCE, d_namespace->qstring(), "namespace");
				scribe.save(TRANSCRIBE_SOURCE, d_namespace_alias->qstring(), "namespace_alias");
				scribe.save(TRANSCRIBE_SOURCE, d_name->qstring(), "name");
			}
			else // loading...
			{
				QString namespace_uri, namespace_alias, name;
				if (!scribe.transcribe(TRANSCRIBE_SOURCE, namespace_uri, "namespace") ||
					!scribe.transcribe(TRANSCRIBE_SOURCE, namespace_alias, "namespace_alias") ||
					!scribe.transcribe(TRANSCRIBE_SOURCE, name, "name"))
				{
					return scribe.get_transcribe_result();
				}

				d_namespace = StringSetSingletons::xml_namespace_instance().insert(
							GPlatesUtils::make_icu_string_from_qstring(namespace_uri));
				d_namespace_alias = StringSetSingletons::xml_namespace_alias_instance().insert(
							GPlatesUtils::make_icu_string_from_qstring(namespace_alias));
				d_name = SingletonType::instance().insert(
							GPlatesUtils::make_icu_string_from_qstring(name));
			}
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}
}

#endif // GPLATES_MODEL_TRANSCRIBEQUALIFIEDXMLNAME_H
