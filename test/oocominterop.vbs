rem On Error Resume Next

Set fso = WScript.CreateObject("Scripting.FileSystemObject")
Set shell = WScript.CreateObject("WScript.Shell")

curdir = shell.CurrentDirectory
dll = fso.GetAbsolutePathName("..\tools\OOCOMLite\.libs\OOCOMLite.dll")
shell.CurrentDirectory = "..\src\OOCore\.libs"

Rem Do the registration
shell.RegWrite "HKCU\Software\Classes\Omega.Interop\", "Omega Online COM Interop"
shell.RegWrite "HKCU\Software\Classes\Omega.Interop\CLSID\", "{BD4D8C57-35ED-4F48-8302-2C90D837306F}"
shell.RegWrite "HKCU\Software\Classes\CLSID\{BD4D8C57-35ED-4F48-8302-2C90D837306F}\", "Omega Online COM Interop"
shell.RegWrite "HKCU\Software\Classes\CLSID\{BD4D8C57-35ED-4F48-8302-2C90D837306F}\InprocServer32\", dll
shell.RegWrite "HKCU\Software\Classes\CLSID\{BD4D8C57-35ED-4F48-8302-2C90D837306F}\InprocServer32\ThreadingModel", "Apartment"

Set oo = WScript.CreateObject("Omega.Interop")
oo.InitializeArgs = "standalone=true, regdb_path=..\..\..\data,user_regdb=..\..\..\data\default_user.regdb" 

Set r = oo.CreateInstance("Omega.Registry")

Rem Clean up behind ourselves
shell.RegDelete "HKCU\Software\Classes\Omega.Interop\CLSID\"
shell.RegDelete "HKCU\Software\Classes\Omega.Interop\"
shell.RegDelete "HKCU\Software\Classes\CLSID\{BD4D8C57-35ED-4F48-8302-2C90D837306F}\InprocServer32\"
shell.RegDelete "HKCU\Software\Classes\CLSID\{BD4D8C57-35ED-4F48-8302-2C90D837306F}\"

shell.CurrentDirectory = curdir
