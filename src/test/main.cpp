/*
  Copyright © 2014 Jesse 'Jeaye' Wilkerson
  See licensing at:
    http://opensource.org/licenses/BSD-3-Clause

  File: test/main.cpp
  Author: Jesse 'Jeaye' Wilkerson
*/

#include <iostream>
#include <string>

#include <jeayeson/jeayeson.hpp>

int main()
{
  /* To start with, create a map and load a file. */
  json_map map{ json_file{ "src/test/json/main.json" } };

  /* We can look at some specify top-level values with "get".
     Notice that "get" returns a reference to the object. */
  std::string &str(map.get<std::string>("str")); /* Get "str" as a mutable string reference. */
  std::cout << "str = " << str << std::endl;
  auto &arr(map.get<json_array>("arr"));

  /* A fallback value can also be specified with "get". It does two things:
       1. Helps deduce the type so that an explicit invocation is not needed
       2. Provides a default fallback value, should anything go wrong while accessing
     Note that these functions do NOT return references, due to incompatibilities with the fallback. */
  std::string const str_copy{ map.get("str", "Default awesomeness") }; // Second param is the default

  /* Delving into maps using dot-notated paths works, too.
     The type can be explicitly specified, or implicit based on the provided fallback.
     They default to json_value, which offers op==, op<<, et cetera. */
  std::cout << map.get_for_path("person.name") << " has " // No fallback, returns json_value&
            << map.get_for_path("person.inventory.coins", 0) << " coins\n"; // Fallback is 0

  /* A less verbose way is to just use op[] on the json_values; this is more convenient,
   * but it comes at the cost of less type-safety and more runtime checks. */
  std::cout << map["person"]["inventory"]["coins"] << std::endl;
  std::cout << map["arr"][1] << std::endl;

  /* Iterators work as expected, based on the C++ stdlib. (const and non-const) */
  for(auto const &it : arr)
  { std::cout << it.as<json_float>() << " "; }
  std::cout << std::endl;
}
