#pragma once

#include "ServerTCPThreadPool.h"
#include "ListFilesDirect.h"
#include <string>


enum E_Commnd
{
	E_LIST_FILE,				// Получить список файлов
	E_FILE_SERVER_TO,			// Записать файл на сервре
	E_FILE_SERVER_FROM,			// Отправить файл с сервера
	E_FILE_SERVER_FROM_PART,	// Прочитать файл с сервера
	E_FIND_FILE,				// Найти файлы на сервере
	E_DIR_PARENT				// Получить путь на сервере
};

// Обработка комманд
E_Result HandleCmd(TSocketData& buff_cmd, TCHAR_ADDRESS address)
{

	return e_OK;
}

struct CFile
{
	FILE* file;

//	unsigned long    size_file;

	CFile() :file(NULL)  {
	}

	~CFile() {
		if (file) Close();
	}

	bool Open_w(const char* name_file) 
	{
		errno_t er = fopen_s(&file, name_file, "wb");
		return file != NULL && er == 0;
	}

	bool Open_r(const char* name_file) 
	{
		errno_t er = fopen_s(&file, name_file, "rb");
		return file != NULL && er == 0;
	}

	unsigned long Size()
	{
		if (file)	{
			fseek(file, 0, SEEK_END);   //пробежка по файлу
			unsigned long  size_file = ftell(file);
			fseek(file, 0, SEEK_SET);
			return size_file;
		}
		return 0;
	}

	bool IsOpen()
	{
		return file != NULL;
	}

	void Close()
	{
		fclose(file);
		file = NULL;
	}

	void Write(char *buff, unsigned long size )
	{
		fwrite(buff, size, 1, file);
	}

	int Write(TSocketData& buff_cmd)
	{
		long size = 0;
		while (buff_cmd.header.size) {
		//	buff_cmd.p_buff = buff;
		//	ZeroMemory(buff_cmd.p_buff, DEFAULT_BUFLEN);
			size = buff_cmd.ReadBuff(DEFAULT_BUFLEN);
			if (size < 1) return size;
			fwrite(buff_cmd.p_buff, size, 1, file);
			buff_cmd.header.size -= size;
		}
		return 1;
	}

	int Read(TSocketData& buff_cmd)
	{
		long size = 0;
		int iRes = 0;
		while (!feof(file)) {
	//		ZeroMemory(buff_cmd.p_buff, DEFAULT_BUFLEN);
			size = fread_s(buff_cmd.p_buff, DEFAULT_BUFLEN, sizeof(char), DEFAULT_BUFLEN, file);
			if (size == 0) break;
			iRes = buff_cmd.SendBuff(size);
			if (iRes < 1) break;
		}
		return iRes;
	}

	unsigned long Read(char *buff, unsigned long _size)
	{
		return fread_s(buff, _size, sizeof(char), _size, file);
	}

	unsigned long Read(unsigned long off, char *buff, unsigned long _size)
	{
		fseek(file, off, SEEK_SET);
		return fread_s(buff, _size, sizeof(char), _size, file);
	}
};

class CHandler
{
public:

	CHandler()
	{
		ZeroMemory(buff, DEFAULT_BUFLEN);
	}

	int HandleCmd(TSocketData& buff_cmd, TCHAR_ADDRESS address)
	{
		switch (buff_cmd.header.cmd)
		{
		case E_LIST_FILE:

			return SendListFiletoClient(buff_cmd);

		case E_FILE_SERVER_TO:

			return ResiveFile(buff_cmd);

		case E_FILE_SERVER_FROM:

			return SendFile(buff_cmd);

		case E_FILE_SERVER_FROM_PART:

			return SendFilePart(buff_cmd);

		case E_DIR_PARENT:

			return SendDirToClient(buff_cmd);

		case E_FIND_FILE:
			
			return SendListFindFiletoClient(buff_cmd);
		}
		return 1;
	}

private:

	int ResiveFile(TSocketData& buff_cmd)
	{
		ZeroMemory(buff, DEFAULT_BUFLEN);
		buff_cmd.p_buff = buff;
		long size = buff_cmd.ReadBuff(DEFAULT_BUFLEN);
		if (size < 1) return 1;
		CFile curr_file;
		std::string name_file = "temp.png";
		buff_cmd.p_buff = GetNameFileAndData(buff_cmd.p_buff, &size, name_file);
		if (name_file.length() == 0 || curr_file.Open_w(name_file.c_str()) == false) {
			// Ошибка создания файла
			//	_printf("Ошибка файда");
			return 1;
		}
		//	_printf(name_file.c_str());
		if (size) {
			curr_file.Write(buff_cmd.p_buff, size);
			buff_cmd.header.size -= size;
		}
		buff_cmd.p_buff = buff;
		size = curr_file.Write(buff_cmd);
		curr_file.Close();
		return size;
	}

	int SendListFiletoClient(TSocketData& buff_out)
	{
		const char *pathParent = NULL;
		if ( buff_out.header.size ) {
			buff_out.p_buff = buff;
			if( buff_out.ReadBuff(buff_out.header.size) )	pathParent = buff_out.p_buff;
		}
		// Получить список файлов и каталогов из текущего директория
		vector<СItemFile> vItemFile;
		CCtrlListFile listFile(pathParent);
		listFile.GetList(vItemFile);
		int iRes = 0;
		// Отослать клиенту
		size_t n = vItemFile.size();
		if (n) {
			buff_out.ClearHeaderBuffNull();
			buff_out.header.size = n * sizeof(СItemFile);
			buff_out.p_buff = (char *)&vItemFile[0];
			iRes = buff_out.SendHeader();
			if (iRes > 0)	iRes = buff_out.SendBuff();
		}
		return iRes;
	}

	int SendListFindFiletoClient(TSocketData& buff_out)
	{	
		if (buff_out.header.size == 0) return 0;
		ZeroMemory(buff, DEFAULT_BUFLEN);
		buff_out.p_buff = buff;
		long size = buff_out.ReadBuff(buff_out.header.size);
		std::string path;
		buff_out.p_buff = GetNameFileAndData(buff_out.p_buff, &size, path);		
		// Получить размер имени файла и имя файла для поиска 
		std::string search_file;
		buff_out.p_buff = GetNameFileAndData(buff_out.p_buff, &size, search_file);
		// Получить список файлов и каталогов из текущего директория
		vector<СItemFile> vItemFile;
		CCtrlListFile listFile(path.c_str() );
		listFile.GetList(vItemFile, search_file.c_str());
		int iRes = 0;
		// Отослать клиенту
		size_t n = vItemFile.size();
		if (n) {
			buff_out.ClearHeaderBuffNull();
			buff_out.header.size = n * sizeof(СItemFile);
			buff_out.p_buff = (char *)&vItemFile[0];
			iRes = buff_out.SendHeader();
			if (iRes > 0)	iRes = buff_out.SendBuff();
		}
		return iRes;
	}

	int SendDirToClient(TSocketData& buff_out)
	{
		int iRes = 0;
		CCtrlListFile listFile;
		buff_out.ClearHeaderBuffNull();
		if (listFile.szPathParent[0]) {
			buff_out.header.size = strlen(listFile.szPathParent);
			buff_out.p_buff = listFile.szPathParent;
			iRes = buff_out.SendHeader();
			if (iRes > 0)	iRes = buff_out.SendBuff();
		}
		return iRes;
	}

	int SendFile(TSocketData& buff_cmd)
	{
		int iRes = 1;
		ZeroMemory(buff, DEFAULT_BUFLEN);
		buff_cmd.p_buff = buff;
		long size = buff_cmd.ReadBuff(buff_cmd.header.size);
		if (size < 1) return 1;
		const char *nameFile = buff_cmd.p_buff;
		CFile curr_file;
		if (nameFile && nameFile[0] && curr_file.Open_r(nameFile)) {
			buff_cmd.header.size = curr_file.Size();
			iRes = buff_cmd.SendHeader();
			if (iRes == -1) return iRes;
			buff_cmd.p_buff = buff;
			iRes = curr_file.Read(buff_cmd);
		}
		return iRes;
	}

	int SendFilePart( TSocketData& buff_cmd )
	{
		int iRes = 1;
		ZeroMemory(buff, DEFAULT_BUFLEN);
		buff_cmd.p_buff = buff;
		long size = buff_cmd.ReadBuff(buff_cmd.header.size);
		if (size < 1) return 1;
		const char *nameFile = buff_cmd.p_buff;
		CFile curr_file;
		if (nameFile && nameFile[0] && curr_file.Open_r(nameFile)) {
			buff_cmd.header.size = curr_file.Size();
			iRes = buff_cmd.SendHeader();
			if (iRes == -1) return iRes;
			ZeroMemory(buff, DEFAULT_BUFLEN);
			buff_cmd.p_buff = buff;
			size = curr_file.Read(buff_cmd.header.param, buff_cmd.p_buff, DEFAULT_BUFLEN);
			if (size) iRes = buff_cmd.SendBuff(size);
		}
		return iRes;
	}

	char* GetNameFileAndData(char* buff_src, long* size_buff, std::string& name_file)
	{
		name_file.clear();
		int size_name = *((int*)(buff_src));
		buff_src += sizeof(int);
		if (size_name + sizeof(int) > *size_buff) return buff_src;
		name_file.append(buff_src, size_name);
		buff_src += size_name;
		*size_buff -= (size_name + sizeof(int));
		return buff_src;
	}

	char buff[DEFAULT_BUFLEN];
};