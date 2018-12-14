#include "stdafx.h"
#include <WinSock2.h>
SOCKET* c = NULL;
int n = 0;
int preIndex = 0;
fd_set fdreadMain[4];
void sendReq(SOCKET c);
void sendError(SOCKET c);
char* html = NULL;

DWORD WINAPI sendFile(LPVOID arg);
typedef struct HandleSendFile {
	HANDLE h;
	DWORD state;
};
typedef struct  FileSocket {
	SOCKET s;
	FILE *f;
	HandleSendFile *Lstate;
};
HandleSendFile HandleSend[100];
DWORD WINAPI ClientThread(LPVOID arg)
{
	fd_set *fdread = (fd_set*)arg;

	while (true)
	{
		if (fdread->fd_count > 0) {
			int countCurnt = fdread->fd_count;
			fdread->fd_count = 100;
			TIMEVAL t;
			t.tv_sec = 100000;
			t.tv_usec = 0;
			int res = select(0, fdread, NULL, NULL, &t);

			for (int i = 0; i < countCurnt; i++)
			{
				sendReq(fdread->fd_array[i]);

			};
			fdread->fd_count = countCurnt;
			for (int i = 0; i < countCurnt; i++) {

				FD_CLR(fdread->fd_array[i], fdread);
			};
		}



	}
	return 0;
}

int main()
{
	for (int i = 0; i < 100; i++) {
		HandleSend[i].h = NULL;
		HandleSend[i].state = 0;
	}
	WSADATA DATA;
	WSAStartup(MAKEWORD(2, 2), &DATA);
	SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	SOCKADDR_IN saddr;
	saddr.sin_addr.S_un.S_addr = 0;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(8888);
	bind(s, (sockaddr*)&saddr, sizeof(saddr));
	listen(s, 1024);
	HANDLE handle[4];
	for (int i = 0; i < 4; i++) {
		handle[i] = NULL;
		FD_ZERO(&fdreadMain[i]);
		fdreadMain[i].fd_count = 0;
	}
	DWORD ID = 0;

	handle[0] = CreateThread(NULL, 0, ClientThread, (LPVOID)&fdreadMain[0], 0, &ID);
	while (true)
	{
		SOCKADDR_IN caddr;
		int clen = sizeof(caddr);
		SOCKET tmp = accept(s, (sockaddr*)&caddr, &clen);
		while (true) {
			if (fdreadMain[0].fd_count < 64) {
				FD_SET(tmp, &fdreadMain[0]);
				break;
			}
			else {
				int i, j = 0;
				for (i = 1; i < 4; i++) {
					if (handle[i] == NULL && i < 4) {
						FD_SET(tmp, &fdreadMain[i]);
						handle[i] = CreateThread(NULL, 0, ClientThread, (LPVOID)&fdreadMain[i], 0, &ID);
						j = 1;
						break;
					}
					else {
						if (fdreadMain[i].fd_count < 64) {
							FD_SET(tmp, &fdreadMain[i]);
							j = 1;
							break;
						}
					}
				}
				if (j == 1) break;
			}
		}
		for (int i = 1; i < 4; i++) {
			if (fdreadMain[i].fd_count == 0 && handle[i] != NULL) {
				DWORD exitcode;
				GetExitCodeThread(handle[i], &exitcode);
				TerminateThread(handle[i], exitcode);
				handle[i] = NULL;
			}
		};

		for (int i = 0; i < 100; i++) {
			if (HandleSend[i].state == 1) {
				CloseHandle(HandleSend[i].h);
				HandleSend[i].h = NULL;
				HandleSend[i].state = 0;
			}
		}


	}
	return 0;
}

void AppendHTML(char* str, int lenName)
{
	char* tmp1 = "\\\\";
	while (strstr(str, tmp1))
	{
		int len = strlen(str);
		char* found = strstr(str, tmp1);
		found[0] = '/';
		strcpy(found + 1, found + 2);
		memset(str + len - 1, 0, 1);
	}

	char* tmp = (char*)calloc(1024, 1);
	memset(tmp, 0, 1024);
	if (strncmp(str, "FOLDER", 6) == 0) {
		sprintf(tmp, "<img src=\"/FILE_\\Users\\ironman\\Desktop\\folder.jpg\" alt=\"folder\" width=\"35\" height=\"30\"><a href=\"http://localhost:8888/%s\">%s</a><br>", str, str+(strlen(str)-lenName));
		//sprintf(tmp, "<i class=\"material-icons\">folder_open</i><a href=\"%s\">%s</a><br>", str, str+6);
	}
	else {
		sprintf(tmp, "<img src=\"/FILE_\\Users\\ironman\\Desktop\\file.jpg\" alt=\"folder\" width=\"35\" height=\"30\"><a href=\"http://localhost:8888/%s\">%s</a><br>", str, str+ (strlen(str) - lenName));
	};
	int oldLen = html == NULL ? 0 : strlen(html);
	int newSize = oldLen + strlen(tmp) + 1;
	html = (char*)realloc(html, newSize);
	memset(html + oldLen, 0, newSize - oldLen);
	strcpy(html + oldLen, tmp);
	free(tmp);
	tmp = NULL;
}

void sendReq(SOCKET c)
{

	char headHtml[] = "HTTP/1.1 200 OK \r\n\r\n <html> <head> <link href=\"https://fonts.googleapis.com/icon?family=Material+Icons\" rel=\"stylesheet\"> <link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\"></head>";
	html = (char *)malloc(strlen(headHtml) * sizeof(char));
	sprintf(html, "%s", headHtml);
	char buffer[2048];
	char verb[1024];
	char path[1024];
	char http[1024];
	memset(buffer, 0, sizeof(buffer));
	recv(c, buffer, 1023, 0);
	printf("\n%s\n", buffer);
	memset(verb, 0, sizeof(verb));
	memset(path, 0, sizeof(path));
	memset(http, 0, sizeof(http));
	sscanf(buffer, "%s%s%s", verb, path, http);
	char* tmp = "%20";
	int len1, len2;
	while (strstr(path, tmp))
	{
		int len = strlen(path);
		char* found = strstr(path, tmp);
		found[0] = ' ';
		strcpy(found + 1, found + 3);
		memset(path + len - 2, 0, 2);
	};

	if (strncmp(verb, "GET", 3) == 0) {
		while ((strlen(path) > 1) && strstr(path + 1, "/"))
		{
			char* found = strstr(path + 1, "/");
			found[0] = '\\';
		}
		if ((strncmp(path, "/FOLDER_", 8) == 0) || (strcmp(path, "/") == 0))
		{
			if (strncmp(path, "/FOLDER_", 8) == 0)
			{
				len1 = strlen(path);

				strcpy(path, path + 8);
				memset(path + len1 - 8, 0, 8);
			}
			if (strcmp(path, "/") == 0)
			{
				WIN32_FIND_DATAA fData;
				int lenName;
				HANDLE hFind = FindFirstFileA("c:\\*.*", &fData);
				AppendHTML(fData.cFileName,2);
				BOOL next = FALSE;
				next = FindNextFileA(hFind, &fData);
				while (next == TRUE)
				{

					char* prefix = (char*)calloc(1024, 1);
					lenName = strlen(fData.cFileName);
					if (fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						sprintf(prefix, "FOLDER_%s", fData.cFileName);
					}
					else
					{
						char *changeExten;
						strrev(fData.cFileName);
						len1 = strlen(fData.cFileName);
						changeExten = strstr(fData.cFileName, ".");
						if (changeExten) {
							changeExten[0] = '-';
						};
						strrev(fData.cFileName);
						sprintf(prefix, "FILE_%s\\%s", path, fData.cFileName);
					}
					AppendHTML(prefix,lenName);
					free(prefix);
					prefix = NULL;
					next = FindNextFileA(hFind, &fData);
				};
				len1 = strlen(html);
				len2 = len1 + 8;
				html = (char*)realloc(html, len2);
				strcpy(html + len1, "</html>");
				send(c, html, strlen(html), 0);
				free(html);
				html = NULL;
				closesocket(c);
			}
			else
			{
				WIN32_FIND_DATAA fData;
				int lenName;
				char* fullpath = (char*)calloc(1024, 1);
				sprintf(fullpath, "c:\\%s\\*.*", path);
				HANDLE hFind = FindFirstFileA(fullpath, &fData);
				free(fullpath);
				fullpath = NULL;
				if (hFind != INVALID_HANDLE_VALUE) {
					BOOL next = FALSE;
					next = FindNextFileA(hFind, &fData);
					
					while (next == TRUE)
					{
						char* tmp = (char*)calloc(1024, 1);
						lenName = strlen(fData.cFileName);
						if (fData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
						{
							sprintf(tmp, "FOLDER_%s\\%s", path, fData.cFileName);
						}
						else {
							char *changeExten;
							strrev(fData.cFileName);
							len1 = strlen(fData.cFileName);

							changeExten = strstr(fData.cFileName, ".");
							if (changeExten) {
								changeExten[0] = '-';
							};
							strrev(fData.cFileName);
							sprintf(tmp, "FILE_%s\\%s", path, fData.cFileName);
						}


						AppendHTML(tmp,lenName);
						free(tmp);
						tmp = NULL;
						next = FindNextFileA(hFind, &fData);
					};
					len1 = strlen(html);
					len2 = len1 + 8;
					html = (char*)realloc(html, len2);
					strcpy(html + len1, "</html>");
					send(c, html, strlen(html), 0);
					free(html);
					html = NULL;
					closesocket(c);
				}
				else {
					sendError(c);
				};

				
			}
		}
		else
		{

			if (strncmp(path, "/FILE_", 6) == 0)
			{
				strcpy(path, path + 6);
				strrev(path);
				char *extenFile;
				extenFile = strstr(path, "-");
				if (extenFile != NULL) {
					extenFile[0] = '.';
				};

				strrev(path);
				char* fullpath = (char*)calloc(1024, 1);
				sprintf(fullpath, "c:\\%s", path);
				FILE* f = fopen(fullpath, "rb");
				if (f != NULL) {
					if (strncmp(path + (strlen(path) - 4), ".mp3", 4) == 0) {
						char* header = "HTTP/1.1 200\r\nContent-Type: audio/mp3\r\n\r\n";
						send(c, header, strlen(header), 0);
					}
					else {
						if (strncmp(path + (strlen(path) - 4), ".txt", 4) == 0) {
							char* header = "HTTP/1.1 200\r\nContent-Type: text/plain\r\n\r\n";
							send(c, header, strlen(header), 0);
						}
						else {
							if (strncmp(path + (strlen(path) - 4), ".mp4", 4) == 0) {
								char* header = "HTTP/1.1 200\r\nContent-Type: video/mp4\r\n\r\n";
								send(c, header, strlen(header), 0);
							}
							else {
								if (strncmp(path + (strlen(path) - 4), ".jpg", 4) == 0) {
									char* header = "HTTP/1.1 200\r\nContent-Type: image/jpeg\r\n\r\n";
									send(c, header, strlen(header), 0);
								}
								else {
									if (strncmp(path + (strlen(path) - 4), ".png", 4) == 0) {
										char* header = "HTTP/1.1 200\r\nContent-Type: image/jpeg\r\n\r\n";
										send(c, header, strlen(header), 0);
									}
									else {
										if (strncmp(path + (strlen(path) - 5), ".html", 5) == 0) {
											char* header = "HTTP/1.1 200\r\nContent-Type: text/html\r\n\r\n";
											send(c, header, strlen(header), 0);
										}
										else {
											if (strncmp(path + (strlen(path) - 4), ".pdf", 4) == 0) {
												char* header = "HTTP/1.1 200\r\nContent-Type: application/pdf\r\n\r\n";
												send(c, header, strlen(header), 0);
											}
											else {
												if (strncmp(path + (strlen(path) - 3), ".js", 3) == 0) {
													char* header = "HTTP/1.1 200\r\nContent-Type: text/javascript\r\n\r\n";
													send(c, header, strlen(header), 0);
												}
												else {
													char* header = "HTTP/1.1 200\r\n\r\n";
													send(c, header, strlen(header), 0);
												}
											}
										}

									}

								}

							}

						}
					}
					int indexHandle=9999;
					while (true) {
						for (int i = 0; i < 100; i++) {
							if (HandleSend[i].h == NULL) {
								indexHandle = i;
								break;
							}
						};
						if (indexHandle != 9999) break;
					};
					FileSocket *fS = (FileSocket *) calloc(1,sizeof(FileSocket));
					fS->s = c;
					fS->f = f;
					fS->Lstate = &HandleSend[indexHandle];
					DWORD IDF = 0;
					HandleSend[indexHandle].h = CreateThread(0, 0, sendFile, (LPVOID)fS, 0, &IDF);
				}
				else {
					sendError(c);
				};

				
				free(fullpath);
				fullpath = NULL;

			}
			else {
				sendError(c);
			}

		}
	}
	else {
		if (strncmp(verb, "POST", 4) == 0) {
			char *dataPost, *indexChar;
			dataPost = strstr(buffer, "username");
			indexChar = strstr(dataPost, "&");
			if (indexChar != NULL) {
				indexChar[0] = ' ';
			};
			char *responsePost = (char *)calloc(1024, 1);
			sprintf(responsePost, "HTTP/1.1 200 OK \r\n\r\n <html> <body style=\"background-color:powderblue;\"><div align=\"center\" style=\"padding-top:250px;\"> <h1 style=\"color:purple;border:1px solid red;\"> %s </h1></body> </html>", dataPost);
			send(c, responsePost, strlen(responsePost), 0);
			dataPost = NULL;
			indexChar = NULL;
			closesocket(c);
		}
		else {
			sendError(c);
		}
	};



}

void sendError(SOCKET c) {
	char* header = "HTTP/1.1 404\r\nContent-Type: text/html\r\n\r\n";
	send(c, header, strlen(header), 0);
	char *dataFile = (char *)calloc(1024, 1);
	memset(dataFile, 0, 1024);
	FILE *f = fopen("c:\\chuan\\error.html", "rb");
	if (f != NULL) {
		while (!feof(f))
		{
			int n = fread(dataFile, 1, 1024, f);
			int len1;
			if (n > 0)
			{
				len1 = send(c, dataFile, n, 0);
			}
			if (!(len1 > 0)) break;
			memset(dataFile, 0, 1024);
		}
		fclose(f);
		f = NULL;
	}
	else {
		dataFile = "found not error";
		send(c, dataFile, strlen(dataFile), 0);
	}
	closesocket(c);
}



DWORD WINAPI sendFile(LPVOID arg) {
	FileSocket *fs = (FileSocket *)arg;

	char* buffer = (char*)calloc(1024, 1);
	while (!feof(fs->f))
	{
		int n = fread(buffer, 1, 1024, fs->f);
		if (n > 0)
		{
			n = send(fs->s, buffer, n, 0);
		}
		if (!(n > 0)) break;
	}
	fclose(fs->f);
	fs->f = NULL;
	fs->Lstate->state = 1;
	closesocket(fs->s);
	return 0;
}