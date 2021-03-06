/*****************************************************************
Name : 
Date : 2017/10/23
By   : CharlotteHonG
Final: 2017/10/23
*****************************************************************/
//#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <windows.h>
#include <atlstr.h> 
using namespace std;

#define SENTENCE_LEN 100
#define _WIN32_WINNT 0x0500


CStringW getClipboard();
void WriteFiles(char* fileName, const CStringW& ClipStrW);
void SetClipboardStr(const CStringW& StrW);
void OptiText(CStringW& ClipStrW);
//================================================================
int main(int argc, char const *argv[]){
	HWND hWnd = GetConsoleWindow();
	ShowWindow(hWnd, SW_HIDE);


	// 獲取剪貼簿內容
	CStringW ClipStrW = getClipboard();
	WriteFiles("Paper.txt", ClipStrW);
	// 處理文字
	OptiText(ClipStrW);
	WriteFiles("PaperOpti.txt", ClipStrW);
	// 設定剪貼簿內容
	SetClipboardStr(ClipStrW);

	// 貼上
	/*
	keybd_event(VK_CONTROL ,0,0,0);
	Sleep(3);
	keybd_event(65+21 ,0,0,0);
	keybd_event(65+21 ,0,KEYEVENTF_KEYUP,0);
	Sleep(3);
	keybd_event(VK_CONTROL ,0,KEYEVENTF_KEYUP,0);
	*/
	return 0;
}
//================================================================
//獲取剪貼簿
CStringW getClipboard(){
	/* 轉型(char*)(LPCSTR)(CStringA)
	剪貼簿參考：https://goo.gl/bjzEeA
	轉型參考：https://goo.gl/bEmvbU */
	CStringW strData;
	if (OpenClipboard(NULL)) {
		HANDLE hClipboardData = GetClipboardData(CF_UNICODETEXT);
		//DWORD dwLength = GlobalSize(hClipboardData);
		if (hClipboardData) {
			WCHAR *pchData = (WCHAR*)GlobalLock(hClipboardData);
			if (pchData) {
				strData = pchData;
				GlobalUnlock(hClipboardData);
			}
		}
		CloseClipboard();
	}
	return strData; 
}
// 寫入文件
void WriteFiles(char* fileName, const CStringW& ClipStrW) {
	ofstream file (fileName, ios::binary);
	file.exceptions (ifstream::eofbit | ifstream::failbit | ifstream::badbit);
	// CP_UTF8 檔頭
	int UTF_8 = 0xBFBBEF;
	file.write((char*)&UTF_8, 3);
	// 寫入 CP_UTF8
	wstring str = ClipStrW;
	string result = std::string();
	result.resize(WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, NULL, 0, 0, 0) - 1);
	char* ptr = &result[0];
	WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, ptr, result.size(), 0, 0);
	file << result;
}
// 設定剪貼簿
void SetClipboardStr(const CStringW& StrW) {
	// 參考：https://goo.gl/XYppCD

	// 轉寬char
	const CStringA& strA = static_cast<const CStringA>(StrW);
	const char* clipText = static_cast<LPCSTR>(strA);
	wchar_t* wString = new wchar_t[strlen(clipText)+1];
	MultiByteToWideChar(CP_ACP, 0, clipText, -1, wString, strlen(clipText));
	wString[strlen(clipText)] = '\0';
	LPWSTR cwdBuffer = wString;
	// Get the current working directory:
	//if( (cwdBuffer = _wgetcwd( NULL, 0 )) == NULL ) return 1; 
	DWORD len = wcslen(cwdBuffer);
	HGLOBAL hdst;
	LPWSTR dst;
	// Allocate string for cwd
	hdst = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (len + 1) * sizeof(WCHAR));
	dst = (LPWSTR)GlobalLock(hdst);
	memcpy(dst, cwdBuffer, len * sizeof(WCHAR));
	dst[len] = 0;
	GlobalUnlock(hdst);
	// Set clipboard data
	if (!OpenClipboard(NULL)) return;
	EmptyClipboard();
	if (!SetClipboardData(CF_UNICODETEXT, hdst)) return;
	CloseClipboard();
	//free(cwdBuffer);
	delete[] wString;
}
// 處理文字
void OptiText(CStringW& ClipStrW) {
	wstring clipStr = (wstring)ClipStrW;
	// 取代文字
	auto StrReplace = [&](wstring& clipStr, wstring dst, wstring str) {
		for (string::size_type idx = clipStr.find(dst, 1);
			idx!=std::string::npos; 
			idx = clipStr.find(dst, idx+1))
		{
			clipStr.replace(idx, dst.length(), str);
			//cout << "idx=" << idx << endl;
		}
	};
	// 刪除跳行
	StrReplace(clipStr, L"\r\n", _T(" "));
	// 刪除連續空格
	size_t len = clipStr.length();
	for (size_t i = 0; i < len-1; i++) {
		if (clipStr[i] == L' ' && clipStr[i+1] == L' ') {
			clipStr.erase(i--,1);
			--len;
		}
	}
	// 句尾空行
	string::size_type idxPre=0;
	for (string::size_type idx = clipStr.find(L". ", 1);
		idx!=std::string::npos; 
		idx = clipStr.find(L". ", idx+1))
	{
		if (idx-idxPre < SENTENCE_LEN) {continue;}
		if (idx>0 && isdigit(clipStr[idx-1])) {continue;}
		clipStr.replace(idx, 2, L". \r\n\r\n");
		idxPre = idx;
	}
	// 結尾補空白方便銜接
	clipStr += L" ";
	ClipStrW = (CStringW)clipStr.c_str();
}