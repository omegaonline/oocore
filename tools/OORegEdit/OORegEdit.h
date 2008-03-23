#ifndef OOREGEDIT_OOREGEDIT_H_INCLUDED_
#define OOREGEDIT_OOREGEDIT_H_INCLUDED_

class OORegEditApp : public wxApp
{
public:
	OORegEditApp()
	{
	}

	virtual bool OnInit();
	virtual int OnExit();

	wxImage* LoadImage(const wxString& name, wxBitmapType type = wxBITMAP_TYPE_BMP);

private:
	wxFileSystem* m_pfsResources;

	DECLARE_NO_COPY_CLASS(OORegEditApp)
};

DECLARE_APP(OORegEditApp)

#endif // OOREGEDIT_OOREGEDIT_H_INCLUDED_
