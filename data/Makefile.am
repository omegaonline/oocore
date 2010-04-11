# Make regdb files from .sql files

.sql.regdb:
	rm -f $@
	$(SQLITE3) $@ < $(srcdir)/common.sql
	$(SQLITE3) $@ < $<
	
localstate_DATA = \
	all_users.regdb \
	default_user.regdb \
	system.regdb
	
EXTRA_DIST = \
	common.sql \
	all_users.sql \
	default_user.sql \
	system.sql
