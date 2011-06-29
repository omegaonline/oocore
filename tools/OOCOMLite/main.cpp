///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
//
// This file is part of OOCOMInterop, the Omega Online COM Interop library.
//
// OOCOMInterop is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCOMInterop is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCOMInterop.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include <initguid.h>

////////////////////////////////////////////////
// ObjBase.h has stupid predefines 
// which do not work with all compilers..
#define DllGetClassObject ObjBase_DllGetClassObject
#define DllCanUnloadNow ObjBase_DllCanUnloadNow

#include <windows.h>
#include <objbase.h>

#undef DllGetClassObject
#undef DllCanUnloadNow
////////////////////////////////////////////////

// {BD4D8C57-35ED-4f48-8302-2C90D837306F}
DEFINE_GUID(CLSID_OmegaInterop,0xbd4d8c57, 0x35ed, 0x4f48, 0x83, 0x2, 0x2c, 0x90, 0xd8, 0x37, 0x30, 0x6f);

HRESULT CreateClassFactory(void** ppv);
bool CanUnloadNow();

extern "C" __declspec(dllexport) HRESULT STDAPICALLTYPE DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
	if (rclsid != CLSID_OmegaInterop)
		return CLASS_E_CLASSNOTAVAILABLE;

	return CreateClassFactory(ppv);
}

extern "C" __declspec(dllexport) HRESULT STDAPICALLTYPE DllCanUnloadNow()
{
	return (CanUnloadNow() ? S_OK : S_FALSE);
}
