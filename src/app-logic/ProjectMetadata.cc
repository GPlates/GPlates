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
			first_character == '>')
		{
			return false;
		}

		// A leading '-' is a sequence entry only when a space follows it, or when it stands alone.
		// "-3" is a negative number, and reading it as a scalar is what lets it be reported as a
		// bad value for its own field rather than failing the whole document as an unsupported
		// construct - a negative radius should cost the radius, not the timestamp schedule.
		if (first_character == '-' &&
			(trimmed_scalar.size() == 1 || trimmed_scalar[1] == ' '))
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


	/**
	 * Reads an optional front-matter scalar that has to be a finite positive number.
	 *
	 * Saying nothing is left alone: @a value keeps its absent state and the consumer applies its
	 * own default. That is deliberately a different thing from saying zero, or a negative, or
	 * something that is not a number at all, each of which is a mistake in the document worth
	 * reporting rather than quietly treating as "unset".
	 *
	 * Returns false with @a diagnostic set when the value is present but unusable.
	 */
	bool
	read_optional_positive_scalar(
			const QMap<QString, QString> &scalar_values,
			const QString &path,
			const QString &units,
			boost::optional<double> &value,
			QString &diagnostic)
	{
		const QMap<QString, QString>::const_iterator scalar_iter = scalar_values.find(path);
		if (scalar_iter == scalar_values.constEnd())
		{
			return true;
		}

		bool is_numeric = false;
		const double number = scalar_iter.value().toDouble(&is_numeric);
		if (!is_numeric || !std::isfinite(number) || number <= 0)
		{
			diagnostic = QObject::tr("%1 is '%2'; it must be a finite positive number in %3.")
					.arg(path, scalar_iter.value(), units);
			return false;
		}

		value = number;
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

	//
	// From here on, every field stands or falls on its own.
	//
	// Above this point a failure means the document could not be read at all - a missing
	// delimiter, broken indentation, a construct outside the supported subset - and there is
	// nothing to salvage, because nothing was successfully parsed. Below it, the document parsed
	// and we are looking at individual values. One bad number there is a mistake in one field, and
	// discarding the other four because of it helps nobody: a project whose radius says zero still
	// has a perfectly good timestamp schedule, and the user would rather have it.
	//
	// So a bad value leaves its own field unset - the consumer applies its own default, exactly as
	// if the document had said nothing - and records why. Everything else is still read.
	//
	QStringList diagnostics;

	//
	// Planet radius. Optional: a document that says nothing about its planet is describing Earth,
	// which is the overwhelmingly common case and not worth making anyone write out.
	//
	// Present but unusable is a different matter, and is reported rather than quietly swapped for
	// Earth's, because that is a typo rather than an omission and hiding it helps nobody.
	//
	if (scalar_values.contains("gplates.planet.radius_m"))
	{
		bool radius_is_numeric = false;
		const double radius_metres =
				scalar_values["gplates.planet.radius_m"].toDouble(&radius_is_numeric);
		if (!radius_is_numeric || !std::isfinite(radius_metres) || radius_metres <= 0)
		{
			const QString radius_problem =
					QObject::tr("gplates.planet.radius_m is '%1'; it must be a finite positive number in metres.")
							.arg(scalar_values["gplates.planet.radius_m"]);
			metadata.planet_radius_is_valid = false;
			metadata.planet_radius_diagnostic = radius_problem;
			diagnostics.append(radius_problem);
		}
		else
		{
			metadata.planet_radius_metres = radius_metres;
		}
	}

	//
	// Intended resolution. Entirely optional - a project that says nothing about it leaves
	// consumers to use their own default, which is a different thing from a project that says
	// zero.
	//
	QString scalar_problem;
	if (!read_optional_positive_scalar(
			scalar_values, "gplates.resolution.default_km", QObject::tr("kilometres"),
			metadata.default_resolution_km, scalar_problem))
	{
		diagnostics.append(scalar_problem);
	}

	// Per-feature-type overrides. The document writes bare names ("MidOceanRidge:") because a
	// colon inside a YAML key would need quoting; the "gpml:" prefix is added here so callers can
	// look these up by the qualified name they already hold.
	const QString by_feature_type_prefix("gplates.resolution.by_feature_type.");
	for (QMap<QString, QString>::const_iterator scalar_iter = scalar_values.constBegin();
			scalar_iter != scalar_values.constEnd();
			++scalar_iter)
	{
		if (!scalar_iter.key().startsWith(by_feature_type_prefix))
		{
			continue;
		}

		// One unusable override costs only that feature type. The others, and the project default,
		// are still worth having.
		const QString feature_type_name = scalar_iter.key().mid(by_feature_type_prefix.length());
		if (feature_type_name.isEmpty() || feature_type_name.contains('.'))
		{
			diagnostics.append(
					QObject::tr("gplates.resolution.by_feature_type expects one feature type name per entry,"
						" such as 'MidOceanRidge: 250'."));
			continue;
		}

		bool resolution_is_numeric = false;
		const double resolution_km = scalar_iter.value().toDouble(&resolution_is_numeric);
		if (!resolution_is_numeric || !std::isfinite(resolution_km) || resolution_km <= 0)
		{
			diagnostics.append(
					QObject::tr("The resolution for feature type '%1' is '%2'; it must be a finite positive"
						" number in kilometres.")
							.arg(feature_type_name, scalar_iter.value()));
			continue;
		}

		// Accept a name already carrying its namespace, so a document that writes
		// "'gpml:MidOceanRidge': 250" is not silently ignored.
		const QString qualified_name = feature_type_name.contains(':')
				? feature_type_name
				: QString("gpml:") + feature_type_name;
		metadata.resolution_km_by_feature_type.insert(qualified_name, resolution_km);
	}

	//
	// The remaining intent fields: how big a step the world is evolved by, and how quickly
	// subduction spreads once it exists. All optional, and all independent of one another.
	//
	// The template documents each of these, so a document setting one has every reason to expect
	// it to mean something. Reading them here is what makes that true: unknown keys are otherwise
	// accepted and ignored, and a value nobody reads is indistinguishable from a typo.
	//
	struct OptionalScalar
	{
		const char *path;
		QString units;
		boost::optional<double> *value;
	};
	const QString my_units = QObject::tr("millions of years");
	const OptionalScalar optional_scalars[] = {
		{ "gplates.reconstruction.granularity_my", my_units, &metadata.granularity_my },
		{ "gplates.subduction.initiation_my", my_units, &metadata.subduction_initiation_my },
		{ "gplates.subduction.propagation_km_per_my", QObject::tr("kilometres per million years"),
				&metadata.subduction_propagation_km_per_my },
		{ "gplates.subduction.reversal_my", my_units, &metadata.subduction_reversal_my },
		{ "gplates.subduction.breakoff_my", my_units, &metadata.subduction_breakoff_my }
	};
	for (const OptionalScalar &optional_scalar : optional_scalars)
	{
		if (!read_optional_positive_scalar(
				scalar_values, optional_scalar.path, optional_scalar.units,
				*optional_scalar.value, scalar_problem))
		{
			diagnostics.append(scalar_problem);
		}
	}

	//
	// Required project timestamps. Entirely optional, and validated independently of everything
	// above: a malformed schedule must not disturb an otherwise-good radius or resolution, since
	// they are unrelated concerns that happen to share a document.
	//
	// This one carries its own validity flag as well as joining the shared diagnostics, because
	// ProjectTimestampSchedule needs to tell "no schedule was asked for" apart from "a schedule
	// was asked for and could not be read" - the first is silence, the second is a warning.
	//
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
	if (!diagnostics.isEmpty())
	{
		// Say what is wrong, then say what still works. A user reading a warning about one field
		// has no way of knowing, otherwise, whether the rest of the document survived - and the
		// answer is that it did.
		diagnostics.append(
				QObject::tr("Everything else in the document is still being used."));
	}
	metadata.diagnostic = diagnostics.join(" ");
	return metadata;
}
