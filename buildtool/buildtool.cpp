// DXGL
// Copyright (C) 2012 William Feely

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

#include <cstdio>
#include <iostream>
#include "../common/releasever.h"

using namespace std;

int ProcessHeaders(char *path)
{
	char pathin[FILENAME_MAX+1];
	char pathout[FILENAME_MAX+1];
	char buffer[1024];
	char verbuffer[20];
	char numstring[16];
	char *findptr;
	int revision = 0; //FIXME:  Get SVN Rev.
	strncpy(pathin,path,FILENAME_MAX);
	strncpy(pathout,path,FILENAME_MAX);
	strncat(pathin,"\\version.h.in",FILENAME_MAX-strlen(pathin));
	strncat(pathout,"\\version.h",FILENAME_MAX-strlen(pathin));
	FILE *filein = fopen(pathin,"r");
	if(!filein)
	{
		cout << "ERROR:  Cannot read file " << pathin << endl;
		return errno;
	}
	FILE *fileout = fopen(pathout,"w");
	if(!fileout)
	{
		cout << "ERROR:  Cannot create file " << pathin << endl;
		return errno;
	}
	while(fgets(buffer,1024,filein))
	{
		findptr = strstr(buffer,"$MAJOR");
		if(findptr) strncpy(findptr,STR(DXGLMAJORVER) "\n",6);
		findptr = strstr(buffer,"$MINOR");
		if(findptr) strncpy(findptr,STR(DXGLMINORVER) "\n",6);
		findptr = strstr(buffer,"$POINT");
		if(findptr) strncpy(findptr,STR(DXGLPOINTVER) "\n",6);
		findptr = strstr(buffer,"$REVISION");
		if(findptr)
		{
			_itoa(revision,verbuffer,10);
			strcat(verbuffer,"\n");
			strncpy(findptr,verbuffer,9);
		}
		findptr = strstr(buffer,"$VERSTRING");
		if(findptr)
		{
			if(revision)
			{
				strcpy(verbuffer, "\"");
				strcat(verbuffer,DXGLSTRVER);
				strcat(verbuffer," r");
				_itoa(revision,numstring,10);
				strcat(verbuffer,numstring);
				strcat(verbuffer,"\"");
				strncpy(findptr,verbuffer,22);
			}
			else strncpy(findptr,"\"" DXGLSTRVER "\"\n",15);
		}
		fputs(buffer,fileout);
	}
	fclose(filein);
	filein = NULL;
	fclose(fileout);
	fileout = NULL;
	strncpy(pathin,path,FILENAME_MAX);
	strncpy(pathout,path,FILENAME_MAX);
	strncat(pathin,"\\version.nsh.in",FILENAME_MAX-strlen(pathin));
	strncat(pathout,"\\version.nsh",FILENAME_MAX-strlen(pathin));
	filein = fopen(pathin,"r");
	if(!filein)
	{
		cout << "ERROR:  Cannot read file " << pathin << endl;
		return errno;
	}
	fileout = fopen(pathout,"w");
	if(!fileout)
	{
		cout << "ERROR:  Cannot create file " << pathin << endl;
		return errno;
	}
	while(fgets(buffer,1024,filein))
	{
		findptr = strstr(buffer,"$PRODUCTVERSTRING");
		if(findptr) strncpy(findptr,"\"" DXGLSTRVER "\"\n",15);
		findptr = strstr(buffer,"$PRODUCTREVISION");
		if(findptr)
		{
			if(revision)
			{
				strcpy(verbuffer,"\"r");
				_itoa(revision,numstring,10);
				strcat(verbuffer,numstring);
				strcat(verbuffer,"\"\n");
				strncpy(findptr,verbuffer,17);
			}
			else strncpy(findptr,"\"\"\n",17);
		}
#ifdef _DEBUG
		if(strstr(buffer,";!define DEBUG")) strcpy(buffer,"!define DEBUG");
#endif
		fputs(buffer,fileout);
	}
	fclose(filein);
	filein = NULL;
	fclose(fileout);
	fileout = NULL;
	return 0;
}

int main(int argc, char *argv[])
{

    cout << "DXGL Build Tool, version " << DXGLSTRVER << endl;
#ifdef _DEBUG
	cout << "Debug version." << endl;
#endif
	if(argc > 1)
	{
		if(!strcmp(argv[1],"makeheader"))
		{
			if(argc < 3)
			{
				cout << "ERROR:  No working directory specified." << endl;
				return 1;
			}
			ProcessHeaders(argv[2]);
			return 0;
		}
	}
	else
	{
	}
    return 0;
}
