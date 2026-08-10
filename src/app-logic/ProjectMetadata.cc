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
		metadata.diagnostic = diagnostic;
		return metadata;
	}
}


GPlatesAppLogic::ProjectMetadata::ProjectMetadata() :
	has_front_matter(false),
	is_valid(true)
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

	metadata.is_valid = true;
	metadata.planet_radius_metres = radius_metres;
	return metadata;
}
