/*
  Copyright © 2014 Jesse 'Jeaye' Wilkerson
  See licensing at:
    http://opensource.org/licenses/BSD-3-Clause

  File: map.hpp
  Author: Jesse 'Jeaye' Wilkerson
*/

#pragma once

#include "traits.hpp"

#include <string>
#include <vector>
#include <utility>
#include <boost/algorithm/string.hpp>

/* Decide underlying map type */
#undef JEAYESON_MAP_T

#ifdef JEAYESON_USE_STD_MAP
  #include <map>
  #define JEAYESON_MAP_T std::map

#elif defined JEAYESON_USE_STD_UNORD
  #include <unordered_map>
  #define JEAYESON_MAP_T std::unordered_map

#elif defined JEAYESON_USE_BOOST_UNORD
  #include <boost/unordered_map.hpp>
  #define JEAYESON_MAP_T boost::unordered_map

#elif defined JEAYESON_USE_OTHER_MAP
  /* Client is responsible for appropriate includes */
  #ifndef JEAYESON_OTHER_MAP
    #error "JEAYESON_USE_OTHER_MAP specified but JEAYSON_OTHER_MAP is undefined"
  #else
    #define JEAYESON_MAP_T JEAYESON_OTHER_MAP
  #endif

#else
  #error "No JEAYESON_USE_[map type] is defined"

#endif

namespace jeayeson
{
  template <typename Value, typename Parser>
  class array;
  std::vector<std::string> tokenize(std::string const &source, std::string const &delim);

  /* Maps provide a wrapper for
   * string-indexed values, which
   * could be any valid JSON object.
   */
  template <typename Value, typename Parser>
  class map
  {
    public:
      using map_t = map<Value, Parser>;
      using array_t = array<Value, Parser>;
      using key_t = std::string;
      using value_type = Value;
      using parser_t = Parser;
      using cstr_t = char const * const;
      using internal_map_t = JEAYESON_MAP_T<key_t, Value>;
      using iterator = typename internal_map_t::iterator;
      using const_iterator = typename internal_map_t::const_iterator;

      static char const delim_open = '{';
      static char const delim_close = '}';

      map(){} /* XXX: User-defined ctor required for variant. */
      explicit map(std::string const &json)
      { reset(json); }
      explicit map(file const &f)
      { reset(f); }
      explicit map(value_type const &val)
      {
        if(val.get_type() == value_type::type_map)
        { *this = val.template as<map_t>(); }
        else
        { throw std::runtime_error("Failed to construct map from non-map"); }
      }
      template <typename T>
      explicit map(std::map<key_t, T> const &container)
      {
        for(auto const &it : container)
        { set(it.first, it.second); }
      }
      map(map const &m) : values_(m.values_)
      { }

      template <typename T = Value>
      auto& get(key_t const &key)
      { return values_[key].template as<T>(); } 
      template <typename T = Value>
      auto const& get(key_t const &key) const
      { return values_[key].template as<T>(); } 

      /* Access with a fallback. */
      template <typename T = Value>
      auto get(key_t const &key, T &&fallback) const
      {
        auto const it(values_.find(key));
        if(it != values_.end())
        { return it->second.template as<T>(); }
        else
        { return static_cast<detail::normalize<T>>(fallback); }
      }
      std::string get(key_t const &key, cstr_t const fallback) const
      { return get<std::string>(key, fallback); }

      Value& operator [](key_t const &key)
      { return get(key); }
      Value const& operator [](key_t const &key) const
      { return get(key); }

      bool empty() const
      { return values_.empty(); }
      size_t size() const
      { return values_.size(); }

      template <typename T = Value>
      T& get_for_path(std::string const &path) const
      {
        std::vector<std::string> const tokens(tokenize(path, "."));
        size_t const path_size(tokens.size() - 1);

        map_t *sub_map(const_cast<map_t*>(this));
        for(size_t i{}; i < path_size; ++i)
        { sub_map = &sub_map->get<map_t>(tokens[i]); }

        return sub_map->get<T>(tokens[path_size]);
      }

      template <typename T = Value>
      T get_for_path(std::string const &path, T &&fallback) const
      {
        std::vector<std::string> const tokens(tokenize(path, "."));
        size_t const path_size(tokens.size() - 1);

        map_t *sub_map(const_cast<map_t*>(this));
        for(size_t i{}; i < path_size; ++i)
        {
          auto const it(sub_map->find(tokens[i]));
          if(it == sub_map->end())
          { return std::forward<T>(fallback); }

          sub_map = &(it->second.template as<map_t>());
        }

        auto const it(sub_map->find(tokens[path_size]));
        if(it == sub_map->end())
        { return std::forward<T>(fallback); }
        return it->second.template as<T>();
      }

      std::vector<key_t> get_keys() const
      {
        std::vector<key_t> keys;
        keys.reserve(values_.size());
        for(auto const &it : values_)
        { keys.push_back(it.first); }
        return keys;
      }

      array_t get_values() const
      {
        array_t arr;
        arr.reserve(values_.size());
        for(auto const &it : values_)
        { arr.add(it.second); }
        return arr;
      }

      /* Searches for an entry with the specified key. */
      iterator find(key_t const &key)
      { return values_.find(key); }
      const_iterator find(key_t const &key) const
      { return values_.find(key); }

      iterator begin()
      { return values_.begin(); }
      const_iterator begin() const
      { return values_.begin(); }
      const_iterator cbegin() const
      { return values_.begin(); }

      iterator end()
      { return values_.end(); }
      const_iterator end() const
      { return values_.end(); }
      const_iterator cend() const
      { return values_.end(); }

      template <typename T>
      void set(key_t const &key, T &&value)
      { values_[key] = std::forward<T>(value); }
      void set(key_t const &key, cstr_t const value)
      { values_[key] = static_cast<std::string>(value); }
      template <typename T>
      void set(key_t const &key, std::map<key_t, T> const &data)
      { set(key, map{ data }); }

      /* Completely wipes out all data in the map. */
      void clear()
      { values_.clear(); }

      /* Completely removes the specified key and destroys its data. */
      void erase(key_t const &key)
      { values_.erase(key); }

      /* Adds the specified map into this map. */
      void merge(map const &m)
      { values_.insert(m.values_.begin(), m.values_.end()); }

      void reset(std::string const &json)
      { *this = Parser::template parse<map_t>(json); }
      void reset(file const &f)
      { *this = Parser::template parse_file<map_t>(f.data); }

      /* Writes the JSON data to string form. */
      std::string to_string() const
      { return Parser::template save<map_t>(*this); }

      template <typename Stream_Value, typename Stream_Parser>
      friend std::ostream& operator <<(std::ostream &stream,
                                       map<Stream_Value, Stream_Parser> const &m);
      template <typename V, typename P>
      friend bool operator ==(map<V, P> const &lhs, map<V, P> const &rhs);
      template <typename V, typename P>
      friend bool operator !=(map<V, P> const &lhs, map<V, P> const &rhs);

    private:
      mutable internal_map_t values_;
  };

  inline std::vector<std::string> tokenize(std::string const &source,
                                           std::string const &delim)
  {
    std::vector<std::string> tokens;
    boost::algorithm::split(tokens, source, boost::is_any_of(delim));
    return tokens;
  }

  template <typename V, typename P>
  bool operator ==(map<V, P> const &lhs, map<V, P> const &rhs)
  { return lhs.values_ == rhs.values_; }
  template <typename V, typename P>
  bool operator !=(map<V, P> const &lhs, map<V, P> const &rhs)
  { return !(lhs == rhs); }
}
