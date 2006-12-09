#define OMEGA_GUID_LINK_HERE
#include "stdafx.h"

#include <vld.h>

#include "./OORegEdit.h"
#include "./MainFrame.h"

IMPLEMENT_APP(OORegEditApp)

bool OORegEditApp::OnInit()
{
	Omega::Initialize_Minimal();

	wxFileSystem::AddHandler(new wxZipFSHandler);

	m_pfsResources = new wxFileSystem;

	// create and show the main frame
    MainFrame* frame = new MainFrame;
    frame->Show(true);

	return true;
}

int OORegEditApp::OnExit()
{
	delete m_pfsResources;

	Omega::Uninitialize();

	return 0;
}

wxImage* OORegEditApp::LoadImage(const wxString& filename, wxBitmapType type)
{
	// Create a URL
	wxString combinedURL(wxT("OORegEdit.bin#zip:") + filename);
	
	// Open the file in the archive
	wxFSFile* file = m_pfsResources->OpenFile(combinedURL);
	if (file)
	{
		wxInputStream* stream = file->GetStream();

		// Load the image
		wxImage* image = new wxImage(*stream, type);
				
		delete file;

		return image;
	}

	return NULL;
}
