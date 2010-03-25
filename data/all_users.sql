BEGIN TRANSACTION;
	
	INSERT INTO RegistryKeys ( Name,Description,Parent,Access) VALUES 
	(
		'Applications',
		'Applications store their configuration beneath this key',
		0,6
	);
	INSERT INTO RegistryKeys ( Name,Description,Parent,Access) VALUES 
	(
		'Objects',
		'',
		0,6
	);
	INSERT INTO RegistryKeys ( Name,Description,Parent,Access) VALUES 
	(
		'OIDs',
		'',
		last_insert_rowid(),6
	);

COMMIT;