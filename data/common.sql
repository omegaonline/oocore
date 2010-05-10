BEGIN TRANSACTION;

	CREATE TABLE RegistryKeys
	(
		Id INTEGER PRIMARY KEY AUTOINCREMENT,
		Name TEXT NOT NULL,
		Description TEXT,
		Parent INTEGER NOT NULL,
		Access INTEGER DEFAULT 3,
		UNIQUE(Name,Parent)
	);
	
	CREATE INDEX idx_RegistryKeys ON RegistryKeys(Parent);
	
	ANALYZE RegistryKeys;
	
	CREATE TABLE RegistryValues 
	(
		Name TEXT NOT NULL,
		Description TEXT,
		Parent INTEGER NOT NULL,
		Type INTEGER NOT NULL,
		Value,
		UNIQUE(Name,Parent)
	);
	
	CREATE INDEX idx_RegistryValues ON RegistryValues(Parent);
	
	CREATE INDEX idx_RegistryValues2 ON RegistryValues(Name,Parent,Type);
	
	CREATE TRIGGER trg_RegistryKeys AFTER DELETE ON RegistryKeys
		BEGIN 
			DELETE FROM RegistryValues WHERE Parent = OLD.Id; 
		END;

	ANALYZE RegistryValues;
	
COMMIT;