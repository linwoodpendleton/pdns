AM_CPPFLAGS += $(YAML_CFLAGS) $(GEOIP_CFLAGS) $(MMDB_CFLAGS) $(LIBCRYPTO_INCLUDES)

EXTRA_DIST = OBJECTFILES OBJECTLIBS

pkglib_LTLIBRARIES = libgeoipbackend.la

libgeoipbackend_la_SOURCES = \
	geoipbackend.cc geoipbackend.hh \
	geoipinterface-dat.cc \
	geoipinterface-mmdb.cc \
	geoipinterface.cc geoipinterface.hh

libgeoipbackend_la_LDFLAGS = -module -avoid-version $(LIBCRYPTO_LDFLAGS) $(LIBSSL_LDFLAGS)
libgeoipbackend_la_LIBADD = $(YAML_LIBS) $(GEOIP_LIBS) $(MMDB_LIBS) $(MYSQL_LIBS) $(CXXFS_LIBS)
