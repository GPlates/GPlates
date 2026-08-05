/*
 * Copyright (C) 2026 The GPlates developers
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 */

#include <cmath>
#include <QMap>
#include <QObject>
#include <QRegExp>
#include <QSet>
#include <QStringList>
#include <QVector>

#include "ProjectMetadata.h"


namespace
{
	struct MappingLevel
	{
		MappingLevel(
				int indentation_,
				const QString &path_) :
			indentation(indentation_),
			path(path_)
		{  }

		int indentation;
		QString path;
	};


	QString
	remove_comment(
			const QString &line,
			bool &valid)
	{
		bool in_single_quote = false;
		bool in_double_quote = false;
		bool escaped = false;

		for (int index = 0; index < line.size(); ++index)
		{
			const QChar character = line[index];
			if (in_double_quote && character == '\\' && !escaped)
			{
				escaped = true;
				continue;
			}

			if (!escaped)
			{
				if (character == '\'' && !in_double_quote)
				{
					in_single_quote = !in_single_quote;
				}
				else if (character == '"' && !in_single_quote)
				{
					in_double_quote = !in_double_quote;
				}
				else if (character == '#' && !in_single_quote && !in_double_quote)
				{
					return line.left(index);
				}
			}

			escaped = false;
		}

		valid = !in_single_quote && !in_double_quote;
		return line;
	}


	bool
	decode_scalar(
			const QString &encoded_scalar,
			QString &scalar)
	{
		const QString trimmed_scalar = encoded_scalar.trimmed();
		if (trimmed_scalar.isEmpty())
		{
			return false;
		}

		// Sequences, flow mappings, anchors, tags and block scalars are outside
		// the intentionally narrow subset accepted by this parser.
		const QChar first_character = trimmed_scalar[0];
		if (first_character == '[' || first_character == '{' ||
			first_character == '&' || first_character == '*' ||
			first_character == '!' || first_character == '|' ||
			first_character == '>' || first_character == '-')
		{
			return false;
		}

		if (first_character == '\'' || first_character == '"')
		{
			if (trimmed_scalar.size() < 2 || trimmed_scalar[trimmed_scalar.size() - 1] != first_character)
			{
				return false;
			}

			scalar = trimmed_scalar.mid(1, trimmed_scalar.size() - 2);
			return true;
		}

		scalar = trimmed_scalar;
		return true;
	}


	GPlatesAppLogic::ProjectMetadata
	invalid_metadata(
			const QString &diagnostic)
	{
		GPlatesAppLogic::ProjectMetadata metadata;
		metadata.has_front_matter = true;
		metadata.is_valid = false;
		metadata.planet_radius_is_valid = false;
		metadata.required_timestamps_are_valid = false;
		metadata.planet_radius_diagnostic = diagnostic;
		metadata.required_timestamps_diagnostic = diagnostic;
		metadata.diagnostic = diagnostic;
		return metadata;
	}
}


GPlatesAppLogic::ProjectMetadata::ProjectMetadata() :
	has_front_matter(false),
	is_valid(true),
	planet_radius_is_valid(true),
	required_timestamps_are_valid(true)
{  }



GPlatesAppLogic::ProjectMetadata
GPlatesAppLogic::ProjectMetadataParser::parse(
		const QString &markdown)
{
	ProjectMetadata metadata;

	QString normalised_markdown = markdown;
	if (!normalised_markdown.isEmpty() && normalised_markdown[0] == QChar(0xfeff))
	{
		normalised_markdown.remove(0, 1);
	}
	QStringList lines = normalised_markdown.split('\n', Qt::KeepEmptyParts);
	for (int line_index = 0; line_index < lines.size(); ++line_index)
	{
		if (lines[line_index].endsWith('\r'))
		{
			lines[line_index].chop(1);
		}
	}

	if (lines.isEmpty() || lines[0].trimmed() != "---")
	{
		return metadata;
	}

	metadata.has_front_matter = true;

	int closing_delimiter_line = -1;
	for (int line_index = 1; line_index < lines.size(); ++line_index)
	{
		if (lines[line_index].trimmed() == "---")
		{
			closing_delimiter_line = line_index;
			break;
		}
	}
	if (closing_delimiter_line < 0)
	{
		return invalid_metadata(QObject::tr("Markdown front matter has no closing '---' delimiter."));
	}

	QMap<QString, QString> scalar_values;
	QSet<QString> declared_paths;
	QVector<MappingLevel> mapping_stack;
	const QRegExp key_expression("^[A-Za-z_][A-Za-z0-9_-]*$");

	for (int line_index = 1; line_index < closing_delimiter_line; ++line_index)
	{
		const QString original_line = lines[line_index];
		if (original_line.contains('\t'))
		{
			return invalid_metadata(QObject::tr("Unsupported tab indentation in Markdown front matter at line %1.").arg(line_index + 1));
		}

		bool comment_syntax_valid = true;
		const QString line = remove_comment(original_line, comment_syntax_valid);
		if (!comment_syntax_valid)
		{
			return invalid_metadata(QObject::tr("Unterminated quoted scalar in Markdown front matter at line %1.").arg(line_index + 1));
		}
		if (line.trimmed().isEmpty())
		{
			continue;
		}

		int indentation = 0;
		while (indentation < line.size() && line[indentation] == ' ')
		{
			++indentation;
		}
		if (indentation % 2 != 0)
		{
			return invalid_metadata(QObject::tr("Front-matter indentation must use two-space levels (line %1).").arg(line_index + 1));
		}

		while (!mapping_stack.isEmpty() && indentation <= mapping_stack.last().indentation)
		{
			mapping_stack.removeLast();
		}

		if (indentation > 0)
		{
			if (mapping_stack.isEmpty() || indentation != mapping_stack.last().indentation + 2)
			{
				return invalid_metadata(QObject::tr("Malformed front-matter indentation at line %1.").arg(line_index + 1));
			}
		}
		else if (!mapping_stack.isEmpty())
		{
			mapping_stack.clear();
		}

		const QString mapping = line.mid(indentation);
		const int colon_index = mapping.indexOf(':');
		if (colon_index <= 0)
		{
			return invalid_metadata(QObject::tr("Expected a YAML mapping at front-matter line %1.").arg(line_index + 1));
		}

		const QString key = mapping.left(colon_index).trimmed();
		if (!key_expression.exactMatch(key))
		{
			return invalid_metadata(QObject::tr("Unsupported YAML key at front-matter line %1.").arg(line_index + 1));
		}

		const QString parent_path = mapping_stack.isEmpty() ? QString() : mapping_stack.last().path;
		const QString path = parent_path.isEmpty() ? key : parent_path + "." + key;
		if (declared_paths.contains(path))
		{
			return invalid_metadata(QObject::tr("Duplicate front-matter key '%1'.").arg(path));
		}
		declared_paths.insert(path);

		const QString encoded_value = mapping.mid(colon_index + 1).trimmed();
		if (encoded_value.isEmpty())
		{
			mapping_stack.append(MappingLevel(indentation, path));
			continue;
		}

		QString scalar;
		if (!decode_scalar(encoded_value, scalar))
		{
			return invalid_metadata(QObject::tr("Unsupported YAML construct at front-matter line %1.").arg(line_index + 1));
		}
		scalar_values.insert(path, scalar);
	}

	if (scalar_values.contains("gplates.schema_version"))
	{
		bool schema_version_is_integer = false;
		const int schema_version = scalar_values["gplates.schema_version"].toInt(&schema_version_is_integer);
		if (!schema_version_is_integer || schema_version != 1)
		{
			return invalid_metadata(QObject::tr("Unsupported gplates.schema_version; this version supports schema version 1."));
		}
	}

	if (!scalar_values.contains("gplates.planet.radius_m"))
	{
		return invalid_metadata(QObject::tr("The Primary Project Document does not define gplates.planet.radius_m."));
	}

	bool radius_is_numeric = false;
	const double radius_metres = scalar_values["gplates.planet.radius_m"].toDouble(&radius_is_numeric);
	if (!radius_is_numeric || !std::isfinite(radius_metres) || radius_metres <= 0)
	{
		return invalid_metadata(QObject::tr("gplates.planet.radius_m must be a finite positive number in metres."));
	}

	metadata.planet_radius_metres = radius_metres;
	metadata.planet_radius_is_valid = true;

	//
	// Required project timestamps. Entirely optional, and validated independently of the radius
	// above: a malformed schedule here must not disturb an otherwise-good radius, since the two
	// are unrelated concerns that happen to share a document. A missing radius already returned
	// above, so nothing below this point can retroactively invalidate it.
	//
	QStringList diagnostics;
	if (scalar_values.contains("gplates.reconstruction.required_timestamps_ma"))
	{
		const QStringList encoded_timestamps =
				scalar_values["gplates.reconstruction.required_timestamps_ma"].split(
						',', Qt::KeepEmptyParts);
		std::vector<double> timestamps;
		timestamps.reserve(static_cast<std::size_t>(encoded_timestamps.size()));
		QString timestamp_problem;
		if (encoded_timestamps.size() < 2)
		{
			timestamp_problem = QObject::tr(
					"gplates.reconstruction.required_timestamps_ma must contain at least two comma-separated ages.");
		}
		else
		{
			double previous_timestamp = 10001.0;
			for (int index = 0; index < encoded_timestamps.size(); ++index)
			{
				bool timestamp_is_numeric = false;
				const QString encoded_timestamp = encoded_timestamps[index].trimmed();
				const double timestamp = encoded_timestamp.toDouble(&timestamp_is_numeric);
				if (encoded_timestamp.isEmpty() || !timestamp_is_numeric ||
						!std::isfinite(timestamp) || timestamp < 0.0 || timestamp > 10000.0)
				{
					timestamp_problem = QObject::tr(
							"Project timestamp %1 must be a finite age from 0 through 10000 Ma.")
							.arg(index + 1);
					break;
				}
				if (timestamp >= previous_timestamp)
				{
					timestamp_problem = QObject::tr(
							"Project timestamps must be strictly descending from older to younger ages; timestamp %1 is out of order or duplicated.")
							.arg(index + 1);
					break;
				}
				timestamps.push_back(timestamp);
				previous_timestamp = timestamp;
			}
		}

		if (timestamp_problem.isEmpty())
		{
			metadata.required_timestamps_ma = timestamps;
		}
		else
		{
			metadata.required_timestamps_are_valid = false;
			metadata.required_timestamps_diagnostic = timestamp_problem;
			diagnostics.append(timestamp_problem);
		}
	}

	metadata.is_valid = diagnostics.isEmpty();
	metadata.diagnostic = diagnostics.join(" ");
	return metadata;
}
