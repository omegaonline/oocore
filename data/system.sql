BEGIN TRANSACTION;
	
	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		1,
		'System',
		0
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		1,
		7
	);
	
	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		2,
		'Sandbox',
		1
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		2,
		6
	);

	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		3,
		'Objects',
		2
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		3,
		6
	);

	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		4,
		'OIDs',
		3
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		4,
		6
	);

	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		5,
		'Applications',
		2
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		5,
		6
	);
		
	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		6,
		'All Users',
		0
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		6,
		6
	);

	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		7,
		'Objects',
		6
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		7,
		6
	);

	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		8,
		'Omega.Activation.RunningObjectTable',
		7
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		8,
		6
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'OID',
		8,
		'{F67F5A41-BA32-48C9-BFD2-7B3701984DC8}'
	);

	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		9,
		'Omega.Registry',
		7
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		9,
		6
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'OID',
		9,
		'{EAAC4365-9B65-4C3C-94C2-CC8CC3E64D74}'
	);

	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		10,
		'OIDs',
		7
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		10,
		6
	);

	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		11,
		'Applications',
		6
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		11,
		6
	);
	
	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES 
	(
		12,
		'Services',
		1
	);
	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES 
	(
		'.access',
		12,
		7
	);
	
COMMIT;
