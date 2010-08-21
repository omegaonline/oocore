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
		1,6
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
		'OIDs',
		'',
		3,6
	);
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		5,
		'Applications',
		'Applications store their configuration beneath this key',
		2,6
	);
		
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		6,
		'All Users',
		'A key shared between all users',
		0,6
	);
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		7,
		'Objects',
		'',
		6,6
	);
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		8,
		'OIDs',
		'',
		7,6
	);
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		9,
		'Applications',
		'Applications store their configuration beneath this key',
		6,6
	);
	
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		10,
		'Services',
		'System services run at startup',
		1,7
	);
	
COMMIT;