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

unique_ptr<GeoIPInterface> GeoIPInterface::makeInterface(const string& dbStr,const string& db_domain_Str,const string& db_isp_Str)
{
  /* parse dbStr */
  map<string, string> opts;
  vector<string> parts1, parts2;
  vector<string> parts3, parts4;
  vector<string> parts5, parts6;
  string driver;
  string filename;
  string filename_domain;
  string filename_isp;
  stringtok(parts1, dbStr, ":");

  if (parts1.size() == 1) {
    stringtok(parts2, parts1[0], ";");
    /* try extension */
    filename = parts2[0];
    size_t pos = filename.find_last_of(".");
    if (pos != string::npos)
      driver = filename.substr(pos + 1);
    else
      driver = "unknown";
  }
  else {
    driver = parts1[0];
    stringtok(parts2, parts1[1], ";");
    filename = parts2[0];
  }

  stringtok(parts3, db_domain_Str, ":");

  if (parts3.size() == 1) {
    stringtok(parts4, parts3[0], ";");
    /* try extension */
    filename_domain = parts4[0];
  }


  stringtok(parts5, db_isp_Str, ":");

  if (parts5.size() == 1) {
    stringtok(parts6, parts5[0], ";");
    /* try extension */
    filename_isp = parts6[0];
  }

  if (parts2.size() > 1) {
    parts2.erase(parts2.begin(), parts2.begin() + 1);
    for (const auto& opt : parts2) {
      vector<string> kv;
      stringtok(kv, opt, "=");
      opts[kv[0]] = kv[1];
    }
  }

  if (parts4.size() > 1) {
    parts4.erase(parts4.begin(), parts4.begin() + 1);
    for (const auto& opt : parts4) {
      vector<string> kv;
      stringtok(kv, opt, "=");
      opts[kv[0]] = kv[1];
    }
  }

  if (parts6.size() > 1) {
    parts6.erase(parts4.begin(), parts6.begin() + 1);
    for (const auto& opt : parts6) {
      vector<string> kv;
      stringtok(kv, opt, "=");
      opts[kv[0]] = kv[1];
    }
  }

  if (driver == "dat") {
    return makeDATInterface(filename,filename_domain,filename_isp, opts);
  }
  else if (driver == "mmdb") {
    return makeMMDBInterface(filename,filename_domain,filename_isp, opts);
  }
  else {
    throw PDNSException(string("Unsupported file type '") + driver + string("' (use type: prefix to force type)"));
  }
}
