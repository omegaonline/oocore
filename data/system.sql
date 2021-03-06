BEGIN TRANSACTION;
	
	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 1, 'System', 0 );
	    INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 1, 7 );

	  INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 2, 'Sandbox', 1 );
	      INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 2, 6 );

	    INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 3, 'Objects', 2 );
	        INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 3, 6 );

		  INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 4, 'OIDs', 3 );
	          INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 4, 6 );

	    INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 5, 'Applications', 2 );
	        INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 5, 6 );
	        
	  INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 6, 'Services', 1 );
	      INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 6, 7 );
		
	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 7, 'All Users', 0 );
	    INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 7, 6 );
	
	  INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 8, 'Objects', 7 );
	      INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 8, 6 );
	
	    INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 11, 'Omega.Surrogate', 8 );
	        INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 11, 6 );
	        INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( 'OID', 11, '{D063D32C-FB9A-004A-D2E5-BB5451808FF5}' );
	        
	    INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 12, 'Omega.ServiceHost', 8 );
	        INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 12, 6 );
	        INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( 'OID', 12, '{1ACC3273-8FB3-9741-E7E6-1CD4C6150FB2}' );

	    INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 14, 'OIDs', 8 );
	        INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 14, 6 );
	        
	        INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 15, '{D063D32C-FB9A-004A-D2E5-BB5451808FF5}', 14 );
	        	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 15, 6 );
	        	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( 'Application', 15, 'Omega.SystemHost' );
	        
        	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 16, '{1ACC3273-8FB3-9741-E7E6-1CD4C6150FB2}', 14 );
	        	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 16, 6 );
	        	INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( 'Application', 16, 'Omega.SystemHost' );

	  INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 18, 'Applications', 7 );
	      INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 18, 6 ); 
	      
	      INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 19, 'Omega.SystemHost', 18 );
	          INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 19, 6 );
	          
	          INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 20, 'Activation', 19 );          	
		          INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 20, 6 );
		          INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( 'SystemHost', 20, '' );

	INSERT INTO RegistryKeys ( Id,Name,Parent) VALUES ( 21, '.links', 0 );
	    INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( '.access', 21, 7 );
	    INSERT INTO RegistryValues ( Name,Parent,Value) VALUES ( 'Local User', 21, 'system:user/' );
	
COMMIT;

ANALYZE;
