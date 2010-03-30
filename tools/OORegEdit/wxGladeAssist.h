#ifndef WXSTDBUTTONS_H_INCLUDED_
#define WXSTDBUTTONS_H_INCLUDED_

class wxStdButtons : public wxStdDialogButtonSizer
{
public:
	wxStdButtons(wxDialog* pDlg, int WXUNUSED(id), long flags = wxID_OK) :
		wxStdDialogButtonSizer()
	{
		wxButton *ok = NULL;
		wxButton *yes = NULL;
		wxButton *no = NULL;

		if (flags & wxOK){
			ok = new wxButton(pDlg, wxID_OK);
			AddButton(ok);
		}

		if (flags & wxCANCEL){
			wxButton *cancel = new wxButton(pDlg, wxID_CANCEL);
			AddButton(cancel);
		}

		if (flags & wxYES){
			yes = new wxButton(pDlg, wxID_YES);
			AddButton(yes);
		}

		if (flags & wxNO){
			no = new wxButton(pDlg, wxID_NO);
			AddButton(no);
		}

		if (flags & wxHELP){
			wxButton *help = new wxButton(pDlg, wxID_HELP);
			AddButton(help);
		}

		if (flags & wxNO_DEFAULT)
		{
			if (no)
			{
				no->SetDefault();
			}
		}
		else
		{
			if (ok)
			{
				ok->SetDefault();
			}
			else if (yes)
			{
				yes->SetDefault();
			}
		}

		if (flags & wxOK)
			SetAffirmativeButton(ok);
		else if (flags & wxYES)
			SetAffirmativeButton(yes);

		Realize();
	}
};

#endif // WXSTDBUTTONS_H_INCLUDED_
