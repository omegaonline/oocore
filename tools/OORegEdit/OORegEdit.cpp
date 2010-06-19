#include "stdafx.h"

#ifdef HAVE_VLD_H
#include <vld.h>
#endif

#include "OORegEdit.h"
#include "MainFrame.h"

IMPLEMENT_APP(OORegEditApp)

bool OORegEditApp::OnInit()
{
	Omega::IException* pE = Omega::Initialize();
	if (pE)
	{
		wxString strText = wxString::Format(_("Exception: %ls."),pE->GetDescription().c_str());

		wxMessageDialog dlg(NULL,strText,_("Critical Error!"),wxOK | wxICON_EXCLAMATION);
		dlg.ShowModal();
		pE->Release();
		return false;
	}

	// create and show the main frame
	MainFrame* frame = new MainFrame;

	frame->Show(true);

	return true;
}

int OORegEditApp::OnExit()
{
	Omega::Uninitialize();

	return 0;
}
