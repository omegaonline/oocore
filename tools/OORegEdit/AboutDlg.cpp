// -*- C++ -*- generated by wxGlade 0.4.1 on Thu May 18 08:11:39 2006

#include "stdafx.h"
#include "AboutDlg.h"
#include "OORegEdit.h"


AboutDlg::AboutDlg(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    wxDialog(parent, id, title, pos, size, wxDEFAULT_DIALOG_STYLE)
{
	wxImage* pImage = wxGetApp().LoadImage(wxT("frame_icon.bmp"));
	if (pImage)
		pImage->SetMaskColour(255,0,255);

    // begin wxGlade: AboutDlg::AboutDlg
    m_picIcon = new wxStaticBitmap(this, -1, wxBitmap(pImage,wxBITMAP_TYPE_ANY));
    m_lblDesc = new wxStaticText(this, -1, _("Omega Online Registry Editor\n\nWARNING: \nEditing this registry can stop Omega Online functioning.  \nOnly edit parts of the registry you understand."));
    m_line = new wxStaticLine(this, -1);
    m_lblVersion = new wxStaticText(this, -1, _("Version 0.1 (Build 1)"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
    m_buttons = new wxStdButtons(this, -1, wxOK);

    set_properties();
    do_layout();
    // end wxGlade

	delete pImage;
}


void AboutDlg::set_properties()
{
    // begin wxGlade: AboutDlg::set_properties
    SetTitle(_("About Omega Online Registry Editor"));
    // end wxGlade
}


void AboutDlg::do_layout()
{
    // begin wxGlade: AboutDlg::do_layout
    wxBoxSizer* m_sizer_1 = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* m_sizer_2 = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* m_sizer_3 = new wxBoxSizer(wxVERTICAL);
    m_sizer_1->Add(300, 10, 0, wxADJUST_MINSIZE, 0);
    m_sizer_2->Add(10, 20, 0, wxADJUST_MINSIZE, 0);
    m_sizer_2->Add(m_picIcon, 0, wxALL|wxADJUST_MINSIZE, 7);
    m_sizer_2->Add(10, 20, 0, wxADJUST_MINSIZE, 0);
    m_sizer_3->Add(m_lblDesc, 0, wxRIGHT|wxTOP|wxBOTTOM|wxEXPAND, 7);
    m_sizer_3->Add(m_line, 0, wxRIGHT|wxEXPAND, 7);
    m_sizer_3->Add(m_lblVersion, 0, wxALL|wxEXPAND, 7);
    m_sizer_2->Add(m_sizer_3, 1, wxEXPAND, 0);
    m_sizer_1->Add(m_sizer_2, 1, wxEXPAND, 0);
    m_sizer_1->Add(m_buttons, 0, wxALL|wxEXPAND, 7);
    SetAutoLayout(true);
    SetSizer(m_sizer_1);
    m_sizer_1->Fit(this);
    m_sizer_1->SetSizeHints(this);
    Layout();
    // end wxGlade
}
