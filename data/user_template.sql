BEGIN TRANSACTION;

INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 1, 'Objects', 0 );
    INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 1, 6 );

	  INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 2, 'OIDs', 1 );
	      INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 2, 6 );

	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 3, 'Applications', 0 );
	      INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 3, 6 ); 

COMMIT;

ANALYZE;
