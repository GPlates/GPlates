/* $Id$ */

/**
 * \file
 *
 * Most recent change:
 *   $Date$
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

#include "XPath.h"

#include "Parse.h"


GPlatesUtils::XPath::Tokenizer::Tokenizer(
		const QString &str) :
	d_str(str),
	d_pos(0),
	d_curr_token(INVALID)
{  }


namespace
{
	enum TokenizerState
	{
		TOKEN_START,
		IN_VARIABLE,
		IN_NUMERIC_LITERAL,
		IN_NUMERIC_LITERAL_IMMEDIATELY_AFTER_E,
		IN_STRING_LITERAL,
		IN_STRING_LITERAL_POTENTIAL_CLOSING_QUOTE_SEEN,
		IN_OPERATOR
	};
}


void
GPlatesUtils::XPath::Tokenizer::next()
{
	QString buffer;
	TokenizerState state = TOKEN_START;

	d_curr_variable = boost::none;
	d_curr_integer_literal = boost::none;
	d_curr_double_literal = boost::none;
	d_curr_string_literal = boost::none;

	while (true)
	{
		QChar c;

		switch (state)
		{
		// When we haven't read any characters yet for this token.
		case TOKEN_START:
			if (d_pos == d_str.length())
			{
				d_curr_token = END;
				return;
			}

			c = d_str.at(d_pos);
			if (c.isLetter() || c == ':' || c == '_')
			{
				state = IN_VARIABLE;
				buffer += c;
			}
			else if (c.isNumber() || c == '.')
			{
				state = IN_NUMERIC_LITERAL;
				buffer += c;
			}
			else if (c == '"')
			{
				state = IN_STRING_LITERAL;
			}
			else if (c == '!' ||
				c == '<' ||
				c == '>')
			{
				state = IN_OPERATOR;
				buffer += c;
			}
			else if (c == '=')
			{
				d_curr_token = GENERAL_EQUALS;
				++d_pos;
				return;
			}
			else if (c == '(')
			{
				d_curr_token = OPENING_PARENTHESIS;
				++d_pos;
				return;
			}
			else if (c == ')')
			{
				d_curr_token = CLOSING_PARENTHESIS;
				++d_pos;
				return;
			}
			else if (c == '[')
			{
				d_curr_token = OPENING_BRACKETS;
				++d_pos;
				return;
			}
			else if (c == ']')
			{
				d_curr_token = CLOSING_BRACKETS;
				++d_pos;
				return;
			}
			else if (c == '@')
			{
				d_curr_token = AT;
				++d_pos;
				return;
			}
			else if (c == '/')
			{
				d_curr_token = SLASH;
				++d_pos;
				return;
			}
			else if (c == '+')
			{
				d_curr_token = PLUS;
				++d_pos;
				return;
			}
			else if (c == '-')
			{
				d_curr_token = MINUS;
				++d_pos;
				return;
			}
			else if (c.isSpace())
			{
				// Do nothing.
			}
			else
			{
				throw_exception();
			}

			break;

		// When the current token starts with a letter.
		case IN_VARIABLE:
			if (d_pos == d_str.length())
			{
				parse_variable(buffer);
				return;
			}

			c = d_str.at(d_pos);
			if (c.isLetter() || c == ':' || c == '_' || c.isNumber()) // Note: XML names can be more flexible.
			{
				buffer += c;
			}
			else
			{
				parse_variable(buffer);
				return;
			}
			break;

		// When the current token starts with a digit.
		case IN_NUMERIC_LITERAL:
			if (d_pos == d_str.length())
			{
				parse_numeric_literal(buffer);
				return;
			}

			c = d_str.at(d_pos);
			if (c.isNumber() || c == '.' || c == 'e' || c == 'E')
			{
				buffer += c;
				if (c == 'e' || c == 'E')
				{
					state = IN_NUMERIC_LITERAL_IMMEDIATELY_AFTER_E;
				}
			}
			else
			{
				parse_numeric_literal(buffer);
				return;
			}
			break;

		// When already in a numeric literal but after we've seen 'E' or 'e'.
		case IN_NUMERIC_LITERAL_IMMEDIATELY_AFTER_E:
			if (d_pos == d_str.length())
			{
				throw_exception();
			}

			c = d_str.at(d_pos);
			if (c.isNumber() || c == '+' || c == '-')
			{
				buffer += c;
				state = IN_NUMERIC_LITERAL;
			}
			else
			{
				throw_exception();
			}
			break;

		// When the current token starts with a quotation mark.
		case IN_STRING_LITERAL:
			if (d_pos == d_str.length())
			{
				throw_exception();
			}

			c = d_str.at(d_pos);
			if (c == '"')
			{
				state = IN_STRING_LITERAL_POTENTIAL_CLOSING_QUOTE_SEEN;
			}
			else
			{
				buffer += c;
			}
			break;

		// When already in string literal and another quotation mark seen; this
		// could represent the end of the string literal or the first quotation
		// mark in an escaped quote (represented by two quotation marks).
		case IN_STRING_LITERAL_POTENTIAL_CLOSING_QUOTE_SEEN:
			if (d_pos == d_str.length())
			{
				d_curr_token = STRING_LITERAL;
				d_curr_string_literal = buffer;
				return;
			}

			c = d_str.at(d_pos);
			if (c == '"')
			{
				buffer += c;
				state = IN_STRING_LITERAL;
			}
			else
			{
				d_curr_token = STRING_LITERAL;
				d_curr_string_literal = buffer;
				return;
			}
			break;

		// When the current token starts with a recognised symbol.
		case IN_OPERATOR:
			if (d_pos == d_str.length())
			{
				parse_operator(buffer);
				++d_pos;
				return;
			}

			c = d_str.at(d_pos);
			if (c == '=')
			{
				buffer += c;
			}

			parse_operator(buffer);
			++d_pos;
			return;
		}

		++d_pos;
	}
}


const QString &
GPlatesUtils::XPath::Tokenizer::get_token_as_string(
		Token token)
{
	static const QString TOKEN_NAMES[] = {
		"INVALID",
		
		"VARIABLE",
		"INTEGER_LITERAL",
		"DOUBLE_LITERAL",
		"STRING_LITERAL",

		"GENERAL_EQUALS",
		"GENERAL_NOT_EQUALS",
		"GENERAL_LESS_THAN",
		"GENERAL_LESS_THAN_OR_EQUAL",
		"GENERAL_GREATER_THAN",
		"GENERAL_GREATER_THAN_OR_EQUAL",

		"OPENING_PARENTHESIS",
		"CLOSING_PARENTHESIS",
		"OPENING_BRACKETS",
		"CLOSING_BRACKETS",
		"AT",
		"SLASH",
		"PLUS",
		"MINUS",

		"AND",
		"OR",

		"END"
	};
	static const QString DEFAULT = "";

	std::size_t index = static_cast<std::size_t>(token);
	if (index < sizeof(TOKEN_NAMES) / sizeof(QString))
	{
		return TOKEN_NAMES[index];
	}
	else
	{
		return DEFAULT;
	}
}


void
GPlatesUtils::XPath::Tokenizer::parse_variable(
		const QString &str)
{
	if (str == "and")
	{
		d_curr_token = AND;
	}
	else if (str == "or")
	{
		d_curr_token = OR;
	}
	else
	{
		d_curr_token = VARIABLE;
		d_curr_variable = str;
	}
}


void
GPlatesUtils::XPath::Tokenizer::parse_numeric_literal(
		const QString &str)
{
	// Attempt to parse as double first.
	Parse<double> parse_double;
	try
	{
		double d = parse_double(str);
		d_curr_token = DOUBLE_LITERAL;
		d_curr_double_literal = d;
	}
	catch (const ParseError &)
	{
		// Then try to parse as integer.
		Parse<int> parse_int;
		try
		{
			int i = parse_int(str);
			d_curr_token = INTEGER_LITERAL;
			d_curr_integer_literal = i;
		}
		catch (const ParseError &)
		{
			throw_exception();
		}
	}
}


void
GPlatesUtils::XPath::Tokenizer::parse_operator(
		const QString &str)
{
	if (str == "!=")
	{
		d_curr_token = GENERAL_NOT_EQUALS;
	}
	else if (str == "<")
	{
		d_curr_token = GENERAL_LESS_THAN;
	}
	else if (str == "<=")
	{
		d_curr_token = GENERAL_LESS_THAN_OR_EQUAL;
	}
	else if (str == ">")
	{
		d_curr_token = GENERAL_GREATER_THAN;
	}
	else if (str == ">=")
	{
		d_curr_token = GENERAL_GREATER_THAN_OR_EQUAL;
	}
	else
	{
		throw_exception();
	}
}


void
GPlatesUtils::XPath::Tokenizer::throw_exception()
{
	d_curr_token = INVALID;
	throw Exception();
}

