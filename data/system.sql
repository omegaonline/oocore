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
		'Omega.Activation.RunningObjectTable',
		'',
		7,6
	);
	INSERT INTO RegistryValues ( Name,Description,Parent,Value) VALUES 
	(
		'OID',
		'Omega::Activation::OID_RunningObjectTable',
		8,
		'{F67F5A41-BA32-48C9-BFD2-7B3701984DC8}'
	);
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		9,
		'Omega.Registry',
		'',
		7,6
	);
	INSERT INTO RegistryValues ( Name,Description,Parent,Value) VALUES 
	(
		'OID',
		'Omega::Registry::OID_Registry',
		9,
		'{EAAC4365-9B65-4C3C-94C2-CC8CC3E64D74}'
	);
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		10,
		'OIDs',
		'',
		7,6
	);
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		11,
		'Applications',
		'Applications store their configuration beneath this key',
		6,6
	);
	
	INSERT INTO RegistryKeys ( Id,Name,Description,Parent,Access) VALUES 
	(
		12,
		'Services',
		'System services run at startup',
		1,7
	);
	
COMMIT;