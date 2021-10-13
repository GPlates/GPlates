#include <iostream>
#include <sstream>
#include <ctime>
#include <QtCore/QUuid>
#include <QtNetwork/QHostInfo>
#include <unicode/unistr.h>
#if _MSC_VER == 1600
#	undef INT8_MIN
#	undef INT16_MIN
#	undef INT32_MIN
#	undef INT8_MAX
#	undef INT16_MAX
#	undef INT32_MAX
#	undef UINT8_MAX
#	undef UINT16_MAX
#	undef UINT32_MAX
#	undef INT64_C
#	undef UINT64_C
#	include <cstdint>
#endif // _MSC_VER == 1600
#include <vector>
#include <unicode/schriter.h>
#if _MSC_VER == 1600
#	undef INT8_MIN
#	undef INT16_MIN
#	undef INT32_MIN
#	undef INT8_MAX
#	undef INT16_MAX
#	undef INT32_MAX
#	undef UINT8_MAX
#	undef UINT16_MAX
#	undef UINT32_MAX
#	undef INT64_C
#	undef UINT64_C
#	include <cstdint>
#endif // _MSC_VER == 1600
#include <QString>
#include <algorithm>
#include <set>
#ifdef __WINDOWS__
#include <boost/intrusive_ptr.hpp>
#endif // __WINDOWS__
#include <iomanip>
#include <cmath>
#ifdef __WINDOWS__
#include <boost/current_function.hpp>
#endif // __WINDOWS__
#include <ostream>
#include <string>
#include <QDebug>
#ifdef __WINDOWS__
#include <boost/none.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/optional.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/config.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/assert.hpp>
#endif // __WINDOWS__
#ifdef __WINDOWS__
#include <boost/detail/workaround.hpp>
#endif // __WINDOWS__
