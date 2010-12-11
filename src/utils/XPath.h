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

#ifndef GPLATES_UTILS_XPATH_H
#define GPLATES_UTILS_XPATH_H

#include <boost/optional.hpp>
#include <QString>


namespace GPlatesUtils
{
	/**
	 * This namespace contains a parser that builds an expression tree from an
	 * XPath expression and facilities to evaluate that parsed expression tree.
	 *
	 * This XPath parser currently only supports a subset of the full XPath
	 * grammar (http://www.w3.org/TR/xpath20/#nt-bnf). It is sufficient to
	 * encode certain predicates, and is sufficient to meet the requirements of the
	 * OpenGIS Filter Encoding Implementation Specification v 1.1
	 * (http://www.opengeospatial.org/standards/filter), section 6.1.2.
	 */
	namespace XPath
	{
		/**
		 * A tokenizer to assist with parsing an XPath expression.
		 */
		class Tokenizer
		{
		public:

			enum Token
			{
				INVALID,

				VARIABLE, // e.g. gpml:foobar
				INTEGER_LITERAL,
				DOUBLE_LITERAL, // Note: we do not distinguish between doubles and decimals
				STRING_LITERAL, // e.g. "Fred"

				GENERAL_EQUALS, // =
				GENERAL_NOT_EQUALS, // !=
				GENERAL_LESS_THAN, // <
				GENERAL_LESS_THAN_OR_EQUAL, // <=
				GENERAL_GREATER_THAN, // >
				GENERAL_GREATER_THAN_OR_EQUAL, // >=

				OPENING_PARENTHESIS, // (
				CLOSING_PARENTHESIS, // )
				OPENING_BRACKETS, // [
				CLOSING_BRACKETS, // ]
				AT, // @
				SLASH, // /
				PLUS, // +
				MINUS, // -

				AND, // and
				OR, // or
				
				END // Signifies that there are no more tokens
			};

			/** Thrown by this class to indicate a failed tokenization */
			struct Exception {  };

			/**
			 * Constructs a Tokenizer that will tokenize @a str.
			 * After construction, @c next() must be called before reading the first token.
			 */
			explicit
			Tokenizer(
					const QString &str);

			/**
			 * Advances the tokenizer to the next token.
			 * May throw @c Exception if the next token is malformed in any way.
			 */
			void
			next();

			/**
			 * Returns the current token as an enumerated value.
			 * Returns END if the tokenizer has reached the end of the string.
			 */
			Token
			curr_token() const
			{
				return d_curr_token;
			}

			/**
			 * Returns the current variable.
			 * This method should only be called if @c curr_token() returns VARIABLE.
			 */
			const QString &
			curr_variable() const
			{
				return *d_curr_variable;
			}

			/**
			 * Returns the current integer literal.
			 * This method should only be called if @c curr_token() returns INTEGER_LITERAL.
			 */
			int
			curr_integer_literal() const
			{
				return *d_curr_integer_literal;
			}

			/**
			 * Returns the current double literal.
			 * This method should only be called if @c curr_token() returns DOUBLE_LITERAL.
			 */
			double
			curr_double_literal() const
			{
				return *d_curr_double_literal;
			}

			/**
			 * Returns the current string literal.
			 * This method should only be called if @c curr_token() returns STRING_LITERAL.
			 */
			const QString &
			curr_string_literal() const
			{
				return *d_curr_string_literal;
			}

			/**
			 * Returns a string version of the given @a token; useful for debugging.
			 */
			static
			const QString &
			get_token_as_string(
					Token token);

		private:

			void
			parse_variable(
					const QString &str);

			void
			parse_numeric_literal(
					const QString &str);

			void
			parse_operator(
					const QString &str);

			void
			throw_exception();

			QString d_str;
			int d_pos;

			Token d_curr_token;
			boost::optional<QString> d_curr_variable;
			boost::optional<int> d_curr_integer_literal;
			boost::optional<double> d_curr_double_literal;
			boost::optional<QString> d_curr_string_literal;
		};
	}
}

#endif  // GPLATES_UTILS_XPATHEXPRESSION_H
