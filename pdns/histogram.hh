/*
 * This file is part of PowerDNS or dnsdist.
 * Copyright -- PowerDNS.COM B.V. and its contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * In addition, for the avoidance of any doubt, permission is granted to
 * link this program with OpenSSL and to (re)distribute the binaries
 * produced as the result of such linking.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#pragma once

#include <algorithm>
#include <atomic>
#include <string>
#include <vector>

namespace pdns {

// By convention, we are using microsecond units
struct Bucket
{
  std::string d_name;
  uint64_t d_boundary{0};
  uint64_t d_count{0};
};

struct AtomicBucket
{
  // We need the constructors in this case, since atomics have a disabled copy constructor.
  AtomicBucket() {}
  AtomicBucket(std::string name, uint64_t boundary, uint64_t val) : d_name(std::move(name)), d_boundary(boundary), d_count(val) {}
  AtomicBucket(const AtomicBucket& rhs) : d_name(rhs.d_name), d_boundary(rhs.d_boundary), d_count(rhs.d_count.load()) {}

  std::string d_name;
  uint64_t d_boundary{0};
  std::atomic<uint64_t> d_count{0};
};

template<class B>
class BaseHistogram
{
public:
  BaseHistogram(const std::string& prefix, const std::vector<uint64_t>& boundaries)
  {
    if (!std::is_sorted(boundaries.cbegin(), boundaries.cend())) {
      throw std::invalid_argument("boundary array must be sorted");
    }
    if (boundaries.size() == 0) {
      throw std::invalid_argument("boundary array must not be empty");
    }
    if (boundaries[0] == 0) {
      throw std::invalid_argument("boundary array's first element should not be zero");
    }
    d_buckets.reserve(boundaries.size() + 1);
    uint64_t prev = 0;
    for (auto b: boundaries) {
      if (prev == b) {
        throw std::invalid_argument("boundary array's elements should be distinct");
      }
      std::string str = prefix + "le-" + std::to_string(b);
      d_buckets.emplace_back(B{str, b, 0});
      prev = b;
    }
    // everything above last boundary
    d_buckets.emplace_back(B{prefix + "le-max", std::numeric_limits<uint64_t>::max(), 0});
  }

  const std::vector<B>& getRawData() const
  {
    return d_buckets;
  }

  uint64_t getCount(size_t i) const
  {
    return d_buckets[i].d_count;
  }

  std::vector<B> getCumulativeBuckets() const
  {
    std::vector<B> ret;
    ret.reserve(d_buckets.size());
    uint64_t c{0};
    for (const auto& b : d_buckets) {
      c += b.d_count;
      ret.emplace_back(B{b.d_name, b.d_boundary, c});
    }
    return ret;
  }

  std::vector<uint64_t> getCumulativeCounts() const
  {
    std::vector<uint64_t> ret;
    ret.reserve(d_buckets.size());
    uint64_t c = 0;
    for (const auto& b : d_buckets) {
      c += b.d_count;
      ret.emplace_back(c);
    }
    return ret;
  }

  static bool lessOrEqual(uint64_t b, const B& bu)
  {
    return b <= bu.d_boundary;
  }

  inline void operator()(uint64_t d)
  {
    auto index = std::upper_bound(d_buckets.begin(), d_buckets.end(), d, lessOrEqual);
    // out index is always valid
    ++index->d_count;
  }

private:
  std::vector<B> d_buckets;
};

template<class T>
using Histogram = BaseHistogram<Bucket>;

template<class T>
using AtomicHistogram = BaseHistogram<AtomicBucket>;

} // namespace pdns
