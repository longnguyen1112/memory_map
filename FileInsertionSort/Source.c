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
void swap(RECORD* array1, RECORD* array2)
{
	RECORD t = *array1;
	*array1 = *array2;
	*array2 = t;
}

int partition(int low, int high, RECORD outfile[])
{
	RECORD pivot = outfile[high];
	int i = (low - 1);

	for (int j = low; j <= high - 1; j++)
	{
		int recIdx = 0;
		while ((int)outfile[j].key[recIdx] == (int)pivot.key[recIdx] && recIdx < 8)
		{
			recIdx++;
		}
		if ((int)outfile[j].key[recIdx] < (int)pivot.key[recIdx])
		{
			i++;
			swap(&outfile[i], &outfile[j]);
		}
	}
	swap(&outfile[i + 1], &outfile[high]);
	return (i + 1);
}

void quicksort(int low, int high, RECORD outfile[])
{
	if (low < high)
	{

		int pi = partition(low, high, outfile);

		quicksort(low, pi - 1, outfile);
		quicksort(pi + 1, high, outfile);
	}
}

void mergeTwoFiles(RECORD from[], RECORD to[],int start, int firstStop, int end)
{
	int num_of_Litem, num_of_Ritem;
	int l = start;
	int r = firstStop;
	int outputIndex = start;
	while (l < firstStop && r < end)
	{
		int recIdx = 0;
		while ((int)from[l].key[recIdx] == (int)from[r].key[recIdx] && recIdx < 8)
		{
			recIdx++;
		}
		if ((int)from[l].key[recIdx] < (int)from[r].key[recIdx])
		{
			to[outputIndex]=from[l];
			l++; outputIndex++;
		}
		else
		{
			to[outputIndex]=from[r];
			r++; outputIndex++;
		}
	}

	//the rest of the record go into the write
	if (l == firstStop) //which mean there is still record left on the right
	{
		while(r<end)
		{
			to[outputIndex] = from[r];
			outputIndex++;
			r++;
		}
	}
	else
	{
		while (l < firstStop)
		{
			to[outputIndex] = from[l];
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
	HANDLE hDest = NULL;
	HANDLE hSource = NULL;
	HANDLE hOut = NULL;
	HANDLE fOut = NULL;
	HANDLE hIn = NULL;

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
	if (processes == 0 || argc==1) {
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
	//hSource = CreateFileMapping(STDInput, &stdOutSA, PAGE_READONLY, 0, 0, NULL);

	if (processes == 1)
	{
		HANDLE hDestination;
		HANDLE hSource;
		int offset;
		int size;
		if (argc == 6)
		{
			hDestination = atoi(argv[2]);
			hSource = atoi(argv[3]);
			offset = atoi(argv[4]);
			size = atoi(argv[5]);
 			RECORD* destMap = MapViewOfFile(hDestination, FILE_MAP_WRITE, 0, 0, FileSize.QuadPart);
			quicksort(offset, size - 1, destMap);
		}
		else
		{
			hSource = CreateFileMapping(STDInput, &stdOutSA, PAGE_READONLY, 0, 0, NULL);
			hDestination = CreateFileMapping(STDOutput, &stdOutSA, PAGE_READWRITE, FileSize.HighPart, FileSize.LowPart, NULL);
			offset = 0;
			size = (FileSize.QuadPart) / 64;
			RECORD* pInFile = MapViewOfFile(hSource, FILE_MAP_READ, 0, 0, FileSize.QuadPart);
			DWORD e = GetLastError();
			RECORD* pOutFile = MapViewOfFile(hDestination, FILE_MAP_WRITE, 0, 0, FileSize.QuadPart);
			e = GetLastError();
			for (int x = 0; x <= size; x++)
			{
				pOutFile[x] = pInFile[x];
			}
			quicksort(0, size - 1, pOutFile);
			UnmapViewOfFile(pInFile);
			UnmapViewOfFile(pOutFile);
			CloseHandle(hSource);
			CloseHandle(hDestination);
		}
	}
	else
	{
		hProc = malloc(processes * sizeof(HANDLE));
		GetStartupInfo(&startUpSearch);
		GetStartupInfo(&startUp);
		if (argc == 2)
		{
			fOut = CreateFile("tempfile.txt",
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, &stdOutSA,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			hSource = CreateFileMapping(fOut, &stdOutSA, PAGE_READWRITE, FileSize.HighPart, FileSize.LowPart, NULL);
			hDest = CreateFileMapping(STDOutput, &stdOutSA, PAGE_READWRITE, FileSize.HighPart, FileSize.LowPart, NULL);
			hIn = CreateFileMapping(STDInput, &stdOutSA, PAGE_READONLY, 0, 0, NULL);
			RECORD* pInFile = MapViewOfFile(hIn, FILE_MAP_READ, 0, 0, FileSize.QuadPart);
			RECORD* pDest = MapViewOfFile(hDest, FILE_MAP_WRITE, 0, 0, FileSize.QuadPart);
			RECORD* pSource = MapViewOfFile(hSource, FILE_MAP_WRITE, 0, 0, FileSize.QuadPart);
			int low = 0;
			int high = (FileSize.QuadPart) / 64;
			int mid = (low + high) / 2;
			processes /= 2;
			for (int i = 0; i < high; i++)
			{
				pDest[i] = pInFile[i];
				pSource[i] = pInFile[i];
			}

			//loop of 2
			int tempLow = low;
			int tempMid = mid;
			for (int i = 0; i < 2; i++)
			{
				sprintf(commandLine, _T("FileInsertionSort %d %d %d %d %d"), processes, hSource, hDest, tempLow, tempMid);
				if (!CreateProcess(NULL, commandLine, NULL, NULL,
					TRUE, 0, NULL, NULL, &startUpSearch, &processInfo))
					printf("ProcCreate failed.");

				hProc[i] = processInfo.hProcess;
				tempLow = tempMid;
				tempMid = high;

			}
			WaitForSingleObject(hProc[0], INFINITE);
			WaitForSingleObject(hProc[1], INFINITE);

			//write on the tempfile
			mergeTwoFiles(pSource, pDest, low, mid, high);
			UnmapViewOfFile(pDest);
			UnmapViewOfFile(pSource);
			CloseHandle(hSource);
			CloseHandle(fOut);
			CloseHandle(hDest);
			free(hProc);

			if (!DeleteFile("tempfile.txt"))
			{
				int e = GetLastError();
				if (e == ERROR_ACCESS_DENIED)
				{
					printf("Did not delete");
				}
			}
		}
		else
		{
			hDest = atoi(argv[3]);
			hSource = atoi(argv[2]);
			RECORD* pDest = MapViewOfFile(hDest, FILE_MAP_WRITE, 0, 0, FileSize.QuadPart);
			RECORD* pSource = MapViewOfFile(hSource, FILE_MAP_WRITE, 0, 0, FileSize.QuadPart);
			int low = atoi(argv[4]);
			int high = atoi(argv[5]);
			int mid = (low + high) / 2;
			processes /= 2;
			int tempLow = low;
			int tempMid = mid;
			for (int i = 0; i < 2; i++)
			{
				sprintf(commandLine, _T("FileInsertionSort %d %d %d %d %d"), processes, hDest, hSource, tempLow, tempMid);
				
				if (!CreateProcess(NULL, commandLine, NULL, NULL,
					TRUE, 0, NULL, NULL, &startUpSearch, &processInfo))
					printf("ProcCreate failed.");

				hProc[i] = processInfo.hProcess;
				tempLow = tempMid;
				tempMid = high;

			}
			WaitForSingleObject(hProc[0], INFINITE);
			WaitForSingleObject(hProc[1], INFINITE);
			mergeTwoFiles(pDest, pSource, low, mid, high);
			UnmapViewOfFile(pSource);
			UnmapViewOfFile(pDest);
			CloseHandle(hSource);
			CloseHandle(fOut);
			CloseHandle(hDest);
			free(hProc);

			
		}
	}


	return 0;
}