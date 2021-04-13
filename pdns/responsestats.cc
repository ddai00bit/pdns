#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "responsestats.hh"
#include <limits>
#include "namespaces.hh"
#include "logger.hh"

#include "dnsparser.hh"

static auto sizeBounds()
{
  std::vector<uint64_t> bounds;

  bounds.push_back(20);
  bounds.push_back(40);
  bounds.push_back(60);
  bounds.push_back(80);
  bounds.push_back(100);
  bounds.push_back(150);
  for (uint64_t n = 200; n < 65000; n += 200) {
    bounds.push_back(n);
  }
  return bounds;
}

ResponseStats::ResponseStats() :
  d_sizecounters("SizeCounters", sizeBounds())
{
  for (unsigned int n = 0; n < 65535; ++n) {
    d_qtypecounters[n] = 0;
  }
  for (unsigned int n = 0; n < 256; ++n) {
    d_rcodecounters[n] = 0;
  }
}

ResponseStats g_rs;

void ResponseStats::submitResponse(uint16_t qtype, uint16_t respsize, uint8_t rcode, bool udpOrTCP)
{
  d_rcodecounters[rcode]++;
  submitResponse(qtype, respsize, udpOrTCP);
}

void ResponseStats::submitResponse(uint16_t qtype, uint16_t respsize, bool udpOrTCP)
{
  d_qtypecounters[qtype]++;
  d_sizecounters(respsize);
}

map<uint16_t, uint64_t> ResponseStats::getQTypeResponseCounts()
{
  map<uint16_t, uint64_t> ret;
  uint64_t count;
  for (unsigned int i = 0; i < 65535; ++i) {
    count = d_qtypecounters[i];
    if (count) {
      ret[i] = count;
    }
  }
  return ret;
}

map<uint16_t, uint64_t> ResponseStats::getSizeResponseCounts()
{
  map<uint16_t, uint64_t> ret;
  for (const auto& sizecounter : d_sizecounters.getRawData()) {
    if (sizecounter.d_count) {
      ret[sizecounter.d_boundary] = sizecounter.d_count;
    }
  }
  return ret;
}

map<uint8_t, uint64_t> ResponseStats::getRCodeResponseCounts()
{
  map<uint8_t, uint64_t> ret;
  uint64_t count;
  for (unsigned int i = 0; i < 256; ++i) {
    count = d_rcodecounters[i];
    if (count) {
      ret[i] = count;
    }
  }
  return ret;
}

string ResponseStats::getQTypeReport()
{
  auto qtypenums = getQTypeResponseCounts();
  ostringstream os;
  boost::format fmt("%s\t%d\n");
  for (const auto& val : qtypenums) {
    os << (fmt % DNSRecordContent::NumberToType(val.first) % val.second).str();
  }
  return os.str();
}
