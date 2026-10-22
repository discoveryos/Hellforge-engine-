#include "../hflib/hfprecompiled.h"
#pragma hdrstop

#include "hfCommon_local.h"

hfCVar com_product_lang_ext( "com_product_lang_ext", "1", CVAR_INTEGER | CVAR_SYSTEM | CVAR_ARCHIVE, "Extension to use when creating language files." );

/*
=================
LoadMapLocalizeData
=================
*/
typedef hfHashTable<hfStrList> ListHash;
void LoadMapLocalizeData(ListHash& listHash) {

	hfStr fileName = "map_localize.cfg";
	const char *buffer = NULL;
	hfLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	if ( fileSystem->ReadFile( fileName, (void**)&buffer ) > 0 ) {
		src.LoadMemory( buffer, strlen(buffer), fileName );
		if ( src.IsLoaded() ) {
			hfStr classname;
			hfToken token;



			while ( src.ReadToken( &token ) ) {
				classname = token;
				src.ExpectTokenString( "{" );

				hfStrList list;
				while ( src.ReadToken( &token) ) {
					if ( token == "}" ) {
						break;
					}
					list.Append(token);
				}

				listHash.Set(classname, list);
			}
		}
		fileSystem->FreeFile( (void*)buffer );
	}

}

void LoadGuiParmExcludeList(hfStrList& list) {

	hfStr fileName = "guiparm_exclude.cfg";
	const char *buffer = NULL;
	hfLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	if ( fileSystem->ReadFile( fileName, (void**)&buffer ) > 0 ) {
		src.LoadMemory( buffer, strlen(buffer), fileName );
		if ( src.IsLoaded() ) {
			hfStr classname;
			hfToken token;



			while ( src.ReadToken( &token ) ) {
				list.Append(token);
			}
		}
		fileSystem->FreeFile( (void*)buffer );
	}
}

bool TestMapVal(hfStr& str) {
	//Already Localized?
	if(str.Find("#str_") != -1) {
		return false;
	}

	return true;
}

bool TestGuiParm(const char* parm, const char* value, hfStrList& excludeList) {

	hfStr testVal = value;

	//Already Localized?
	if(testVal.Find("#str_") != -1) {
		return false;
	}

	//Numeric
	if(testVal.IsNumeric()) {
		return false;
	}

	//Contains ::
	if(testVal.Find("::") != -1) {
		return false;
	}

	//Contains /
	if(testVal.Find("/") != -1) {
		return false;
	}

	if(excludeList.Find(testVal)) {
		return false;
	}

	return true;
}

void GetFileList(const char* dir, const char* ext, hfStrList& list) {

	//Recurse Subdirectories
	hfStrList dirList;
	Sys_ListFiles(dir, "/", dirList);
	for(int i = 0; i < dirList.Num(); i++) {
		if(dirList[i] == "." || dirList[i] == "..") {
			continue;
		}
		hfStr fullName = va("%s/%s", dir, dirList[i].c_str());
		GetFileList(fullName, ext, list);
	}

	hfStrList fileList;
	Sys_ListFiles(dir, ext, fileList);
	for(int i = 0; i < fileList.Num(); i++) {
		hfStr fullName = va("%s/%s", dir, fileList[i].c_str());
		list.Append(fullName);
	}
}

int LocalizeMap(const char* mapName, hfLangDict &langDict, ListHash& listHash, hfStrList& excludeList, bool writeFile) {

	common->Printf("Localizing Map '%s'\n", mapName);

	int strCount = 0;
	
	hfMapFile map;
	if ( map.Parse(mapName, false, false ) ) {
		int count = map.GetNumEntities();
		for ( int j = 0; j < count; j++ ) {
			hfMapEntity *ent = map.GetEntity( j );
			if ( ent ) {

				hfStr classname = ent->epairs.GetString("classname");

				//Hack: for info_location
				bool hasLocation = false;

				hfStrList* list;
				listHash.Get(classname, &list);
				if(list) {

					for(int k = 0; k < list->Num(); k++) {

						hfStr val = ent->epairs.GetString((*list)[k], "");
						
						if(val.Length() && classname == "info_location" && (*list)[k] == "location") {
							hasLocation = true;
						}

						if(val.Length() && TestMapVal(val)) {
							
							if(!hasLocation || (*list)[k] == "location") {
								//Localize it!!!
								strCount++;
								ent->epairs.Set( (*list)[k], langDict.AddString( val ) );
							}
						}
					}
				}

				listHash.Get("all", &list);
				if(list) {
					for(int k = 0; k < list->Num(); k++) {
						hfStr val = ent->epairs.GetString((*list)[k], "");
						if(val.Length() && TestMapVal(val)) {
							//Localize it!!!
							strCount++;
							ent->epairs.Set( (*list)[k], langDict.AddString( val ) );
						}
					}
				}

				//Localize the gui_parms
				const hfKeyValue* kv = ent->epairs.MatchPrefix("gui_parm");
				while( kv ) {
					if(TestGuiParm(kv->GetKey(), kv->GetValue(), excludeList)) {
						//Localize It!
						strCount++;
						ent->epairs.Set( kv->GetKey(), langDict.AddString( kv->GetValue() ) );
					}
					kv = ent->epairs.MatchPrefix( "gui_parm", kv );
				}
			}
		}
		if(writeFile && strCount > 0)  {
			//Before we write the map file lets make a backup of the original
			hfStr file =  fileSystem->RelativePathToOSPath(mapName);
			hfStr bak = file.Left(file.Length() - 4);
			bak.Append(".bak_loc");
			fileSystem->CopyFile( file, bak );
			
			map.Write( mapName, ".map" );
		}
	}

	common->Printf("Count: %d\n", strCount);
	return strCount;
}

/*
=================
LocalizeMaps_f
=================
*/
CONSOLE_COMMAND( localizeMaps, "localize maps", NULL ) {
	if ( args.Argc() < 2 ) {
		common->Printf( "Usage: localizeMaps <count | dictupdate | all> <map>\n" );
		return;
	}

	int strCount = 0;
	
	bool count = false;
	bool dictUpdate = false;
	bool write = false;

	if ( hfStr::Icmp( args.Argv(1), "count" ) == 0 ) {
		count = true;
	} else if ( hfStr::Icmp( args.Argv(1), "dictupdate" ) == 0 ) {
		count = true;
		dictUpdate = true;
	} else if ( hfStr::Icmp( args.Argv(1), "all" ) == 0 ) {
		count = true;
		dictUpdate = true;
		write = true;
	} else {
		common->Printf( "Invalid Command\n" );
		common->Printf( "Usage: localizeMaps <count | dictupdate | all>\n" );
		return;

	}

	hfLangDict strTable;
	hfStr filename = va("strings/english%.3i.lang", com_product_lang_ext.GetInteger());

	{
		// I think this is equivalent...
		const byte * buffer = NULL;
		int len = fileSystem->ReadFile( filename, (void**)&buffer );
		if ( verify( len > 0 ) ) {
			strTable.Load( buffer, len, filename );
		}
		fileSystem->FreeFile( (void *)buffer );

		// ... to this
		//if ( strTable.Load( filename ) == false) {
		//	//This is a new file so set the base index
		//	strTable.SetBaseID(com_product_lang_ext.GetInteger()*100000);
		//}
	}

	common->SetRefreshOnPrint( true );
	
	ListHash listHash;
	LoadMapLocalizeData(listHash);

	hfStrList excludeList;
	LoadGuiParmExcludeList(excludeList);

	if(args.Argc() == 3) {
		strCount += LocalizeMap(args.Argv(2), strTable, listHash, excludeList, write);
	} else {
		hfStrList files;
		GetFileList("base/maps", "*.map", files);
		for ( int i = 0; i < files.Num(); i++ ) {
			hfStr file =  fileSystem->OSPathToRelativePath(files[i]);
			strCount += LocalizeMap(file, strTable, listHash, excludeList, write);		
		}
	}

	if(count) {
		common->Printf("Localize String Count: %d\n", strCount);
	}

	common->SetRefreshOnPrint( false );

	if(dictUpdate) {
		strTable.Save( filename );
	}
}

/*
=================
LocalizeGuis_f
=================
*/
CONSOLE_COMMAND( localizeGuis, "localize guis", NULL ) {

	if ( args.Argc() != 2 ) {
		common->Printf( "Usage: localizeGuis <all | gui>\n" );
		return;
	}

	hfLangDict strTable;

	hfStr filename = va("strings/english%.3i.lang", com_product_lang_ext.GetInteger());

	{
		// I think this is equivalent...
		const byte * buffer = NULL;
		int len = fileSystem->ReadFile( filename, (void**)&buffer );
		if ( verify( len > 0 ) ) {
			strTable.Load( buffer, len, filename );
		}
		fileSystem->FreeFile( (void *)buffer );

		// ... to this
		//if(strTable.Load( filename ) == false) {
		//	//This is a new file so set the base index
		//	strTable.SetBaseID(com_product_lang_ext.GetInteger()*100000);
		//}
	}

	hfFileList *files;
	if ( hfStr::Icmp( args.Argv(1), "all" ) == 0 ) {
		hfStr game = cvarSystem->GetCVarString( "game_expansion" );
		if(game.Length()) {
			files = fileSystem->ListFilesTree( "guis", "*.gui", true, game );
		} else {
			files = fileSystem->ListFilesTree( "guis", "*.gui", true );
		}
		for ( int i = 0; i < files->GetNumFiles(); i++ ) {
			commonLocal.LocalizeGui( files->GetFile( i ), strTable );
		}
		fileSystem->FreeFileList( files );

		if(game.Length()) {
			files = fileSystem->ListFilesTree( "guis", "*.pd", true, game );
		} else {
			files = fileSystem->ListFilesTree( "guis", "*.pd", true, "base" );
		}
		
		for ( int i = 0; i < files->GetNumFiles(); i++ ) {
			commonLocal.LocalizeGui( files->GetFile( i ), strTable );
		}
		fileSystem->FreeFileList( files );

	} else {
		commonLocal.LocalizeGui( args.Argv(1), strTable );
	}
	strTable.Save( filename );
}

CONSOLE_COMMAND( localizeGuiParmsTest, "Create test files that show gui parms localized and ignored.", NULL ) {

	common->SetRefreshOnPrint( true );

	hfFile *localizeFile = fileSystem->OpenFileWrite( "gui_parm_localize.csv" ); 
	hfFile *noLocalizeFile = fileSystem->OpenFileWrite( "gui_parm_nolocalize.csv" ); 

	hfStrList excludeList;
	LoadGuiParmExcludeList(excludeList);

	hfStrList files;
	GetFileList("base/maps", "*.map", files);

	for ( int i = 0; i < files.Num(); i++ ) {
		
		common->Printf("Testing Map '%s'\n", files[i].c_str());
		hfMapFile map;

		hfStr file =  fileSystem->OSPathToRelativePath(files[i]);
		if ( map.Parse(file, false, false ) ) {
			int count = map.GetNumEntities();
			for ( int j = 0; j < count; j++ ) {
				hfMapEntity *ent = map.GetEntity( j );
				if ( ent ) {
					const hfKeyValue* kv = ent->epairs.MatchPrefix("gui_parm");
					while( kv ) {
						if(TestGuiParm(kv->GetKey(), kv->GetValue(), excludeList)) {
							hfStr out = va("%s,%s,%s\r\n", kv->GetValue().c_str(), kv->GetKey().c_str(), file.c_str());
							localizeFile->Write( out.c_str(), out.Length() );
						} else {
							hfStr out = va("%s,%s,%s\r\n", kv->GetValue().c_str(), kv->GetKey().c_str(), file.c_str());
							noLocalizeFile->Write( out.c_str(), out.Length() );
						}
						kv = ent->epairs.MatchPrefix( "gui_parm", kv );
					}
				}
			}
		}
	}
	
	fileSystem->CloseFile( localizeFile );
	fileSystem->CloseFile( noLocalizeFile );

	common->SetRefreshOnPrint( false );
}


CONSOLE_COMMAND( localizeMapsTest, "Create test files that shows which strings will be localized.", NULL ) {

	ListHash listHash;
	LoadMapLocalizeData(listHash);


	common->SetRefreshOnPrint( true );

	hfFile *localizeFile = fileSystem->OpenFileWrite( "map_localize.csv" ); 
	
	hfStrList files;
	GetFileList("base/maps", "*.map", files);

	for ( int i = 0; i < files.Num(); i++ ) {

		common->Printf("Testing Map '%s'\n", files[i].c_str());
		hfMapFile map;

		hfStr file =  fileSystem->OSPathToRelativePath(files[i]);
		if ( map.Parse(file, false, false ) ) {
			int count = map.GetNumEntities();
			for ( int j = 0; j < count; j++ ) {
				hfMapEntity *ent = map.GetEntity( j );
				if ( ent ) {
					
					//Temp code to get a list of all entity key value pairs
					/*hfStr classname = ent->epairs.GetString("classname");
					if(classname == "worldspawn" || classname == "func_static" || classname == "light" || classname == "speaker" || classname.Left(8) == "trigger_") {
						continue;
					}
					for( int i = 0; i < ent->epairs.GetNumKeyVals(); i++) {
						const hfKeyValue* kv = ent->epairs.GetKeyVal(i);
						hfStr out = va("%s,%s,%s,%s\r\n", classname.c_str(), kv->GetKey().c_str(), kv->GetValue().c_str(), file.c_str());
						localizeFile->Write( out.c_str(), out.Length() );
					}*/

					hfStr classname = ent->epairs.GetString("classname");
					
					//Hack: for info_location
					bool hasLocation = false;

					hfStrList* list;
					listHash.Get(classname, &list);
					if(list) {

						for(int k = 0; k < list->Num(); k++) {

							hfStr val = ent->epairs.GetString((*list)[k], "");
							
							if(classname == "info_location" && (*list)[k] == "location") {
								hasLocation = true;
							}

							if(val.Length() && TestMapVal(val)) {
								
								if(!hasLocation || (*list)[k] == "location") {
									hfStr out = va("%s,%s,%s\r\n", val.c_str(), (*list)[k].c_str(), file.c_str());
									localizeFile->Write( out.c_str(), out.Length() );
								}
							}
						}
					}

					listHash.Get("all", &list);
					if(list) {
						for(int k = 0; k < list->Num(); k++) {
							hfStr val = ent->epairs.GetString((*list)[k], "");
							if(val.Length() && TestMapVal(val)) {
								hfStr out = va("%s,%s,%s\r\n", val.c_str(), (*list)[k].c_str(), file.c_str());
								localizeFile->Write( out.c_str(), out.Length() );
							}
						}
					}
				}
			}
		}
	}

	fileSystem->CloseFile( localizeFile );

	common->SetRefreshOnPrint( false );
}

/*
===============
hfCommonLocal::LocalizeSpecificMapData
===============
*/
void hfCommonLocal::LocalizeSpecificMapData( const char *fileName, hfLangDict &langDict, const hfLangDict &replaceArgs ) {
	hfStr out, ws, work;

	hfMapFile map;
	if ( map.Parse( fileName, false, false ) ) {
		int count = map.GetNumEntities();
		for ( int i = 0; i < count; i++ ) {
			hfMapEntity *ent = map.GetEntity( i );
			if ( ent ) {
				for ( int j = 0; j < replaceArgs.GetNumKeyVals(); j++ ) {
					const hfLangKeyValue *kv = replaceArgs.GetKeyVal( j );
					const char *temp = ent->epairs.GetString( kv->key );
					if ( ( temp != NULL ) && *temp ) {
						hfStr val = kv->value;
						if ( val == temp ) {
							ent->epairs.Set( kv->key, langDict.AddString( temp ) );
						}
					}
				}
			}
		}
		map.Write( fileName, ".map" );
	}
}

/*
===============
hfCommonLocal::LocalizeMapData
===============
*/
void hfCommonLocal::LocalizeMapData( const char *fileName, hfLangDict &langDict ) {
	const char *buffer = NULL;
	hfLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );

	common->SetRefreshOnPrint( true );

	if ( fileSystem->ReadFile( fileName, (void**)&buffer ) > 0 ) {
		src.LoadMemory( buffer, strlen(buffer), fileName );
		if ( src.IsLoaded() ) {
			common->Printf( "Processing %s\n", fileName );
			hfStr mapFileName;
			hfToken token, token2;
			hfLangDict replaceArgs;
			while ( src.ReadToken( &token ) ) {
				mapFileName = token;
				replaceArgs.Clear();
				src.ExpectTokenString( "{" );
				while ( src.ReadToken( &token) ) {
					if ( token == "}" ) {
						break;
					}
					if ( src.ReadToken( &token2 ) ) {
						if ( token2 == "}" ) {
							break;
						}
						replaceArgs.AddKeyVal( token, token2 );
					}
				}
				common->Printf( "  localizing map %s...\n", mapFileName.c_str() );
				LocalizeSpecificMapData( mapFileName, langDict, replaceArgs );
			}
		}
		fileSystem->FreeFile( (void*)buffer );
	}

	common->SetRefreshOnPrint( false );
}

/*
===============
hfCommonLocal::LocalizeGui
===============
*/
void hfCommonLocal::LocalizeGui( const char *fileName, hfLangDict &langDict ) {
	hfStr out, ws, work;
	const char *buffer = NULL;
	out.Empty();
	int k;
	char ch;
	char slash = '\\';
	char tab = 't';
	char nl = 'n';
	hfLexer src( LEXFL_NOFATALERRORS | LEXFL_NOSTRINGCONCAT | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
	if ( fileSystem->ReadFile( fileName, (void**)&buffer ) > 0 ) {
		src.LoadMemory( buffer, strlen(buffer), fileName );
		if ( src.IsLoaded() ) {
			hfFile *outFile = fileSystem->OpenFileWrite( fileName ); 
			common->Printf( "Processing %s\n", fileName );

			const bool captureToImage = false;
			UpdateScreen( captureToImage );
			hfToken token;
			while( src.ReadToken( &token ) ) {
				src.GetLastWhiteSpace( ws );
				out += ws;
				if ( token.type == TT_STRING ) {
					out += va( "\"%s\"", token.c_str() );
				} else {
					out += token;
				}
				if ( out.Length() > 200000 ) {
					outFile->Write( out.c_str(), out.Length() );
					out = "";
				}
				work = token.Right( 6 );
				if ( token.Icmp( "text" ) == 0 || work.Icmp( "::text" ) == 0 || token.Icmp( "choices" ) == 0 ) {
					if ( src.ReadToken( &token ) ) {
						// see if already exists, if so save that id to this position in this file
						// otherwise add this to the list and save the id to this position in this file
						src.GetLastWhiteSpace( ws );
						out += ws;
						token = langDict.AddString( token );
						out += "\"";
						for ( k = 0; k < token.Length(); k++ ) {
							ch = token[k];
							if ( ch == '\t' ) {
								out += slash;
								out += tab;
							} else if ( ch == '\n' || ch == '\r' ) {
								out += slash;
								out += nl;
							} else {
								out += ch;
							}
						}
						out += "\"";
					}
				} else if ( token.Icmp( "comment" ) == 0 ) {
					if ( src.ReadToken( &token ) ) {
						// need to write these out by hand to preserve any \n's
						// see if already exists, if so save that id to this position in this file
						// otherwise add this to the list and save the id to this position in this file
						src.GetLastWhiteSpace( ws );
						out += ws;
						out += "\"";
						for ( k = 0; k < token.Length(); k++ ) {
							ch = token[k];
							if ( ch == '\t' ) {
								out += slash;
								out += tab;
							} else if ( ch == '\n' || ch == '\r' ) {
								out += slash;
								out += nl;
							} else {
								out += ch;
							}
						}
						out += "\"";
					}
				}
			}
			outFile->Write( out.c_str(), out.Length() );
			fileSystem->CloseFile( outFile );
		}
		fileSystem->FreeFile( (void*)buffer );
	}
}

