#pragma once
#include <Windows.h>
#include <string.h>

struct �ItemFile
{
	char				name_file[MAX_PATH];	// ��� ����� ��� ��������
	char				type;					// 0 - �������, 1 - ����
	long				size_file;
	unsigned long long	date;

	�ItemFile()
	{
		name_file[0] = 0;
		type = size_file = date = 0;
	}

	void SetNameFile(const char *name)
	{
		lstrcpyn(name_file, name, MAX_PATH - 1);
		name_file[MAX_PATH - 1] = 0;
	}

	void SetData(FILETIME &t)
	{
		ULARGE_INTEGER ui;
		ui.HighPart = t.dwHighDateTime;
		ui.LowPart = t.dwLowDateTime;
		date = ui.QuadPart;
	}

	void GetDate(FILETIME &t)
	{
		ULARGE_INTEGER ui;
		ui.QuadPart = date;
		t.dwHighDateTime = ui.HighPart;
		t.dwLowDateTime = ui.LowPart;
	}

	void GetDate(SYSTEMTIME &st)
	{
		FILETIME t;
		GetDate(t);
		FileTimeToSystemTime(&t, &st);
	}
};




class CCtrlListFile
{
public:

	char szPathParent[MAX_PATH];

	void GetList(vector<�ItemFile> & vItemFiles)
	{
		SearchFilePrt( vItemFiles , szPathParent);
	}

	void GetList(vector<�ItemFile> & vItemFiles, const char *search)
	{
		SearchFilePrt(vItemFiles, szPathParent, search);
	}

	CCtrlListFile()
	{
		GetPuthModule(szPathParent);
	}

	CCtrlListFile(const char *path)
	{
		if(path && path[0])
			SetPathParent(path);
		else GetPuthModule(szPathParent);
	}

	void SetPathParent(const char *path)
	{
		lstrcpyn(szPathParent, path, MAX_PATH - 1);
		szPathParent[MAX_PATH - 1] = 0;
	}

private:

	//// ����� ������ � ����� ������� 
	void SearchFilePrt(vector<�ItemFile> &_data, char szPath[MAX_PATH] )
	{
		_data.clear();
		WIN32_FIND_DATA fd;
		char * CONST lpLastChar = szPath + lstrlen(szPath);
		lstrcat(szPath, "\\*");
		HANDLE const hFind = FindFirstFile(szPath, &fd);
		*lpLastChar = '\0';
		if (hFind == INVALID_HANDLE_VALUE) return ;
		if (FindNextFile(hFind, &fd) == NULL) return ; // ���������� ���� �����
		�ItemFile item;
		item.type = 0;
		do
		{
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { // ����������
				item.type = 0;
			}
			else {
				item.type = 1;
				item.size_file = fd.nFileSizeLow;
			}
			// ����				
			item.SetNameFile(fd.cFileName);
			item.SetData(fd.ftLastWriteTime);
			_data.push_back(item);

		} while (FindNextFile(hFind, &fd) != NULL);
		FindClose(hFind);
	}

	// ����� ������ � ����� ������� 
	void SearchFilePrt(vector<�ItemFile> &_data, char szPath[MAX_PATH], const char *search)
	{
		WIN32_FIND_DATA fd;
		char * CONST lpLastChar = szPath + lstrlen(szPath);
		lstrcat(szPath, "\\*");
		HANDLE const hFind = FindFirstFile(szPath, &fd);
		*lpLastChar = '\0';
		if (hFind == INVALID_HANDLE_VALUE) return ;
		�ItemFile item;
		item.type = 1;
		do
		{
			if (lstrcmp(fd.cFileName, ".") == 0 || lstrcmp(fd.cFileName, "..") == 0) continue;
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				lstrcat(szPath, "\\");
				lstrcat(szPath, fd.cFileName);
				SearchFilePrt(_data, szPath, search);
				*lpLastChar = '\0';
				continue;
			}
			if ( strstr(fd.cFileName, search) != NULL ) {
				item.size_file = fd.nFileSizeLow;
				lstrcat(szPath, "\\");
				lstrcat(szPath, fd.cFileName);
				item.SetNameFile(szPath);
				item.SetData(fd.ftLastWriteTime);
				_data.push_back(item);
				*lpLastChar = '\0';
			}
		} while (FindNextFile(hFind, &fd) != NULL);
		FindClose(hFind);
	}

	void GetPuthModule(char name_puth[MAX_PATH])
	{
		GetModuleFileName(NULL, name_puth, MAX_PATH);
		char *p = strrchr(name_puth, '\\');
		if (p) *p = 0;
	}

};