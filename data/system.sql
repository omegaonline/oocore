BEGIN TRANSACTION;
	
	INSERT INTO RegistryKeys ( Name,Description,Parent,Access) VALUES 
	(
		'All Users',
		'A common key for all users',
		0,6
	);
	INSERT INTO RegistryKeys ( Name,Description,Parent,Access) VALUES 
	(
		'Local User',
		'A key unique to each user of the local computer',
		0,4
	);
	INSERT INTO RegistryKeys ( Name,Description,Parent,Access) VALUES 
	(
		'System',
		'The system configuration key',
		0,7
	);

COMMIT;