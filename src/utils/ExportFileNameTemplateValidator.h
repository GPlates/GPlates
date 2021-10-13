/* $Id$ */

/**
 * \file validate filename template.
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

#ifndef GPLATES_UTILS_EXPORTFILENAMETEMPLATEVALIDATOR_H
#define GPLATES_UTILS_EXPORTFILENAMETEMPLATEVALIDATOR_H

#include <boost/shared_ptr.hpp>
#include <QString>
#include <string>
#include <iostream>
#include <sstream>

#include "ExportTemplateFilenameSequence.h"
#include "ExportAnimationStrategyExporterID.h"

namespace GPlatesUtils
{
	class ExportFileNameTemplateValidator
	{
	public:
		
		static const std::string INVALID_CHARACTERS;

		virtual
		bool
		is_valid(
				const QString&) = 0;

		virtual
		~ExportFileNameTemplateValidator()
		{ }

		class ResultReport
		{
			friend class ExportFileNameTemplateValidator;

		public:
			const QString&
			message() const
			{
				return d_message;
			}

		private:
			QString d_message;
		};

		const 
		ResultReport&
		get_result_report()
		{
			return d_result;
		}
	
	protected:
		/**
		 * return true if filename passed the validation, otherwise false.
		 */
		bool
		file_sequence_validate(
				const QString &filename);

		/**
		 * return true if no invalid characters found, otherwise false.
		 */
		bool
		has_invalid_characters(
				const QString &filename);

		/**
		 * return true if %P found, otherwise false.
		 */
		bool
		has_percent_P(
				const QString &filename);

		inline
		void
		set_result_message(
				const QString &msg)
		{
			d_result.d_message = msg;
		}
		ResultReport d_result;
	};

	class ExportRotationFileNameTemplateValidator :
		public GPlatesUtils::ExportFileNameTemplateValidator
	{
	public:
		virtual
		bool
		is_valid(
				const QString&);
	};

	class ExportResolvedTopologyFileNameTemplateValidator :
		public ExportFileNameTemplateValidator
	{
		virtual
		bool
		is_valid(
				const QString&);
	};

	class ExportVelocityFileNameTemplateValidator:
		public ExportFileNameTemplateValidator
	{
		virtual
		bool
		is_valid(
				const QString&);
	};

	class ExportSvgFileNameTemplateValidator :
		public ExportFileNameTemplateValidator
	{
		virtual
		bool
		is_valid(
				const QString&);
	};

	class ExportReconstructedGeometryFileNameTemplateValidator :
		public ExportFileNameTemplateValidator
	{
		virtual
		bool
		is_valid(
				const QString&);
	};

	class ExportRasterFileNameTemplateValidator :
		public ExportFileNameTemplateValidator
	{
		virtual
		bool
		is_valid(
				const QString&);
	};

	class ExportRotationParamsFileNameTemplateValidator :
		public ExportFileNameTemplateValidator
	{
		virtual
		bool
		is_valid(
				const QString&);
	};

	class ExportFlowlineFileNameTemplateValidator :
		public ExportFileNameTemplateValidator
	{
		virtual
		bool
		is_valid(
				const QString&);
	};

	class DummyExportFileNameTemplateValidator :
		public ExportFileNameTemplateValidator
	{
		virtual
		bool
		is_valid(
				const QString&);
	};
	
	class ExportFileNameTemplateValidatorFactory
	{
	public:	
		static
		boost::shared_ptr<ExportFileNameTemplateValidator>
		create_validator(
				GPlatesUtils::Exporter_ID);
	};
}

#endif // GPLATES_UTILS_EXPORTFILENAMETEMPLATEVALIDATOR_H

