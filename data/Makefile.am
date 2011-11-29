# Make regdb files from .sql files

.sql.regdb:
	@rm -f $@
	@$(SQLITE3) $@ < $(srcdir)/common.sql
	$(AM_V_GEN)$(SQLITE3) $@ < $<
	
oovardir = @localstatedir@/lib/@PACKAGE@

oovar_DATA = \
	sandbox.regdb \
	default_user.regdb \
	system.regdb
	
CLEANFILES = $(oovar_DATA)
	
EXTRA_DIST = \
	common.sql \
	sandbox.sql \
	default_user.sql \
	system.sql

dist_pkgdata_DATA = \
	ooserver.conf 
