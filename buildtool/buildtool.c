// DXGL
// Copyright (C) 2012-2024 William Feely

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#define _CRT_SECURE_NO_DEPRECATE
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include "../common/releasever.h"

typedef struct
{
	int major;
	int minor;
	int point;
	int build;
	BOOL beta;
	char verstring[MAX_PATH];
	char revision[41];
	char branch[MAX_PATH];
} DXGLVER;

BOOL IsWOW64()
{
	HANDLE hKernel32;
#ifdef _WIN64
	return TRUE;
#else
	BOOL is64;
	BOOL(__stdcall * _IsWow64Process)(HANDLE hProcess, PBOOL Wow64Process) = NULL;
	hKernel32 = GetModuleHandle(_T("kernel32.dll"));
	if(hKernel32) _IsWow64Process = (BOOL(__stdcall *)(HANDLE, PBOOL))
		GetProcAddress(hKernel32, "IsWow64Process");
	if (_IsWow64Process)
	{
		if (!_IsWow64Process(GetCurrentProcess(), &is64)) return FALSE;
		else return is64;
	}
	else return FALSE;
#endif
}

static const char git_install_path[] = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Git_is1";

int GetGitVersion(char *path, DXGLVER *version)
{
	char pathbase[FILENAME_MAX + 1];
	char pathin[FILENAME_MAX + 1];
	char pathout[FILENAME_MAX + 1];
	char workingpath[MAX_PATH];
	char repopath[MAX_PATH + 4];
	HKEY hKey;
	BOOL foundgit = FALSE;
	char gitpath[(MAX_PATH + 1) * 4];
	char gitcmd[(MAX_PATH + 1) * 4];
	DWORD buffersize = MAX_PATH + 1;
	BOOL is64;
	FILE *pipe;
	char buffer[4096];
#ifdef _WIN64
	is64 = TRUE;
#else
	is64 = IsWOW64();
#endif
	strncpy(pathbase, path, FILENAME_MAX);
	strncpy(pathin, path, FILENAME_MAX);
	pathin[FILENAME_MAX] = 0;
	strncpy(pathout, path, FILENAME_MAX);
	pathout[FILENAME_MAX] = 0;
	pathbase[strlen(pathbase) - 7] = 0;
	strncat(pathin, "\\rev.in", FILENAME_MAX - strlen(pathin));
	pathin[FILENAME_MAX] = 0;
	strncat(pathout, "\\rev", FILENAME_MAX - strlen(pathin));
	pathout[FILENAME_MAX] = 0;
	if (is64)
	{
		// User X64 registry
		if (RegOpenKeyExA(HKEY_CURRENT_USER, git_install_path, 0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
		{
			// System X64 registry
			if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, git_install_path, 0, KEY_READ | KEY_WOW64_64KEY, &hKey) != ERROR_SUCCESS)
			{
				// User IA-32 registry
				if (RegOpenKeyExA(HKEY_CURRENT_USER, git_install_path, 0, KEY_READ | KEY_WOW64_32KEY, &hKey) != ERROR_SUCCESS)
				{
					// System IA-32 registry
					if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, git_install_path, 0, KEY_READ | KEY_WOW64_32KEY, &hKey) != ERROR_SUCCESS)
						hKey = 0;						
				}
			}
		}

	}
	else
	{
		// User registry
		if (RegOpenKeyExA(HKEY_CURRENT_USER, git_install_path, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		{
			// System registry
			if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, git_install_path, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
				hKey = 0;
		}
	}
	if (hKey)
	{
		if (RegQueryValueExA(hKey, "InstallLocation", NULL, NULL, (LPBYTE)gitpath, &buffersize) == ERROR_SUCCESS)
		{
			foundgit = TRUE;
			GetCurrentDirectoryA(MAX_PATH, workingpath);
			strcpy(repopath, path);
			strcat(repopath, "\\..");
			SetCurrentDirectoryA(repopath);
			strcat(gitpath, "bin\\git.exe ");
			strcpy(gitcmd, "\"");
			strcat(gitcmd, gitpath);
			strcat(gitcmd, "\" rev-parse HEAD");
			pipe = _popen(gitcmd, "r");
			if (pipe)
			{
				fgets(buffer, 4096, pipe);
				_pclose(pipe);
				if(strrchr(buffer, '\n')) *(strrchr(buffer, '\n')) = 0;
				buffer[40] = 0; // Buffer overrun protection
				strcpy(version->revision, buffer);
				strcpy(gitcmd, "\"");
				strcat(gitcmd, gitpath);
				strcat(gitcmd, "\" describe --always --long --dirty");
				pipe = _popen(gitcmd, "r");
				if (pipe)
				{
					fgets(buffer, 4096, pipe);
					_pclose(pipe);
					if (strrchr(buffer, '\n')) *(strrchr(buffer, '\n')) = 0;
					buffer[MAX_PATH] = 0; // Buffer overrun protection
					strcpy(version->verstring, buffer);
				}
				else
				{
					MessageBoxA(NULL, "Unexpected Git error", "Fatal Error", MB_OK | MB_ICONERROR);
					ExitProcess(1);
				}
				strcpy(gitcmd, "\"");
				strcat(gitcmd, gitpath);
				strcat(gitcmd, "\" rev-parse --abbrev-ref HEAD");
				pipe = _popen(gitcmd, "r");
				if (pipe)
				{
					fgets(buffer, 4096, pipe);
					_pclose(pipe);
					if (strrchr(buffer, '\n')) *(strrchr(buffer, '\n')) = 0;
					buffer[MAX_PATH] = 0; // Buffer overrun protection
					strcpy(version->branch, buffer);
				}
				else
				{
					MessageBoxA(NULL, "Unexpected Git error", "Fatal Error", MB_OK | MB_ICONERROR);
					ExitProcess(1);
				}
			}
			else foundgit = FALSE;
			if (!foundgit)
			{
				int result = MessageBoxA(NULL, "Could not find Git for Windows, would you like to download it?", "git.exe not found",
					MB_YESNO | MB_ICONWARNING);
				if (result == IDYES)
				{
					puts("ERROR:  Please try again after installing Git for Windows.");
					ShellExecuteA(NULL, "open", "https://git-scm.com/download/win", NULL, NULL, SW_SHOWNORMAL);
					exit(-1);
				}
				else return 0;
			}
			SetCurrentDirectoryA(workingpath);
		}
	}
	return 1;
}

/* No longer used after migration to Git
int GetSVNRev(char *path)
{
	char pathbase[FILENAME_MAX+1];
	char pathin[FILENAME_MAX+1];
	char pathout[FILENAME_MAX+1];
	HKEY hKey;
	BOOL foundsvn = FALSE;
	char svnpath[(MAX_PATH + 1) * 4];
	DWORD buffersize = MAX_PATH + 1;
	STARTUPINFOA startinfo;
	PROCESS_INFORMATION process;
	FILE *revfile;
	char revstring[32];
	strncpy(pathbase, path, FILENAME_MAX);
	strncpy(pathin,path,FILENAME_MAX);
	pathin[FILENAME_MAX] = 0;
	strncpy(pathout,path,FILENAME_MAX);
	pathout[FILENAME_MAX] = 0;
	pathbase[strlen(pathbase)-7] = 0;
	strncat(pathin,"\\rev.in",FILENAME_MAX-strlen(pathin));
	pathin[FILENAME_MAX] = 0;
	strncat(pathout,"\\rev",FILENAME_MAX-strlen(pathin));
	pathout[FILENAME_MAX] = 0;
	if(RegOpenKeyExA(HKEY_LOCAL_MACHINE,"Software\\TortoiseSVN",0,KEY_READ,&hKey) == ERROR_SUCCESS)
	{
		if(RegQueryValueExA(hKey,"Directory",NULL,NULL,(LPBYTE)svnpath,&buffersize) == ERROR_SUCCESS)
		{
			foundsvn = TRUE;
			strcat(svnpath,"bin\\subwcrev.exe ");
			strcat(svnpath,pathbase);
			strcat(svnpath," ");
			strcat(svnpath,pathin);
			strcat(svnpath," ");
			strcat(svnpath,pathout);
			ZeroMemory(&startinfo,sizeof(STARTUPINFOA));
			startinfo.cb = sizeof(STARTUPINFO);
			puts(svnpath);
			if(CreateProcessA(NULL,svnpath,NULL,NULL,FALSE,0,NULL,NULL,&startinfo,&process))
			{
				WaitForSingleObject(process.hProcess,INFINITE);
				CloseHandle(process.hProcess);
				CloseHandle(process.hThread);
				revfile = fopen(pathout,"r");
				if(!revfile)
				{
					puts("WARNING:  Failed to create revision file");
					RegCloseKey(hKey);
					return 0;
				}
				fgets(revstring,32,revfile);
				fclose(revfile);
				RegCloseKey(hKey);
				return atoi(revstring);
			}
			else foundsvn = FALSE;
		}
		RegCloseKey(hKey);
	}
	if(!foundsvn)
	{
		int result = MessageBoxA(NULL,"Could not find subwcrev.exe, would you like to download TortoiseSVN?","TortoiseSVN not found",
			MB_YESNO|MB_ICONWARNING);
		if(result == IDYES)
		{
			puts("ERROR:  Please try again after installing TortoiseSVN.");
			ShellExecuteA(NULL,"open","https://tortoisesvn.net/",NULL,NULL,SW_SHOWNORMAL);
			exit(-1);
		}
		else return 0;
	}
	return 0;
} */

void ParseVersion(DXGLVER *version, BOOL git)
{
	char numstring[16];
	char *findptr;
	char *findptr2;
	if (git)
	{
		ZeroMemory(numstring, 16);
		findptr = strchr(version->verstring, '.');
		if (findptr)
		{
			strncpy(numstring, version->verstring,
				((INT_PTR)findptr - (INT_PTR)version->verstring < 16 ?
				(INT_PTR)findptr - (INT_PTR)&version->verstring : 15));
			version->major = atoi(numstring);
			ZeroMemory(numstring, 16);
			findptr++;
			findptr2 = strchr(findptr, '.');
			if (findptr2)
			{
				strncpy(numstring, findptr,
					((INT_PTR)findptr2 - (INT_PTR)findptr < 16 ? (INT_PTR)findptr2 - (INT_PTR)findptr : 15));
				version->minor = atoi(numstring);
				ZeroMemory(numstring, 16);
				findptr2++;
				findptr = strchr(findptr2, '-');
				if (findptr)
				{
					strncpy(numstring, findptr2,
						((INT_PTR)findptr - (INT_PTR)findptr2 < 16 ? (INT_PTR)findptr - (INT_PTR)findptr2 : 15));
					version->build = atoi(numstring);
					ZeroMemory(numstring, 16);
					findptr++;
					findptr2 = strchr(findptr, '-');
					if (findptr2)
					{
						strncpy(numstring, findptr,
							((INT_PTR)findptr2 - (INT_PTR)findptr < 16 ? (INT_PTR)findptr2 - (INT_PTR)findptr : 15));
						version->build = atoi(numstring);
						if (version->build != 0) version->beta = TRUE;
						else version->beta = FALSE;
					}
				}
			}
		}
	}
	else
	{
		version->beta = FALSE;
		strcpy(version->branch, "Non-Git");
		strcpy(version->revision, "");
		strcpy(version->verstring, "0.5.19-0-Non-Git");
	}
}

int ProcessHeaders(char *path)
{
	char pathin[FILENAME_MAX+1];
	char pathout[FILENAME_MAX+1];
	char buffer[1024];
	char verbuffer[MAX_PATH];
	char numstring[16];
	char *findptr;
	char *findptr2;
	FILE *filein;
	FILE *fileout;
	DXGLVER version;
	//int revision = GetSVNRev(path);
	BOOL nosign = FALSE;
	version.build = 0;
	version.major = 0;
	version.minor = 5;
	version.point = 23;
	if (!GetGitVersion(path, &version)) ParseVersion(&version, TRUE);
	else ParseVersion(&version, TRUE);
	if (SIGNMODE < 1) nosign = TRUE;
	if ((SIGNMODE == 1) && version.beta) nosign = TRUE;
	#ifdef _DEBUG
	if (SIGNMODE <= 2) nosign = TRUE;
	#endif
	strncpy(pathin,path,FILENAME_MAX);
	pathin[FILENAME_MAX] = 0;
	strncpy(pathout,path,FILENAME_MAX);
	pathout[FILENAME_MAX] = 0;
	strncat(pathin,"\\version.h.in",FILENAME_MAX-strlen(pathin));
	pathin[FILENAME_MAX] = 0;
	strncat(pathout,"\\version.h",FILENAME_MAX-strlen(pathin));
	pathout[FILENAME_MAX] = 0;
	filein = fopen(pathin,"r");
	if(!filein)
	{
		fputs("ERROR:  Cannot read file ", stdout);
		puts(pathin);
		return errno;
	}
	fileout = fopen(pathout,"w");
	if(!fileout)
	{
		fputs("ERROR:  Cannot create file ", stdout);
		puts(pathin);
		return errno;
	}
	while(fgets(buffer,1024,filein))
	{
		findptr = strstr(buffer,"$MAJOR");
		if (findptr)
		{
			_itoa(version.major, verbuffer, 10);
			strcat(verbuffer, "\n");
			strncpy(findptr, verbuffer, 9);
		}
		findptr = strstr(buffer,"$MINOR");
		if (findptr)
		{
			_itoa(version.minor, verbuffer, 10);
			strcat(verbuffer, "\n");
			strncpy(findptr, verbuffer, 9);
		}
		findptr = strstr(buffer,"$POINT");
		if (findptr)
		{
			_itoa(version.point, verbuffer, 10);
			strcat(verbuffer, "\n");
			strncpy(findptr, verbuffer, 9);
		}
		findptr = strstr(buffer,"$BUILD");
		if(findptr)
		{
			_itoa(version.build,verbuffer,10);
			strcat(verbuffer,"\n");
			strncpy(findptr,verbuffer,9);
		}
		findptr = strstr(buffer, "$BRANCH");
		if (findptr)
		{
			strcpy(findptr, version.branch);
			strcat(findptr, "\n");
		}
		findptr = strstr(buffer, "$REVISION");
		if (findptr)
		{
			strcpy(findptr, version.revision);
			strcat(findptr, "\n");
		}
		findptr = strstr(buffer,"$VERSTRING");
		if(findptr)
		{
			if (version.beta)
			{
				strcpy(verbuffer, "\"");
				strcat(verbuffer, version.verstring);
				strcat(verbuffer, "\"\n");
				strcpy(findptr, verbuffer);
			}
			else
			{
				strcpy(verbuffer, "\"");
				_itoa(version.major, numstring, 10);
				strcat(verbuffer, numstring);
				strcat(verbuffer, ".");
				_itoa(version.minor, numstring, 10);
				strcat(verbuffer, numstring);
				strcat(verbuffer, ".");
				_itoa(version.point, numstring, 10);
				strcat(verbuffer, numstring);
				strcat(verbuffer, "\"\n");
				strcpy(findptr, verbuffer);
			}
		}
		if (version.beta)
		{
			if (strstr(buffer, "//#define DXGLBETA")) strcpy(buffer, "#define DXGLBETA");
		}
		fputs(buffer,fileout);
	}
	fclose(filein);
	filein = NULL;
	fclose(fileout);
	fileout = NULL;
	strncpy(pathin,path,FILENAME_MAX);
	pathin[FILENAME_MAX] = 0;
	strncpy(pathout,path,FILENAME_MAX);
	pathout[FILENAME_MAX] = 0;
	strncat(pathin,"\\version.nsh.in",FILENAME_MAX-strlen(pathin));
	pathin[FILENAME_MAX] = 0;
	strncat(pathout,"\\version.nsh",FILENAME_MAX-strlen(pathin));
	pathout[FILENAME_MAX] = 0;
	filein = fopen(pathin,"r");
	if(!filein)
	{
		fputs("ERROR:  Cannot read file ", stdout);
		puts(pathin);
		return errno;
	}
	fileout = fopen(pathout,"w");
	if(!fileout)
	{
		fputs("ERROR:  Cannot create file ", stdout);
		puts(pathin);
		return errno;
	}
	while(fgets(buffer,1024,filein))
	{
		findptr = strstr(buffer, "$PRODUCTVERNUMBER");
		if (findptr)
		{
			strcpy(verbuffer, "\"");
			itoa(version.major, numstring, 10);
			strcat(verbuffer, numstring);
			strcat(verbuffer, ".");
			itoa(version.minor, numstring, 10);
			strcat(verbuffer, numstring);
			strcat(verbuffer, ".");
			itoa(version.point, numstring, 10);
			strcat(verbuffer, numstring);
			strcat(verbuffer, ".");
			itoa(version.build, numstring, 10);
			strcat(verbuffer, numstring);
			strcat(verbuffer, "\"\n");
			strcpy(findptr, verbuffer);
		}
		findptr = strstr(buffer,"$PRODUCTVERSTRING");
		if (findptr)
		{
			if (version.beta)
			{
				strcpy(verbuffer, "\"");
				strcat(verbuffer, version.verstring);
				strcat(verbuffer, "\"\n");
				strcpy(findptr, verbuffer);
			}
			else
			{
				strcpy(verbuffer, "\"");
				_itoa(version.major, numstring, 10);
				strcat(verbuffer, numstring);
				strcat(verbuffer, ".");
				_itoa(version.minor, numstring, 10);
				strcat(verbuffer, numstring);
				strcat(verbuffer, ".");
				_itoa(version.point, numstring, 10);
				strcat(verbuffer, numstring);
				strcat(verbuffer, "\"\n");
				strcpy(findptr, verbuffer);
			}
		}
		findptr = strstr(buffer, "$COMPILERTYPE");
		if(findptr)
		{
			#ifdef _MSC_VER
			#if (_MSC_VER == 1400)
			strncpy(findptr, "\"VC2005\"\n", 13);
			#elif (_MSC_VER == 1500)
			strncpy(findptr, "\"VC2008\"\n", 13);
			#elif (_MSC_VER == 1600)
			strncpy(findptr, "\"VC2010\"\n", 13);
			#elif (_MSC_VER == 1800)
			strncpy(findptr, "\"VC2013\"\n", 13);
			#elif (_MSC_VER == 1941)
			strncpy(findptr, "\"VC2022_11\"\n", 13);
			#elif ((_MSC_VER >= 1930) && (_MSC_VER < 1941))
			#error Please update your Visual Studio 2022 to Update 11 before continuing.  If you have an expired MSDN subscription and cannot update your paid version of Visual Studio, you can still use the Community version to compile DXGL.
			#error If this error persists after updating Visual Studio, try uninstalling older MSVC toolchains.
			#elif (_MSC_VER > 1941)
			#pragma message ("Detected a newer version of Visual Studio, compiling assuming 2022.11.")
			strncpy(findptr, "\"VC2022_11\"\n", 13);
			#else
			#pragma message ("Can't detect MSVC version!")
			strncpy(findptr, "\"UNKNOWN\"\n", 13);
			#endif
			#endif
		}
		findptr = strstr(buffer, "$SIGNTOOL");
		if (findptr)
		{
			if (nosign) strncpy(findptr, "\"0\"\n", 10);
			else strncpy(findptr, "\"1\"\n", 10);
		}
		if (version.beta)
		{
			if(strstr(buffer,";!define _BETA")) strcpy(buffer,"!define _BETA\n");
		}
#ifdef _DEBUG
		if(strstr(buffer,";!define _DEBUG")) strcpy(buffer,"!define _DEBUG\n");
#endif
#ifdef _M_X64
		if (strstr(buffer, ";!define _CPU_X64")) strcpy(buffer, "!define _CPU_X64\n");
#endif
		fputs(buffer,fileout);
	}
	fclose(filein);
	filein = NULL;
	fclose(fileout);
	fileout = NULL;
	return 0;
}

void ParseHTMLFile(const TCHAR *filein, const TCHAR *fileout, const TCHAR *filetemplate)
{
	FILE *in;
	FILE *out;
	FILE *template;
	FILE *title;
	TCHAR filetitle[MAX_PATH * 2];
	TCHAR *ptr;
	char buffer[32768];
	in = _tfopen(filein, _T("r"));
	if (!in) return;
	template = _tfopen(filetemplate, _T("r"));
	if (!template)
	{
		fclose(in);
		return;
	}
	out = _tfopen(fileout, _T("w"));
	if (!out)
	{
		fclose(in);
		fclose(template);
		return;
	}
	_tcscpy(filetitle, filein);
	ptr = _tcsrchr(filetitle, _T('b'));
	*ptr = 0;
	_tcscat(filetitle, _T("title"));
	title = _tfopen(filetitle, _T("r"));
	fputs("Procesing help file ", stdout);
	_putts(filein);
	while (fgets(buffer, 32768, template))
	{
		if (!strncmp(buffer, "$HTMLCONTENT", 12))
		{
			rewind(in);
			while (fgets(buffer, 32768, in))
			{
				fputs(buffer, out);
			}
		}
		else if (!strncmp(buffer, "$HTMLTITLE", 10))
		{
			rewind(title);
			if (title)
			{
				fgets(buffer, 32768, title);
				fputs(buffer, out);
				fputs("\r\n", out);
			}
			else fputs("Unknown title\r\n", out);
		}
		else
		{
			fputs(buffer, out);
		}
	}
	fclose(in);
	fclose(template);
	fclose(out);
	if (title) fclose(title);
}

int ProcessHTMLFiles(const TCHAR *path, const TCHAR *templatepath)
{
	WIN32_FIND_DATA finddata;
	HANDLE hFind = NULL;
	TCHAR findpath[MAX_PATH+1];
	TCHAR filename[MAX_PATH * 2];
	TCHAR fileout[MAX_PATH * 2];
	TCHAR *ptr;
	_tcsncpy(findpath, path, MAX_PATH+1);
	_tcscat(findpath, _T("*.*"));
	hFind = FindFirstFile(findpath, &finddata);
	if (hFind == INVALID_HANDLE_VALUE) return 1;
	do
	{
		if (!_tcscmp(finddata.cFileName, _T("."))) continue;
		if (!_tcscmp(finddata.cFileName, _T(".."))) continue;
		_tcscpy(filename, path);
		_tcscat(filename, finddata.cFileName);
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			_tcscat(filename, _T("\\"));
			if (ProcessHTMLFiles(filename, templatepath))
			{
				FindClose(hFind);
				return 1;
			}
			continue;
		}
		ptr = _tcsrchr(filename, _T('.'));
		if (ptr)
		{
			if (!_tcscmp(ptr, _T(".htmlbody")))
			{
				_tcscpy(fileout, filename);
				ptr = _tcsrchr(fileout, _T('b'));
				*ptr = 0;
				ParseHTMLFile(filename, fileout, templatepath);
			}
		}
	} while (FindNextFile(hFind, &finddata));
	FindClose(hFind);
	return 0;
}

int MakeHelp(char *path)
{
	HKEY hKey;
	BOOL foundhhc = FALSE;
	char hhcpath[(MAX_PATH + 1) * 2];
	DWORD buffersize = MAX_PATH + 1;
	PROCESS_INFORMATION process;
	STARTUPINFOA startinfo;
	char htmlpath[MAX_PATH + 1];
	TCHAR htmlpath2[MAX_PATH + 1];
	char templatepath[MAX_PATH + 1];
	TCHAR templatepath2[MAX_PATH + 1];
	strcpy(htmlpath, path);
	if (strrchr(htmlpath, '\\')) *(strrchr(htmlpath, '\\')) = 0;
	strcpy(templatepath, htmlpath);
	strcat(htmlpath, "\\html\\");
	strcat(templatepath, "\\template.html");
#ifdef _UNICODE
	MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED, htmlpath, -1, htmlpath2, MAX_PATH + 1);
	MultiByteToWideChar(CP_THREAD_ACP, MB_PRECOMPOSED, templatepath, -1, templatepath2, MAX_PATH + 1);
#else
	strncpy(htmlpath2, htmlpath, MAX_PATH + 1);
	strncpy(templatepath2, templatepath, MAX_PATH + 1);
#endif
	if (ProcessHTMLFiles(htmlpath2, templatepath2))
	{
		puts("Error processing HTML files.");
		return -1;
	}
	#ifdef _M_X64
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\HTML Help Workshop", 0, KEY_READ|KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
	#else
	if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\HTML Help Workshop", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	#endif
	{
		if(RegQueryValueExA(hKey,"InstallDir",NULL,NULL,(LPBYTE)hhcpath,&buffersize) == ERROR_SUCCESS)
		{
			strcat(hhcpath,"\\hhc.exe");
			ZeroMemory(&startinfo,sizeof(STARTUPINFOA));
			startinfo.cb = sizeof(STARTUPINFOA);
			strcat(hhcpath," ");
			strncat(hhcpath,path,MAX_PATH);
			if(CreateProcessA(NULL,hhcpath,NULL,NULL,FALSE,0,NULL,NULL,&startinfo,&process))
			{
				foundhhc = TRUE;
				WaitForSingleObject(process.hProcess,INFINITE);
				CloseHandle(process.hProcess);
				CloseHandle(process.hThread);
			}
		}
		RegCloseKey(hKey);
	}
	if(!foundhhc)
	{
		int result = MessageBoxA(NULL,"Could not find HTML Help Workshop, would you like to download it?\n\nThis will download from Internet Archive as the official download site was closed.","HTML Help Workshop not found",
			MB_YESNO|MB_ICONERROR);
		if(result == IDYES) ShellExecuteA(NULL,"open", "https://web.archive.org/web/20200918004813/https://download.microsoft.com/download/0/A/9/0A939EF6-E31C-430F-A3DF-DFAE7960D564/htmlhelp.exe"
			,NULL,NULL,SW_SHOWNORMAL);
		puts("ERROR:  HTML Help Compiler not found.");
		return -1;
	}
	return 0;
}

int MakeInstaller(char *path)
{
	HKEY hKey;
	BOOL foundnsis = FALSE;
	char nsispath[(MAX_PATH+1)*2];
	DWORD buffersize = MAX_PATH+1;
	PROCESS_INFORMATION process;
	STARTUPINFOA startinfo;
	#ifdef _M_X64
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\NSIS", 0, KEY_READ|KEY_WOW64_32KEY, &hKey) == ERROR_SUCCESS)
	#else
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\NSIS", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	#endif
	{
		if(RegQueryValueExA(hKey,"",NULL,NULL,(LPBYTE)nsispath,&buffersize) == ERROR_SUCCESS)
		{
			strcat(nsispath,"\\makensis.exe");
			ZeroMemory(&startinfo,sizeof(STARTUPINFOA));
			startinfo.cb = sizeof(STARTUPINFOA);
			strcat(nsispath," ");
			strncat(nsispath,path,MAX_PATH);
			if(CreateProcessA(NULL,nsispath,NULL,NULL,FALSE,0,NULL,NULL,&startinfo,&process))
			{
				foundnsis = TRUE;
				WaitForSingleObject(process.hProcess,INFINITE);
				CloseHandle(process.hProcess);
				CloseHandle(process.hThread);
			}
		}
		RegCloseKey(hKey);
	}
	if(!foundnsis)
	{
		int result = MessageBoxA(NULL,"Could not find NSIS, would you like to download it?","NSIS not found",
			MB_YESNO|MB_ICONERROR);
		if(result == IDYES) ShellExecuteA(NULL,"open","https://nsis.sourceforge.io/Main_Page",NULL,NULL,SW_SHOWNORMAL);
		puts("ERROR:  NSIS not found.");
		return -1;
	}
	return 0;
}

int SignEXE(char *exefile, char *path)
{
	PROCESS_INFORMATION process;
	STARTUPINFOA startinfo;
	const char signtoolsha384path[] = "signtool sign /tr http://timestamp.sectigo.com /td sha384 /fd sha384 /as ";
	char signpath[MAX_PATH + 80];
	DXGLVER version;
	BOOL nosign = FALSE;
	version.build = 0;
	version.major = 0;
	version.minor = 5;
	version.point = 20;
	if (!GetGitVersion(path, &version)) ParseVersion(&version, TRUE);
	else ParseVersion(&version, TRUE);
	if (SIGNMODE < 1) nosign = TRUE;
	if ((SIGNMODE == 1) && version.beta) nosign = TRUE;
	#ifdef _DEBUG
		if (SIGNMODE <= 2) nosign = TRUE;
	#endif
	if (nosign)
	{
		puts("Skipping file signature.");
		return 0;
	}
#if (_MSC_VER >= 1920)
	strcpy(&signpath, &signtoolsha384path);
	strcat(&signpath, "\"");
	strncat(&signpath, exefile, MAX_PATH);
	strcat(&signpath, "\"");
	ZeroMemory(&startinfo, sizeof(STARTUPINFOA));
	startinfo.cb = sizeof(STARTUPINFOA);
	if (CreateProcessA(NULL, &signpath, NULL, NULL, FALSE, 0, NULL, NULL, &startinfo, &process))
	{
		WaitForSingleObject(process.hProcess, INFINITE);
		CloseHandle(process.hProcess);
		CloseHandle(process.hThread);
		Sleep(15000);
	}
#endif
	return 0;
}

int main(int argc, char *argv[])
{
	puts("DXGL Build Tool");
#ifdef _DEBUG
	puts("Debug version.");
#endif
	if(argc > 1)
	{
		if(!strcmp(argv[1],"makeheader"))
		{
			if(argc < 3)
			{
				puts("ERROR:  No working directory specified.");
				return 1;
			}
			return ProcessHeaders(argv[2]);
		}
		if(!strcmp(argv[1],"makehelp"))
		{
			if(argc < 3)
			{
				puts("ERROR:  No working directory specified.");
				return 1;
			}
			return MakeHelp(argv[2]);
		}
		if(!strcmp(argv[1],"makeinstaller"))
		{
			if(argc < 3)
			{
				puts("ERROR:  No working directory specified.");
				return 1;
			}
			return MakeInstaller(argv[2]);
		}
		if (!strcmp(argv[1], "sign"))
		{
			if (argc < 3)
			{
				puts("ERROR: No file specified.");
				return 1;
			}
			else if (argc < 4)
			{
				puts("ERROR:  No working directory specified.");
				return 1;
			}
			return SignEXE(argv[2], argv[3]);
		}
	}
	else
	{
	}
    return 0;
}
