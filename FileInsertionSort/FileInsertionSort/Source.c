#define WIN32_LEAN_AND_MEAN 

#define NOATOM
#define NOCLIPBOARD
#define NOCOMM
#define NOCTLMGR
#define NOCOLOR
#define NODEFERWINDOWPOS
#define NODESKTOP
#define NODRAWTEXT
#define NOEXTAPI
#define NOGDICAPMASKS
#define NOHELP
#define NOICONS
#define NOTIME
#define NOIMM
#define NOKANJI
#define NOKERNEL
#define NOKEYSTATES
#define NOMCX
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMSG
#define NONCMESSAGES
#define NOPROFILER
#define NORASTEROPS
#define NORESOURCE
#define NOSCROLL
//#define NOSERVICE		/* Windows NT Services */
#define NOSHOWWINDOW
#define NOSOUND
#define NOSYSCOMMANDS
#define NOSYSMETRICS
#define NOSYSPARAMS
#define NOTEXTMETRIC
#define NOVIRTUALKEYCODES
#define NOWH
#define NOWINDOWSTATION
#define NOWINMESSAGES
#define NOWINOFFSETS
#define NOWINSTYLES
#define OEMRESOURCE
#pragma warning(disable : 4996)

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>
#include <WinSock2.h>
#include <math.h>

#if !defined(_Wp64)
#define DWORD_PTR DWORD
#define LONG_PTR LONG
#define INT_PTR INT
#endif

#define DATALEN 56
#define KEYLEN 8
typedef struct _RECORD {
	TCHAR key[KEYLEN];
	TCHAR data[DATALEN];
} RECORD;

#define RECSIZE sizeof (RECORD)

RECORD ReadRecord(HANDLE STDInput, int spot_o) {
	DWORD BIn;
	RECORD temp;
	LARGE_INTEGER spot;
	spot.QuadPart = spot_o * 64;
	SetFilePointerEx(STDInput, spot, NULL, FILE_BEGIN);
	ReadFile(STDInput, &temp, RECSIZE, &BIn, NULL);
	return temp;
}

void WriteRecord(HANDLE STDOutput, int spot_o, RECORD to_write) {
	DWORD Bout;

	LARGE_INTEGER spot;
	spot.QuadPart = spot_o * 64;
	SetFilePointerEx(STDOutput, spot, NULL, FILE_BEGIN);
	WriteFile(STDOutput, &to_write, RECSIZE, &Bout, NULL);
	//return temp;
}

//quick sort

int partition(int low, int high, HANDLE outfile)
{
	RECORD pivot = ReadRecord(outfile, high);
	int i = (low - 1);

	for (int j = low; j <= high - 1; j++)
	{
		RECORD jItem = ReadRecord(outfile, j);
		//fix it here jitem=read record
		int recIdx = 0;
		while ((int)jItem.key[recIdx] == (int)pivot.key[recIdx] && recIdx < 8)
		{
			recIdx++;
		}
		if ((int)jItem.key[recIdx] < (int)pivot.key[recIdx])
		{
			i++;
			RECORD iItem = ReadRecord(outfile, i);
			WriteRecord(outfile, i, jItem);
			WriteRecord(outfile, j, iItem);
		}
	}
	RECORD iItem = ReadRecord(outfile, i + 1);
	WriteRecord(outfile, i+1, pivot);
	WriteRecord(outfile, high, iItem);
	return (i + 1);
}

void quicksort(int low, int high, HANDLE outfile)
{
	if (low < high)
	{

		int pi = partition(low, high, outfile);

		quicksort(low, pi - 1, outfile);
		quicksort(pi + 1, high, outfile);
	}
}

void mergeTwoFiles(HANDLE leftout, HANDLE rightout, HANDLE result)
{
	int num_of_Litem, num_of_Ritem;
	LARGE_INTEGER largeL, largeR;
	int l = 0;
	int r = 0;
	GetFileSizeEx(leftout, &largeL);
	GetFileSizeEx(rightout, &largeR);
	num_of_Litem = largeL.QuadPart/64;
	num_of_Ritem = largeR.QuadPart/64;
	int outputIndex = 0;
	while (l < num_of_Litem && r < num_of_Ritem)
	{
		RECORD lRecord = ReadRecord(leftout, l);
		RECORD rRecord = ReadRecord(rightout, r);
		int recIdx = 0;
		while ((int)lRecord.key[recIdx] == (int)rRecord.key[recIdx] && recIdx < 8)
		{
			recIdx++;
		}
		if ((int)lRecord.key[recIdx] < (int)rRecord.key[recIdx])
		{
			WriteRecord(result, outputIndex, lRecord);
			l++; outputIndex++;
		}
		else
		{
			WriteRecord(result, outputIndex, rRecord);
			r++; outputIndex++;
		}
	}

	//the rest of the record go into the write
	if (l == num_of_Litem) //which mean there is still record left on the right
	{
		while(r<num_of_Ritem)
		{
			RECORD rLeftover = ReadRecord(rightout, r);
			WriteRecord(result, outputIndex, rLeftover);
			outputIndex++;
			r++;
		}
	}
	else
	{
		while (l < num_of_Litem)
		{
			RECORD lLeftover = ReadRecord(leftout, l);
			WriteRecord(result, outputIndex, lLeftover);
			outputIndex++;
			l++;
		}
	}
	
}


int _tmain(int argc, LPTSTR argv[])
{

	SECURITY_ATTRIBUTES stdOutSA = /* SA for inheritable handle. */
	{ sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

	
	RECORD data;
	HANDLE STDInput, STDOutput;
	LARGE_INTEGER FileSize;
	DWORD BIn, Bout;
	PROCESS_INFORMATION processInfo;
	STARTUPINFO startUpSearch, startUp;
	STDInput = GetStdHandle(STD_INPUT_HANDLE);
	STDOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	HANDLE* hProc;  /* Pointer to an array of proc handles. */
	HANDLE* hOutFiles;
	typedef struct { TCHAR tempFile[MAX_PATH]; } PROCFILE;
	PROCFILE* InFile; /* Pointer to array of temp file names. */
	PROCFILE* OutFile; /* Pointer to array of temp file names. */
	TCHAR commandLine[200]; //name of a command
	HANDLE hInTempFile, hOutTempFile;

	int processes = atoi(argv[1]);

	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	int numCPU = sysinfo.dwNumberOfProcessors;
	int numCPUMax;
	if (processes == 0) {
		for (int i = 0; i <= 20; i++) //you should not have more than 1 million processors computer
		{
			if (pow(2, i) >= numCPU) 
			{
				processes = pow(2, i);
				break;
			}
		}
	}
	
	GetFileSizeEx(STDInput, &FileSize);

	if (processes == 1)
	{
		LARGE_INTEGER spot;
		int size = FileSize.QuadPart / 64;


		for (int x = 0; x < size; x++)  // Fill the output file (You can't sort the input file!!!)
		{
			ReadFile(STDInput, &data, RECSIZE, &BIn, NULL);
			WriteFile(STDOutput, &data, RECSIZE, &Bout, NULL);
		}
		quicksort(0, size - 1, STDOutput);
	}
	else
	{
		InFile = malloc(processes * sizeof(PROCFILE)); //array of infile name
		OutFile = malloc(processes * sizeof(PROCFILE)); //array of outfile name
		hProc = malloc(processes * sizeof(HANDLE));	//array to proc handle
		hOutFiles = malloc(processes * sizeof(HANDLE));

		GetStartupInfo(&startUpSearch);
		GetStartupInfo(&startUp);

		processes /= 2;
		sprintf(commandLine,"FileInsertionSort %d",processes);

		for (int iProc = 0; iProc < 2; iProc++)
		{
			if (GetTempFileName(_T("."), _T("In"), 0, InFile[iProc].tempFile) == 0) //create in temp
			{
				printf("Error creating file name!");
				return 1;
			}

			if (GetTempFileName(_T("."), _T("Out"), 0, OutFile[iProc].tempFile) == 0) //create out temp
			{
				printf("Error creating file name!");
				return 1;
			}

			hInTempFile = /* This handle is inheritable */
				CreateFile(InFile[iProc].tempFile,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE, &stdOutSA,
					CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);


			hOutTempFile = /* This handle is inheritable */
				CreateFile(OutFile[iProc].tempFile,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE, &stdOutSA,
					CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			hOutFiles[iProc] = hOutTempFile; //store the output handle so we can comeback later

			int numitems = (FileSize.QuadPart) / 64 / 2; //numbers of items for each process

			if (((FileSize.QuadPart) / 64) % 2 != 0 && iProc == 1)
			{
				for (int x = 0; x <= numitems; x++) //read and write data in the temp file
				{
					ReadFile(STDInput, &data, RECSIZE, &BIn, NULL); //data is passed by reference
					WriteFile(hInTempFile, &data, RECSIZE, &Bout, NULL);
					//WriteFile(hOutTempFile, &data, RECSIZE, &Bout, NULL);
				}
			}
			else {
				for (int x = 0; x < numitems; x++) //read and write data in the temp file
				{
					ReadFile(STDInput, &data, RECSIZE, &BIn, NULL); //data is passed by reference
					WriteFile(hInTempFile, &data, RECSIZE, &Bout, NULL);
					//WriteFile(hOutTempFile, &data, RECSIZE, &Bout, NULL);
				}
			}
			
			LARGE_INTEGER spot;
			spot.QuadPart = 0;
			SetFilePointerEx(hInTempFile, spot, NULL, FILE_BEGIN);
			startUpSearch.dwFlags = STARTF_USESTDHANDLES;
			startUpSearch.hStdOutput = hOutTempFile;
			startUpSearch.hStdError = hOutTempFile;
			startUpSearch.hStdInput = hInTempFile;

			if (!CreateProcess(NULL, commandLine, NULL, NULL,
				TRUE, 0, NULL, NULL, &startUpSearch, &processInfo))
				printf("ProcCreate failed.");

			CloseHandle(hInTempFile); 
			hProc[iProc] = processInfo.hProcess;
		}

		//wait
		for (int i = 0; i < 2; i++)
		{
			WaitForSingleObject(hProc[i], INFINITE); //wait infinite
		}

		//merge into 1 file
		//this will be the std output where the 2 child will be merged into
		STDOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		mergeTwoFiles(hOutFiles[0], hOutFiles[1], STDOutput);

		for (int iProc = 1; iProc >= 0; iProc--) {
			LARGE_INTEGER spot;
			spot.QuadPart = 0;
			SetFilePointerEx(hOutFiles[iProc], spot, NULL, FILE_BEGIN);
			CloseHandle(hOutFiles[iProc]);

			if (!DeleteFile(InFile[iProc].tempFile))
			{
				printf("Cannot delete temp file1.");
				return 1;
			}
			if (!DeleteFile(OutFile[iProc].tempFile))
			{
				printf("Cannot delete temp file2.");
				return 1;
			}
		}

	}


	return 0;
}