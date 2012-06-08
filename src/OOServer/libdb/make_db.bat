del %2system.regdb
%1\sqlite3.exe %2system.regdb < %3data\common.sql
%1\sqlite3.exe %2system.regdb < %3data\system.sql

del %2user_template.regdb
%1\sqlite3.exe %2user_template.regdb < %3data\common.sql
%1\sqlite3.exe %2user_template.regdb < %3data\user_template.sql
