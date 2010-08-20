BEGIN TRANSACTION;
	
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		1,
		'System',
		'The system configuration key',
		0,7
	);
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		2,
		'Sandbox',
		'A key for the sandbox user',
		0,6
	);
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		3,
		'Objects',
		'',
		2,6
	);
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		4,
		'All Users',
		'A key shared between all users',
		0,6
	);
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		5,
		'Objects',
		'',
		4,6
	);
	INSERT INTO RegistryKeys ( Name,Description,Parent,Access) VALUES 
	(
		'OIDs',
		'',
		5,6
	);
	INSERT INTO RegistryKeys ( Name,Description,Parent,Access) VALUES 
	(
		'Applications',
		'Applications store their configuration beneath this key',
		,6
	);
	
COMMIT;