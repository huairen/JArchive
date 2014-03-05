#include "JWin/JApp.h"
#include "MainDlg.h"
//#include <vld.h>

class ArchiveApp : public JApp
{
public:
	ArchiveApp(HINSTANCE hInst, LPSTR lpCmdLine, int nCmdShow) : JApp(hInst,lpCmdLine,nCmdShow)
	{ }

	bool InitWindow()
	{
		if(!m_MainDlg.Create())
			return false;

		m_MainDlg.MoveToCenter();
		m_MainDlg.Show();

		return true;
	}

private:
	CMainDlg m_MainDlg;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	ArchiveApp app(hInstance,lpCmdLine,nCmdShow);
	return app.Run();
}