/**
 * Copyright (C) 2022 The University of Sydney, Australia
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

#ifndef GPLATES_SYSTEMFIXES_QTYPEINFO_H
#define GPLATES_SYSTEMFIXES_QTYPEINFO_H

#include <boost/optional.hpp>
#include <QtGlobal>
#include <QTypeInfo>

//
// Qt6 generates errors when compiling source code produced by the Meta-Object Compiler (MOC) where
// boost::optional<T> (for some wrapped type T) is used in a signal/slot.
// The error is that 'operator<' cannot be found for the wrapped type 'T'.
//
// The problem arises with boost::optional<T> because boost::optional<> always has 'operator<'
// but the wrapped type 'T' doesn't necessarily.
//
// Qt6 improved its meta type system using C++17 features (see <QtCore/qtypeinfo.h>) and
// has code (similar to ours below) for std::optional (introduced in C++17) that bypasses
// detecting 'operator<' on std::optional (because it's always there) and instead detecting
// 'operator<' on the wrapped type 'T'.
//
// Although I'm not sure why the following code (see <QtCore/qtypeinfo.h>) doesn't result in
// 'has_operator_less_than<boost::optional<T>>' inheriting from 'std::false_type' due to
// 'decltype(bool(std::declval<const T&>() < std::declval<const T&>()))' failing because
// 'boost::optional<T> < boost::optional<T>' failing due to 'T < T' failing...
//
//     template <typename, typename = void>
//     struct has_operator_less_than : std::false_type{};
//     template <typename T>
//     struct has_operator_less_than<T, std::void_t<decltype(bool(std::declval<const T&>() < std::declval<const T&>()))>>
//             : std::true_type{};
//
// In any case it appears we need to explicitly bypass testing for 'boost::optional<T> < boost::optional<T>' and
// test 'T < T' directly.
//
// The same applies to 'operator==' (but usually that's usually available for most wrapped types anyway).
//
// This was tested against Qt 6.2.4.
//
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	namespace QTypeTraits
	{
		// Bypass detecting 'operator==' on 'boost::optional<T>' and instead detect it on 'T'.
		template <typename T>
		struct has_operator_equal<boost::optional<T>> : has_operator_equal<T> {};

		// Bypass detecting 'operator<' on 'boost::optional<T>' and instead detect it on 'T'.
		template <typename T>
		struct has_operator_less_than<boost::optional<T>> : has_operator_less_than<T> {};
	}
#endif

#endif // GPLATES_SYSTEMFIXES_QTYPEINFO_H
