/*
 * Copyright (C) 2020 Veloman Yunkan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#ifndef ZIM_DIRENT_LOOKUP_H
#define ZIM_DIRENT_LOOKUP_H

#include "zim_types.h"
#include "debug.h"
#include "narrowdown.h"

#include <algorithm>
#include <map>
#include <mutex>
#include <vector>

namespace zim
{

template<class Impl>
class DirentLookup
{
public: // types
  typedef std::pair<bool, article_index_t> Result;

public: // functions
  DirentLookup(Impl* _impl, article_index_type cacheEntryCount);

  article_index_t getNamespaceRangeBegin(char ns) const;
  article_index_t getNamespaceRangeEnd(char ns) const;

  Result find(char ns, const std::string& url);

private: // functions
  std::string getDirentKey(article_index_type i) const;

private: // types
  typedef std::map<char, article_index_t> NamespaceBoundaryCache;

private: // data
  Impl* impl = nullptr;

  mutable NamespaceBoundaryCache namespaceBoundaryCache;
  mutable std::mutex cacheAccessMutex;

  article_index_type articleCount = 0;
  NarrowDown lookupGrid;
};

template<class Impl>
std::string
DirentLookup<Impl>::getDirentKey(article_index_type i) const
{
  const auto d = impl->getDirent(article_index_t(i));
  return d->getNamespace() + d->getUrl();
}

template<class Impl>
DirentLookup<Impl>::DirentLookup(Impl* _impl, article_index_type cacheEntryCount)
{
  ASSERT(impl == nullptr, ==, true);
  impl = _impl;
  articleCount = article_index_type(impl->getCountArticles());
  if ( articleCount )
  {
    const article_index_type step = std::max(1u, articleCount/cacheEntryCount);
    for ( article_index_type i = 0; i < articleCount-1; i += step )
    {
        lookupGrid.add(getDirentKey(i), i, getDirentKey(i+1));
    }
    lookupGrid.close(getDirentKey(articleCount - 1), articleCount - 1);
  }
}

template<typename IMPL>
article_index_t getNamespaceBeginOffset(IMPL& impl, char ch)
{
  ASSERT(ch, >=, 32);
  ASSERT(ch, <=, 127);

  article_index_type lower = 0;
  article_index_type upper = article_index_type(impl.getCountArticles());
  auto d = impl.getDirent(article_index_t(0));
  while (upper - lower > 1)
  {
    article_index_type m = lower + (upper - lower) / 2;
    auto d = impl.getDirent(article_index_t(m));
    if (d->getNamespace() >= ch)
      upper = m;
    else
      lower = m;
  }

  article_index_t ret = article_index_t(d->getNamespace() < ch ? upper : lower);
  return ret;
}

template<typename IMPL>
article_index_t getNamespaceEndOffset(IMPL& impl, char ch)
{
  ASSERT(ch, >=, 32);
  ASSERT(ch, <, 127);
  return getNamespaceBeginOffset(impl, ch+1);
}



template<class Impl>
article_index_t
DirentLookup<Impl>::getNamespaceRangeBegin(char ch) const
{
  ASSERT(ch, >=, 32);
  ASSERT(ch, <=, 127);

  {
    std::lock_guard<std::mutex> lock(cacheAccessMutex);
    const auto it = namespaceBoundaryCache.find(ch);
    if (it != namespaceBoundaryCache.end())
      return it->second;
  }

  auto ret = getNamespaceBeginOffset(*impl, ch);

  std::lock_guard<std::mutex> lock(cacheAccessMutex);
  namespaceBoundaryCache[ch] = ret;
  return ret;
}

template<class Impl>
article_index_t
DirentLookup<Impl>::getNamespaceRangeEnd(char ns) const
{
  return getNamespaceRangeBegin(ns+1);
}

template<typename Impl>
typename DirentLookup<Impl>::Result
DirentLookup<Impl>::find(char ns, const std::string& url)
{
  const auto r = lookupGrid.getRange(ns + url);
  article_index_type l(r.begin);
  article_index_type u(r.end);

  if (l == u)
    return {false, article_index_t(l)};

  while (true)
  {
    article_index_type p = l + (u - l) / 2;
    const auto d = impl->getDirent(article_index_t(p));

    const int c = ns < d->getNamespace() ? -1
                : ns > d->getNamespace() ? 1
                : url.compare(d->getUrl());

    if (c < 0)
      u = p;
    else if (c > 0)
    {
      if ( l == p )
        return {false, article_index_t(u)};
      l = p;
    }
    else
      return {true, article_index_t(p)};
  }
}

} // namespace zim

#endif // ZIM_DIRENT_LOOKUP_H
