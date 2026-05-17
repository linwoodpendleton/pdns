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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "geoipbackend.hh"
#include "geoipinterface.hh"

#ifdef HAVE_MMDB

#include "maxminddb.h"
#include "pdns/logging.hh"

class GeoIPInterfaceMMDB : public GeoIPInterface
{
public:
  GeoIPInterfaceMMDB(Logr::log_t slog,
                     const string& fname,
                     const string& fnameDomain,
                     const string& fnameISP,
                     const string& fnameCountry,
                     const string& fnameConnection,
                     const string& modeStr,
                     const string& language) :
    d_slog(slog)
  {
    int flags = 0;
    if (modeStr == "")
      /* for the benefit of ifdef */
      ;
#ifdef HAVE_MMAP
    else if (modeStr == "mmap")
      flags |= MMDB_MODE_MMAP;
#endif
    else
      throw PDNSException(string("Unsupported mode ") + modeStr + ("for geoipbackend-mmdb"));

    openDb(d_s, fname, flags, "primary");
    openOptional(d_domain, d_hasDomain, fnameDomain, flags, "domain");
    openOptional(d_isp, d_hasIsp, fnameISP, flags, "isp");
    openOptional(d_country, d_hasCountry, fnameCountry, flags, "country");
    openOptional(d_conn, d_hasConn, fnameConnection, flags, "connection-type");
    d_lang = language;
  }

  bool queryCountry(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    auto& db = d_hasCountry ? d_country : d_s;
    if (!mmdbLookupIn(db, ip, false, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, "country", "iso_code", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    ret = string(data.utf8_string, data.data_size);
    return true;
  };

  bool queryCountryV6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    auto& db = d_hasCountry ? d_country : d_s;
    if (!mmdbLookupIn(db, ip, true, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, "country", "iso_code", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    ret = string(data.utf8_string, data.data_size);
    return true;
  };

  bool queryCountry2(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    return queryCountry(ret, gl, ip);
  }

  bool queryCountry2V6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    return queryCountryV6(ret, gl, ip);
  }

  bool queryContinent(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    auto& db = d_hasCountry ? d_country : d_s;
    if (!mmdbLookupIn(db, ip, false, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, "continent", "code", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    ret = string(data.utf8_string, data.data_size);
    return true;
  }

  bool queryContinentV6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    auto& db = d_hasCountry ? d_country : d_s;
    if (!mmdbLookupIn(db, ip, true, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, "continent", "code", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    ret = string(data.utf8_string, data.data_size);
    return true;
  }

  bool queryName(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    if (!mmdbLookup(ip, false, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, "autonomous_system_organization", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    ret = string(data.utf8_string, data.data_size);
    return true;
  }

  bool queryNameV6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    if (!mmdbLookup(ip, true, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, "autonomous_system_organization", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    ret = string(data.utf8_string, data.data_size);
    return true;
  }

  bool queryASnum(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    // Prefer the dedicated ISP db (it carries autonomous_system_number);
    // fall back to the primary db.
    if (d_hasIsp && lookupNumericField(d_isp, ip, false, gl, ret, "autonomous_system_number"))
      return true;
    return lookupNumericField(d_s, ip, false, gl, ret, "autonomous_system_number");
  }

  bool queryASnumV6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    if (d_hasIsp && lookupNumericField(d_isp, ip, true, gl, ret, "autonomous_system_number"))
      return true;
    return lookupNumericField(d_s, ip, true, gl, ret, "autonomous_system_number");
  }

  bool queryRegion(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    if (!mmdbLookup(ip, false, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, "subdivisions", "0", "iso_code", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    ret = string(data.utf8_string, data.data_size);
    return true;
  }

  bool queryRegionV6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    if (!mmdbLookup(ip, true, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, "subdivisions", "0", "iso_code", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    ret = string(data.utf8_string, data.data_size);
    return true;
  }

  bool queryCity(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    if (!mmdbLookup(ip, false, gl, res))
      return false;
    if ((MMDB_get_value(&res.entry, &data, "cities", "0", NULL) != MMDB_SUCCESS || !data.has_data) && (MMDB_get_value(&res.entry, &data, "city", "names", d_lang.c_str(), NULL) != MMDB_SUCCESS || !data.has_data))
      return false;
    ret = string(data.utf8_string, data.data_size);
    return true;
  }

  bool queryCityV6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    if (!mmdbLookup(ip, true, gl, res))
      return false;
    if ((MMDB_get_value(&res.entry, &data, "cities", "0", NULL) != MMDB_SUCCESS || !data.has_data) && (MMDB_get_value(&res.entry, &data, "city", "names", d_lang.c_str(), NULL) != MMDB_SUCCESS || !data.has_data))
      return false;
    ret = string(data.utf8_string, data.data_size);
    return true;
  }

  bool queryLocation(GeoIPNetmask& gl, const string& ip,
                     double& latitude, double& longitude,
                     std::optional<int>& /* alt */, std::optional<int>& prec) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    if (!mmdbLookup(ip, false, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, "location", "latitude", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    latitude = data.double_value;
    if (MMDB_get_value(&res.entry, &data, "location", "longitude", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    longitude = data.double_value;
    if (MMDB_get_value(&res.entry, &data, "location", "accuracy_radius", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    prec = data.uint16;
    return true;
  }

  bool queryLocationV6(GeoIPNetmask& gl, const string& ip,
                       double& latitude, double& longitude,
                       std::optional<int>& /* alt */, std::optional<int>& prec) override
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    if (!mmdbLookup(ip, true, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, "location", "latitude", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    latitude = data.double_value;
    if (MMDB_get_value(&res.entry, &data, "location", "longitude", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    longitude = data.double_value;
    if (MMDB_get_value(&res.entry, &data, "location", "accuracy_radius", NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    prec = data.uint16;
    return true;
  }

  bool queryDomain(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    if (!d_hasDomain) return false;
    return lookupStringField(d_domain, ip, false, gl, ret, "domain");
  }
  bool queryDomainV6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    if (!d_hasDomain) return false;
    return lookupStringField(d_domain, ip, true, gl, ret, "domain");
  }

  bool queryISP(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    if (!d_hasIsp) return false;
    return lookupStringField(d_isp, ip, false, gl, ret, "isp");
  }
  bool queryISPV6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    if (!d_hasIsp) return false;
    return lookupStringField(d_isp, ip, true, gl, ret, "isp");
  }

  bool queryASO(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    if (d_hasIsp && lookupStringField(d_isp, ip, false, gl, ret, "autonomous_system_organization"))
      return true;
    return lookupStringField(d_s, ip, false, gl, ret, "autonomous_system_organization");
  }
  bool queryASOV6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    if (d_hasIsp && lookupStringField(d_isp, ip, true, gl, ret, "autonomous_system_organization"))
      return true;
    return lookupStringField(d_s, ip, true, gl, ret, "autonomous_system_organization");
  }

  bool queryORG(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    if (!d_hasIsp) return false;
    return lookupStringField(d_isp, ip, false, gl, ret, "organization");
  }
  bool queryORGV6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    if (!d_hasIsp) return false;
    return lookupStringField(d_isp, ip, true, gl, ret, "organization");
  }

  bool queryASN2(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    return queryASnum(ret, gl, ip);
  }
  bool queryASN2V6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    return queryASnumV6(ret, gl, ip);
  }

  bool queryConnectionType(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    if (!d_hasConn) return false;
    return lookupStringField(d_conn, ip, false, gl, ret, "connection_type");
  }
  bool queryConnectionTypeV6(string& ret, GeoIPNetmask& gl, const string& ip) override
  {
    if (!d_hasConn) return false;
    return lookupStringField(d_conn, ip, true, gl, ret, "connection_type");
  }

  ~GeoIPInterfaceMMDB() override
  {
    MMDB_close(&d_s);
    if (d_hasDomain) MMDB_close(&d_domain);
    if (d_hasIsp) MMDB_close(&d_isp);
    if (d_hasCountry) MMDB_close(&d_country);
    if (d_hasConn) MMDB_close(&d_conn);
  };

private:
  MMDB_s d_s{};
  MMDB_s d_domain{};
  bool d_hasDomain{false};
  MMDB_s d_isp{};
  bool d_hasIsp{false};
  MMDB_s d_country{};
  bool d_hasCountry{false};
  MMDB_s d_conn{};
  bool d_hasConn{false};
  string d_lang;
  Logr::log_t d_slog;

  void openDb(MMDB_s& target, const string& fname, int flags, const char* tag)
  {
    memset(&target, 0, sizeof(target));
    int ec = MMDB_open(fname.c_str(), flags, &target);
    if (ec != MMDB_SUCCESS)
      throw PDNSException(string("Cannot open ") + fname + " (" + tag + "): " + string(MMDB_strerror(ec)));
    SLOG(g_log << Logger::Debug << "Opened MMDB database " << fname << " (" << tag << " type: " << target.metadata.database_type << ")" << endl,
         d_slog->info(Logr::Debug, "Opened MMDB database",
                      "file", Logging::Loggable(fname),
                      "role", Logging::Loggable(string(tag)),
                      "type", Logging::Loggable(target.metadata.database_type)));
  }

  void openOptional(MMDB_s& target, bool& hasFlag, const string& fname, int flags, const char* tag)
  {
    if (fname.empty()) {
      hasFlag = false;
      return;
    }
    openDb(target, fname, flags, tag);
    hasFlag = true;
  }

  bool mmdbLookup(const string& ip, bool v6, GeoIPNetmask& gl, MMDB_lookup_result_s& res)
  {
    return mmdbLookupIn(d_s, ip, v6, gl, res);
  }

  bool mmdbLookupIn(MMDB_s& db, const string& ip, bool v6, GeoIPNetmask& gl, MMDB_lookup_result_s& res)
  {
    int gai_ec = 0, mmdb_ec = 0;
    res = MMDB_lookup_string(&db, ip.c_str(), &gai_ec, &mmdb_ec);

    if (gai_ec != 0) {
      SLOG(g_log << Logger::Warning << "MMDB_lookup_string(" << ip << ") failed: " << gai_strerror(gai_ec) << endl,
           d_slog->error(Logr::Warning, gai_strerror(gai_ec), "MMDB lookup failed", "ip", Logging::Loggable(ip)));
    }
    else if (mmdb_ec != MMDB_SUCCESS) {
      SLOG(g_log << Logger::Warning << "MMDB_lookup_string(" << ip << ") failed: " << MMDB_strerror(mmdb_ec) << endl,
           d_slog->error(Logr::Warning, MMDB_strerror(mmdb_ec), "MMDB lookup failed", "ip", Logging::Loggable(ip)));
    }
    else if (res.found_entry) {
      gl.netmask = res.netmask;
      /* If it's a IPv6 database, IPv4 netmasks are reduced from 128, so we need to deduct
         96 to get from [96,128] => [0,32] range */
      if (!v6 && gl.netmask > 32)
        gl.netmask -= 96;
      return true;
    }
    return false;
  }

  bool lookupStringField(MMDB_s& db, const string& ip, bool v6, GeoIPNetmask& gl, string& ret, const char* field)
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    if (!mmdbLookupIn(db, ip, v6, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, field, NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    ret = string(data.utf8_string, data.data_size);
    return true;
  }

  bool lookupNumericField(MMDB_s& db, const string& ip, bool v6, GeoIPNetmask& gl, string& ret, const char* field)
  {
    MMDB_entry_data_s data;
    MMDB_lookup_result_s res;
    if (!mmdbLookupIn(db, ip, v6, gl, res))
      return false;
    if (MMDB_get_value(&res.entry, &data, field, NULL) != MMDB_SUCCESS || !data.has_data)
      return false;
    ret = std::to_string(data.uint32);
    return true;
  }
};

unique_ptr<GeoIPInterface> GeoIPInterface::makeMMDBInterface(Logr::log_t slog,
                                                             const string& fname,
                                                             const string& fnameDomain,
                                                             const string& fnameISP,
                                                             const string& fnameCountry,
                                                             const string& fnameConnection,
                                                             const map<string, string>& opts)
{
  string mode = "";
  string language = "en";
  const auto& opt_mode = opts.find("mode");
  if (opt_mode != opts.end())
    mode = opt_mode->second;
  const auto& opt_lang = opts.find("language");
  if (opt_lang != opts.end())
    language = opt_lang->second;
  return std::make_unique<GeoIPInterfaceMMDB>(slog, fname, fnameDomain, fnameISP, fnameCountry, fnameConnection, mode, language);
}

#else

unique_ptr<GeoIPInterface> GeoIPInterface::makeMMDBInterface([[maybe_unused]] Logr::log_t slog,
                                                             [[maybe_unused]] const string& fname,
                                                             [[maybe_unused]] const string& fnameDomain,
                                                             [[maybe_unused]] const string& fnameISP,
                                                             [[maybe_unused]] const string& fnameCountry,
                                                             [[maybe_unused]] const string& fnameConnection,
                                                             [[maybe_unused]] const map<string, string>& opts)
{
  throw PDNSException("libmaxminddb support not compiled in");
}

#endif
