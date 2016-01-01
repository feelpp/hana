/*
@copyright Louis Dionne 2013-2016
Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE.md or copy at http://boost.org/LICENSE_1_0.txt)
 */

#include <boost/hana/core/common.hpp>
namespace hana = boost::hana;


static_assert(hana::has_common<int, float>{}, "");
static_assert(!hana::has_common<void, float>{}, "");

int main() { }
