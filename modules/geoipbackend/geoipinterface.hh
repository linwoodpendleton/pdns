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
#include "boost/optional.hpp"

#include "geoipbackend.hh"

class GeoIPInterface
{
public:
  enum GeoIPQueryAttribute
  {
    ASn,
    City,
    Continent,
    Country,
    Country2,
    Name,
    Region,
    Location,
    // Extensions for split-by-province/ISP/connection-type routing.
    // Keep these last so existing serialized values in zone files remain valid.
    Domain,
    ISP,
    ASO,
    ORG,
    ASN2,
    ConnectionType,
  };

  virtual bool queryCountry(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryCountryV6(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryCountry2(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryCountry2V6(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryContinent(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryContinentV6(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryName(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryNameV6(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryASnum(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryASnumV6(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryRegion(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryRegionV6(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryCity(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryCityV6(string& ret, GeoIPNetmask& gl, const string& ip) = 0;
  virtual bool queryLocation(GeoIPNetmask& gl, const string& ip,
                             double& latitude, double& longitude,
                             std::optional<int>& alt, std::optional<int>& prec)
    = 0;
  virtual bool queryLocationV6(GeoIPNetmask& gl, const string& ip,
                               double& latitude, double& longitude,
                               std::optional<int>& alt, std::optional<int>& prec)
    = 0;

  // Optional extension queries backed by extra MaxMind DBs (Domain, ISP,
  // Connection-Type). Backends without these DBs simply return false.
  virtual bool queryDomain(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }
  virtual bool queryDomainV6(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }
  virtual bool queryISP(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }
  virtual bool queryISPV6(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }
  virtual bool queryASO(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }
  virtual bool queryASOV6(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }
  virtual bool queryORG(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }
  virtual bool queryORGV6(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }
  virtual bool queryASN2(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }
  virtual bool queryASN2V6(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }
  virtual bool queryConnectionType(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }
  virtual bool queryConnectionTypeV6(string& /*ret*/, GeoIPNetmask& /*gl*/, const string& /*ip*/) { return false; }

  virtual ~GeoIPInterface() = default;

  // dbStr is the primary (City/Country) database path. The remaining string
  // arguments are optional paths to auxiliary MaxMind databases (Domain, ISP,
  // dedicated Country, Connection-Type). Empty string means "not configured".
  static unique_ptr<GeoIPInterface> makeInterface(Logr::log_t slog,
                                                  const string& dbStr,
                                                  const string& dbDomainStr = "",
                                                  const string& dbISPStr = "",
                                                  const string& dbCountryStr = "",
                                                  const string& dbConnectionStr = "");

private:
  static unique_ptr<GeoIPInterface> makeMMDBInterface(Logr::log_t slog,
                                                     const string& fname,
                                                     const string& fnameDomain,
                                                     const string& fnameISP,
                                                     const string& fnameCountry,
                                                     const string& fnameConnection,
                                                     const map<string, string>& opts);
  static unique_ptr<GeoIPInterface> makeDATInterface(const string& fname, const map<string, string>& opts);
};
