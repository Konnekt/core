#pragma once

namespace Konnekt {

	int CALLBACK ProfileDialogProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);
	int CALLBACK PlugsDialogProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

	void InitInstance(HINSTANCE hInst);
};