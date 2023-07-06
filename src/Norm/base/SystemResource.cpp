// Copyright (c) 2023 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

#include "SystemResource.h"
#include "Portability.h"
#include "MemoryManager.h"

// Pour eviter les warning sur strcpy et sprintf
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <math.h>
#include <assert.h>

// Supression des gestionnaire de crash en Windows, qui ouvrent des boites de dialogue bloquante
// Methode interne, appelee uniquement par l'allocateur dans MemoryManager
extern void SuppressCrashHandlers();

// Following defines added to compute the MemGetAdressablePhysicalMemory values
#define SIZE_OF_SIZE_T_32_BITS_SYSTEM 4
#define SIZE_OF_SIZE_T_64_BITS_SYSTEM 8

///////////////////////////////////////////////////////////////////////////////////////////
// Implementation independante du systeme

longint MemGetPhysicalMemoryReserve()
{
	longint lPhysicalMemoryReserve;

	// Calcul de la taille reservee au systeme
	lPhysicalMemoryReserve = MemGetAvailablePhysicalMemory();
	if (lPhysicalMemoryReserve > MemGetAdressablePhysicalMemory())
		lPhysicalMemoryReserve = MemGetAdressablePhysicalMemory();
	lPhysicalMemoryReserve /= 16;
	if (lPhysicalMemoryReserve < 16 * lMB)
		lPhysicalMemoryReserve = 16 * lMB;
	if (lPhysicalMemoryReserve > 64 * lMB)
		lPhysicalMemoryReserve = 64 * lMB;
	return lPhysicalMemoryReserve;
}

///////////////////////////////////////////////////////////////////////////////////////////
// Implementation Windows
#ifdef _WIN32

#include <winsock2.h>
#include <iphlpapi.h>
#include <windows.h>
#include <string.h>
#include <psapi.h>

/* Rappel de la structure MEMORYSTATUS du header winbase.h
typedef struct _MEMORYSTATUS { // mst
DWORD dwLength;        // sizeof(MEMORYSTATUS)
DWORD dwMemoryLoad;    // percent of memory in use
DWORD dwTotalPhys;     // bytes of physical memory
DWORD dwAvailPhys;     // free physical memory bytes
DWORD dwTotalPageFile; // bytes of paging file
DWORD dwAvailPageFile; // free bytes of paging file
DWORD dwTotalVirtual;  // user bytes of address space
DWORD dwAvailVirtual;  // free user bytes
} MEMORYSTATUS, *LPMEMORYSTATUS;
*/

// Librairies necessaires: Winmm.lib et psapi.lib
#pragma comment(lib, "Winmm")
#pragma comment(lib, "psapi")

void SystemSleep(double dSeconds)
{
	int nMilliseconds;
	assert(dSeconds >= 0);
	assert(dSeconds < INT_MAX / 1000);

	nMilliseconds = int(floor(dSeconds * 1000.0 + 0.0005));
	timeBeginPeriod(1);
	Sleep(nMilliseconds);
	timeEndPeriod(1);
}

longint DiskGetFreeSpace(const char* sPathName)
{
	longint lFreeDiskSpace = 0;
	int nLength;
	WCHAR* pszPathName;
	int nError;
	unsigned __int64 lFreeBytesAvailable;
	unsigned __int64 lTotalNumberOfBytes;
	unsigned __int64 lTotalNumberOfFreeBytes;

	assert(sPathName != NULL);

	p_SetMachineLocale();
	// Passage en WCHAR
	nLength = (int)strlen(sPathName);
	pszPathName = (WCHAR*)SystemObject::NewMemoryBlock((nLength + 1) * sizeof(WCHAR));
	mbstowcs(pszPathName, sPathName, nLength + 1);

	// Appel de la routine Windows
	nError = GetDiskFreeSpaceEx(pszPathName, (PULARGE_INTEGER)&lFreeBytesAvailable,
				    (PULARGE_INTEGER)&lTotalNumberOfBytes, (PULARGE_INTEGER)&lTotalNumberOfFreeBytes);
	if (nError != 0)
		lFreeDiskSpace = lFreeBytesAvailable;

	// Nettoyage
	SystemObject::DeleteMemoryBlock(pszPathName);
	p_SetApplicationLocale();

	// Nettoyage de la chaine allouee
	assert(lFreeDiskSpace >= 0);
	return lFreeDiskSpace;
};

typedef BOOL(WINAPI* LPFN_GLPI)(PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);

typedef BOOL(WINAPI* LPFN_GLPI_EX)(LOGICAL_PROCESSOR_RELATIONSHIP, PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX, PDWORD);

int SystemGetProcessorNumber()
{
	// Adapte du site
	// https://msdn.microsoft.com/en-us/library/ms683194
	LPFN_GLPI_EX glpi_ex;
	LPFN_GLPI glpi;
	int processorCoreCount;
	SYSTEM_INFO sysinfo;
	int nDefaultProcessorCount;
	boolean bProcessorInformationAvailable;
	boolean bProcessorInformationExAvailable;
	DWORD len;
	DWORD rc;
	char* buffer = NULL;
	char* ptr = NULL;
	processorCoreCount = 0;
	len = 0;

	// Par defaut, on renvoie le nombre de coeurs logiques
	GetSystemInfo(&sysinfo);
	nDefaultProcessorCount = (int)sysinfo.dwNumberOfProcessors;

	glpi = (LPFN_GLPI)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation");
	glpi_ex = (LPFN_GLPI_EX)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformationEx");
	bProcessorInformationAvailable = glpi != NULL;
	bProcessorInformationExAvailable = glpi_ex != NULL;

	// Utilisation de la methode GetLogicalProcessorInformationEx si elle est disponible
	if (bProcessorInformationExAvailable)
	{
		// Appel de la methode pour avoir la taille de la structure
		rc = glpi_ex(RelationAll, NULL, &len);
		assert(rc == false);
		assert(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
		assert(len > 0);

		// Allocation du buffer a la bonne taille
		buffer = SystemObject::NewCharArray(len);

		// Appel de la methode avec la bonne taille de structure
		rc = glpi_ex(RelationAll, (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)buffer, &len);
		if (rc == false)
			return nDefaultProcessorCount;

		// Parcours de la structure en se deplacant dans le buffer par sauts dont la taille est donnee par la
		// structure
		ptr = buffer;
		while (ptr < buffer + len)
		{
			PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX pi = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ptr;
			if (pi->Relationship == RelationProcessorCore)
				processorCoreCount++;
			ptr += pi->Size;
		}
		SystemObject::DeleteCharArray(buffer);
		return processorCoreCount;
	}

	// Utilisation de la methode GetLogicalProcessorInformation si GetLogicalProcessorInformationEx n'est pas
	// disponible
	if (bProcessorInformationAvailable)
	{
		// Appel de la methode pour avoir la taille de la structure
		rc = glpi(NULL, &len);
		assert(rc == false);
		assert(GetLastError() == ERROR_INSUFFICIENT_BUFFER);
		assert(len > 0);

		// Allocation du buffer a la bonne taille
		buffer = SystemObject::NewCharArray(len);

		// Appel de la methode avec la bonne taille de structure
		rc = glpi((PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)buffer, &len);
		if (rc == false)
			return nDefaultProcessorCount;

		// Parcours de la structure en se deplacant dans le buffer par sauts de taille fixe
		ptr = buffer;
		while (ptr < buffer + len)
		{
			PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pi = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)ptr;
			if (pi->Relationship == RelationProcessorCore)
				processorCoreCount++;
			ptr += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
		}
		SystemObject::DeleteCharArray(buffer);
		return processorCoreCount;
	}

	return nDefaultProcessorCount;
}

const char* CurrentPreciseTimestamp()
{
	char* sBuffer = StandardGetBuffer();
	SYSTEMTIME st;

	GetLocalTime(&st);
	sprintf_s(sBuffer, 30, "%04d-%02d-%02d %02d:%02d:%02d.%03d%c", st.wYear, st.wMonth, st.wDay, st.wHour,
		  st.wMinute, st.wSecond, st.wMilliseconds, '\0');
	return sBuffer;
}

const char* CurrentTimestamp()
{
	char* sBuffer = StandardGetBuffer();
	SYSTEMTIME st;

	GetLocalTime(&st);
	sprintf_s(sBuffer, 30, "%04d-%02d-%02d %02d:%02d:%02d%c", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute,
		  st.wSecond, '\0');
	return sBuffer;
}

longint MemGetAdressablePhysicalMemory()
{
	// On utilise une variable locale pour eviter le warning "conditional expression is constant"
	int nSizeOfSizeT = sizeof(size_t);

	if (nSizeOfSizeT == SIZE_OF_SIZE_T_32_BITS_SYSTEM)
	{
		// Windows limite la memoire utilisable a 2 Go par process en 32 bits
		// 2^(sizeof(size_t)*8-20))/2
		return 2 * lGB - 128 * lMB;
	}
	else
	{
		// Bien que l'espace theoriquement adressable soit sur 64 bits, les limites actuelles
		// paraissent tres differentes selon les systemes: (ref:
		// http://www.presence-pc.com/actualite/windows-4go-ram-64bits-33121/)
		//   Contrairement aux idees recues (encore), un processeur 64 bits ne gere pas sa memoire sur 64 bits,
		//   du moins pas actuellement et pas en x86. En theorie, en 64 bits, il est possible de gerer 16
		//   Exaoctets de memoire, mais les processeurs ne gerent pas la memoire sur 64 bits. Chez AMD, avec le
		//   K8 et le K10, l'adressage s'effectue sur 40 bits (1 To de memoire) alors qu'Intel se limite a 36
		//   bits sur les Core 2 Duo (38 ou 40 bits dans les serveurs). Sur le Core i7, Intel travaille en 40
		//   bits (1 To). Notons enfin que la memoire virtuelle est geree en 48 bits (256 To).
		// Le choix de 40 bits d'addresse (2^(40-20)) est un compromis acceptable (1 To de memoire adressable)
		return lTB;
	}
}

longint MemGetAvailablePhysicalMemory()
{
	MEMORYSTATUS memstat;

	// L'utilisation conseillee GlobalMemoryStatusEx n'a pas ete concluante
	// On renvoie en effet la memoire totale de la machine, sans la limiter a 2 GB
	GlobalMemoryStatus(&memstat);
	return memstat.dwTotalPhys;
}

longint MemGetFreePhysicalMemory()
{
	MEMORYSTATUS memstat;

	// L'utilisation conseillee GlobalMemoryStatusEx n'a pas ete concluante
	// On renvoie en effet la memoire totale de la machine, sans la limiter a 2 GB
	GlobalMemoryStatus(&memstat);
	return memstat.dwAvailPhys;
}

longint MemGetCurrentProcessVirtualMemory()
{
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc));
	SIZE_T virtualMemUsedByMe = pmc.WorkingSetSize;
	return virtualMemUsedByMe;
}

const int nMACAddressLength = 18;

// Librairie necessaire: Iphlpapi.lib
#pragma comment(lib, "Iphlpapi")

// Recherche l'adresse MAC et la remplit
const char* GetMACAddress()
{
	boolean bDisplayDetails = false;
	const int nPriorityBluetooth = 0;
	const int nPriorityWireless = 1;
	const int nPriorityOther = 2;
	const int nPriorityLocal = 3;
	int nBestPriority;
	int nPriority;
	char* sBuffer;
	ULONG uFlags;
	ULONG uFamily;
	PIP_ADAPTER_ADDRESSES pAdapterAddresses;
	DWORD dwBufLen;
	DWORD dwStatus;
	const int nDefaultBufferSize = 16384;
	char sDescription[1001];
	static char sMACAddress[nMACAddressLength];
	char sNewMACAddress[nMACAddressLength];

	// Initialisation d'une valeur par defaut
	// Pour avoir une adresse MAC unique et permanente pouvant servir d'identifiant de machine, on priorise les type
	// de carte des moins permanentes (bluetooth) aux plus permanentes (reseau local), et par niveau de priorite, on
	// prendra la plus grand addresse MAC (ordre lexicographique) La solution est heuristique, mais devrait
	// permettre de minimiser les risques de changement d'identifiant de machine intempestif
	strcpy(sMACAddress, "00-00-00-00-00-00");
	nBestPriority = -1;

	// Flags pour GetAdaptersAddresses
	// On a du rajouter le parametre non documente GAA_FLAG_SKIP_DNS_INFO (cf. forum ci-dessous)
	// pour eviter le message "Invalid parameter passed to C runtime function."
	// https:
	// //developercommunity.visualstudio.com/content/problem/363323/getadaptersaddresses-invalid-parameter-passed-to-c.html
	uFlags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_DNS_INFO;

	// Default to unspecified address family (both)
	uFamily = AF_UNSPEC;

	// Appel a GetAdaptersAddresses en deux temps, pour obtenir la taille souhaitee, puis pour obtenir les infos
	// On passe d'abord par nDefaultBufferSize (micropsoft recommande de demander au moins 15 kb au premier appel)
	dwBufLen = nDefaultBufferSize;
	sBuffer = SystemObject::NewCharArray(dwBufLen);
	pAdapterAddresses = (PIP_ADAPTER_ADDRESSES)sBuffer;
	dwStatus = GetAdaptersAddresses(uFamily, uFlags, NULL, pAdapterAddresses, &dwBufLen);
	if (dwStatus == ERROR_BUFFER_OVERFLOW)
	{
		SystemObject::DeleteCharArray(sBuffer);
		sBuffer = SystemObject::NewCharArray(dwBufLen);
		pAdapterAddresses = (PIP_ADAPTER_ADDRESSES)sBuffer;
		dwStatus = GetAdaptersAddresses(uFamily, uFlags, NULL, pAdapterAddresses, &dwBufLen);
		if (bDisplayDetails)
			printf(" GetMACAddress - GetAdaptersAddresses -> %ld (%ld)\n", dwStatus, dwBufLen);
	}

	// Si OK, recherche de la premiere MAC address valid
	if (dwStatus == ERROR_SUCCESS)
	{
		// Boucle sur les infos trouvees
		while (pAdapterAddresses != NULL)
		{
			nPriority = nPriorityOther;

			// Copie des "wide char" vers des char pour la descrition de la carte, et passage n minuscule
			wcstombs(sDescription, pAdapterAddresses->FriendlyName, sizeof(sDescription) - 1);
			_strlwr(sDescription);

			// On test si on est en blue tooth
			if (strstr(sDescription, "bluetooth") != NULL)
				nPriority = nPriorityBluetooth;
			// On test si on est en wireless
			else if (pAdapterAddresses->ConnectionType == IF_TYPE_IEEE80211 ||
				 strstr(sDescription, "wireless") != NULL)
				nPriority = nPriorityWireless;
			// On teste si on est en reseau local
			else if (strstr(sDescription, "local area") != NULL)
				nPriority = nPriorityLocal;

			// Affichage (on utilise le %ls car les chaines sont en double byte character)
			if (bDisplayDetails)
			{
				printf("\tAdapter (%ld): (priority=%d), %ls \n\t\t(type=%ld, %ls)\n",
				       pAdapterAddresses->PhysicalAddressLength, nPriority,
				       pAdapterAddresses->FriendlyName, pAdapterAddresses->IfType,
				       pAdapterAddresses->Description);
			}

			// Test si addresse de taille 6 (adresse MAC)
			// Initialisation une premiere fois (meme si wifi) et les fois suivante que si non wifi
			if (pAdapterAddresses->PhysicalAddressLength == 6 && nPriority >= nBestPriority)
			{
				// Memorisation de la nouvelle adresse que si elle est plus grande que la precedente
				// (ou d'une priorite plus forte, pour favoriser les cates reseau persistantes)
				// Pour garantir que l'on ne depend pas de l'ordre dans la liste
				sprintf(sNewMACAddress, "%02x-%02x-%02x-%02x-%02x-%02x",
					(unsigned char)pAdapterAddresses->PhysicalAddress[0],
					(unsigned char)pAdapterAddresses->PhysicalAddress[1],
					(unsigned char)pAdapterAddresses->PhysicalAddress[2],
					(unsigned char)pAdapterAddresses->PhysicalAddress[3],
					(unsigned char)pAdapterAddresses->PhysicalAddress[4],
					(unsigned char)pAdapterAddresses->PhysicalAddress[5]);
				if (nPriority > nBestPriority || strcmp(sNewMACAddress, sMACAddress) > 0)
					strcpy(sMACAddress, sNewMACAddress);
				if (bDisplayDetails)
					printf("\t  MAC %s -> %s\n", sNewMACAddress, sMACAddress);

				// Memorisation de la priorite
				nBestPriority = nPriority;
			}
			pAdapterAddresses = pAdapterAddresses->Next;
		}
	}
	if (bDisplayDetails)
		printf(" GetMACAddress -> %s\n", sMACAddress);

	// Nettoyage
	SystemObject::DeleteCharArray(sBuffer);
	return sMACAddress;
}

const char* GetSerialNumber()
{
	static char sSerialNumber[2];

	// On passe par une variable static pour que la memoire de la chaine de caractere
	// sSerialNumber soit conserve apres le retour de la fonction
	strcpy(sSerialNumber, "");
	return sSerialNumber;
}

const char* GetLocalHostName()
{
	/*static char sHostName[256];
	gethostname(sHostName, 256);
	return sHostName; PBM LNK2019*/
	// TODO BG: ameliorer l'implementation
	return getenv("COMPUTERNAME");
}

long WINAPI unhandled_exception_handler(EXCEPTION_POINTERS* p_exceptions)
{
	// Suppress C4100 Warnings unused parameters required to match the
	// function signature of the API call.
	(void*)p_exceptions;

	// Throw any and all exceptions to the ground.
	return EXCEPTION_EXECUTE_HANDLER;
}

// Cf. http://www.zachburlingame.com/2011/04/silently-terminate-on-abortunhandled-exception-in-windows/
void SuppressCrashHandlers()
{
	// Register our own unhandled exception handler
	// http://msdn.microsoft.com/en-us/library/ms680634(v=vs.85).aspx
	SetUnhandledExceptionFilter(unhandled_exception_handler);

	// Minimize what notifications are made when an error occurs
	// http://msdn.microsoft.com/en-us/library/ms680621(v=vs.85).aspx
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT | SEM_NOGPFAULTERRORBOX |
		     SEM_NOOPENFILEERRORBOX);

	// When the app crashes, don't print the abort message and don't call Dr. Watson to make a crash dump.
	// http://msdn.microsoft.com/en-us/library/e631wekh(v=VS.100).aspx
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
}

const char* GetMachineGUID()
{
	HKEY hKey;
	int nError;
	char* res = StandardGetBuffer();
	res[0] = '\0';
	LONG lRes = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Cryptography", 0,
				  KEY_READ | KEY_WOW64_64KEY, &hKey);
	if (lRes == ERROR_SUCCESS)
	{
		DWORD dwBufferSize = BUFFER_LENGTH;
		nError = RegQueryValueExA(hKey, "MachineGuid", 0, NULL, (LPBYTE)res, &dwBufferSize);
		if (nError != ERROR_SUCCESS)
		{
			res[0] = '\0';
		}
		RegCloseKey(hKey);
	}
	return res;
}

int GetMaxOpenedFileNumber()
{
	return _getmaxstdio();
}

// Pur acceder aux information du processeur
#include <intrin.h>

// Acces au nom du processeur pour alimenter les SystemInfos
// https : //vcpptips.wordpress.com/2012/12/30/how-to-get-the-cpu-name/
static char* GetProcessorName()
{
	static char CPUBrandString[0x40];
	int CPUInfo[4] = {-1};
	__cpuid(CPUInfo, 0x80000000);
	int nExIds = CPUInfo[0];

	memset(CPUBrandString, 0, sizeof(CPUBrandString));

	// Get the information associated with each extended ID.
	for (int i = 0x80000000; i <= nExIds; ++i)
	{
		__cpuid(CPUInfo, i);
		// Interpret CPU brand string.
		if (i == 0x80000002)
			memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
		else if (i == 0x80000003)
			memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
		else if (i == 0x80000004)
			memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
	}
	return CPUBrandString;
}

typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

// Acces a la version de windows pour alimenter les SystemInfos
// https://stackoverflow.com/questions/36543301/detecting-windows-10-version/36543774#36543774
static char* GetOsVersion()
{
	static char sWindowsVersion[100];

	sWindowsVersion[0] = '\0';

	// Acces a la DLL
	HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
	if (hMod)
	{
		// Recherche de la fonction donnant les information de version
		RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
		if (fxPtr != nullptr)
		{
			RTL_OSVERSIONINFOW osInfo = {0};
			osInfo.dwOSVersionInfoSize = sizeof(osInfo);
			if (fxPtr(&osInfo) == 0)
			{
				sprintf_s(sWindowsVersion, "windows %d.%d (%d)", osInfo.dwMajorVersion,
					  osInfo.dwMinorVersion, osInfo.dwBuildNumber);
			}
		}
	}
	return sWindowsVersion;
}

const char* GetSystemInfos()
{
	char* sInfo = StandardGetBuffer();
	char sBuffer[100];

	// Nom du processeur
	sInfo[0] = '\0';
	SecureStrcpy(sInfo, "cpu=", BUFFER_LENGTH);
	SecureStrcpy(sInfo, GetProcessorName(), BUFFER_LENGTH);
	SecureStrcpy(sInfo, "\n", BUFFER_LENGTH);

	// OS
	SecureStrcpy(sInfo, "os=", BUFFER_LENGTH);
	SecureStrcpy(sInfo, GetOsVersion(), BUFFER_LENGTH);
	SecureStrcpy(sInfo, "\n", BUFFER_LENGTH);
	return sInfo;
}

#endif // _WIN32

///////////////////////////////////////////////////////////////////////////////////////////
// Implementation Linux
#ifdef __linux_or_apple__

#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <cmath>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

#ifndef __ANDROID__
#include <ifaddrs.h>
#endif

#ifdef __APPLE__
#include <libproc.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/vmmeter.h>
#include <mach/mach.h>

#else // __APPLE__

#include <netpacket/packet.h>
#include <sys/sysinfo.h>

#ifdef __ANDROID__
#include <sys/vfs.h> // ANDROID https://svn.boost.org/trac/boost/ticket/8816
#else
#include <sys/statvfs.h>
#endif // __ANDROID__
#endif // __APPLE__
using namespace std;

void SystemSleep(double dSeconds)
{
	struct timespec req;
	req.tv_sec = (int)floor(dSeconds);
	req.tv_nsec = (long)floor((dSeconds - req.tv_sec) * 1000000000.0 + 0.0005);
	nanosleep(&req, NULL);
}

longint DiskGetFreeSpace(const char* sPathName)
{
	// cf. statvfs for linux.
	// http://stackoverflow.com/questions/1449055/disk-space-used-free-total-how-do-i-get-this-in-c
	// http://pubs.opengroup.org/onlinepubs/009695399/basedefs/sys/statvfs.h.html

#if defined(__ANDROID__) || defined(__APPLE__)
	struct statfs fiData;
#else
	struct statvfs64 fiData;
#endif
	longint lFree;
	int nRet;

	assert(sPathName != NULL);

	p_SetMachineLocale();

#if defined(__ANDROID__) || defined(__APPLE__)
	nRet = statfs(sPathName, &fiData);
#else
	nRet = statvfs64(sPathName, &fiData);
#endif

	if (nRet < 0)
	{
		lFree = 0;
	}
	else
	{
		lFree = fiData.f_bavail;
		lFree *= fiData.f_bsize;
	}
	p_SetApplicationLocale();
	assert(lFree >= 0);
	return lFree;
}

int SystemGetProcessorNumber()
{
#ifdef __APPLE__
	int rc;
	int numCPU;
	int activeCPU;
	std::size_t len = sizeof(numCPU);

	// Nombre de CPU physques
	rc = sysctlbyname("hw.physicalcpu", &numCPU, &len, NULL, 0);
	if (rc < 0)
	{
		return 0;
	}

	// Nombre de CPU actifs
	rc = sysctlbyname("hw.activecpu", &activeCPU, &len, NULL, 0);
	if (rc < 0)
	{
		return 0;
	}
	return min(numCPU, activeCPU);
#else  // __APPLE__
	FILE* file;
	int nRes;
	const int nLineSize = 4096;
	const int nMaxProcIds = 1024;
	char sLine[nLineSize];
	const int nNumberSize = 9;
	char sNumber[nNumberSize];
	int vProcIds[nMaxProcIds];
	int bOk;
	int nCoresNumber;
	int i;
	char c;
	int j;
	int nCpuNumber;
	int nProcId;

	// Ce code s'inspire de la page web http://xmodulo.com/how-to-find-number-of-cpu-cores-on.html
	// Tout repose qur la lecture du fichier /proc/cpuinfo
	// La commande  "cat /proc/cpuinfo | grep "^physical id" | sort | uniq | wc -l" permet d'avoir le nombre de
	// processeurs physiques La commande "cat /proc/cpuinfo | grep "^cpu cores" | uniq" permet d'obtenir le nombre
	// de coeurs par processeurs

	nCoresNumber = 0;
	nCpuNumber = 0;
	bOk = true;

	// Ouverture du fichier
	file = p_fopen("/proc/cpuinfo", "r");
	if (file == NULL)
	{
		bOk = false;
	}
	else
	{
		// Initialisation du tableau des ids des processeurs
		for (i = 0; i < nMaxProcIds; i++)
		{
			vProcIds[i] = 0;
		}
		// Parcours du fichier ligne a ligne
		while (fgets(sLine, nLineSize, file))
		{
			/***********************
			 * Extraction des identifiants des processeurs
			 ***************************/

			// Si la ligne commence par "physical id" on la stocke en tant que clef dans le dictionnaire
			if (!memcmp(sLine, "physical id", 11))
			{
				c = ' ';
				i = 0;

				// Positionnemant apres le ':'
				while (c != ':')
				{
					c = sLine[i];
					i++;
				}

				// Extraction de l'id du processeur
				j = 0;
				while (c != '\n' and c != '\0')
				{
					if (j > nNumberSize)
					{
						bOk = false;
						break;
					}
					c = sLine[i];
					sNumber[j] = c;
					j++;
					i++;
				}
				sNumber[j] = '\0';

				// On tag l'Id
				nProcId = atoi(sNumber);
				if (nProcId < nMaxProcIds)
					vProcIds[nProcId] = 1;
			}

			/**********************************
			 * Extraction du nombre de coeurs
			 **********************************/
			// Si la ligne commence par "cpu cores", on en extrait le chiffre a la fin
			else if (nCoresNumber == 0 and !memcmp(sLine, "cpu cores", 9))
			{
				c = ' ';
				i = 0;

				// Positionnemant apres le ':'
				while (c != ':')
				{
					c = sLine[i];
					i++;
				}

				// Recopie de ce qui suit le ':'
				j = 0;
				while (c != '\n' and c != '\0')
				{
					if (j > nNumberSize)
					{
						bOk = false;
						break;
					}
					c = sLine[i];
					sNumber[j] = c;
					j++;
					i++;
				}
				sNumber[j] = '\0';
				nCoresNumber = atoi(sNumber);
				if (not bOk)
					break;
			}
		}
		fclose(file);
	}

	// Comptage du nombre de Cpu
	for (i = 0; i < nMaxProcIds; i++)
	{
		if (vProcIds[i])
			nCpuNumber++;
	}

	if (nCpuNumber * nCoresNumber == 0 or not bOk)
	{
		// Si echec, renvoie le nombre de processeurs en ignorant l'hyper-threading
		nRes = sysconf(_SC_NPROCESSORS_ONLN);
	}
	else
	{
		// On renvoie le nombre de processeurs * le nombre de coeurs par processeur
		nRes = nCpuNumber * nCoresNumber;
	}

	// Garde fou : on renvoie au moins 1 coeur
	if (nRes == 0)
		nRes = 1;
	return nRes;
#endif // __APPLE__
}

const char* CurrentPreciseTimestamp()
{
	char* sBuffer = StandardGetBuffer();
	struct timeval tv;
	struct tm* ptm;
	char time_string[30];
	long milliseconds;

	/* Obtain the time of day, and convert it to a tm struct. */
	gettimeofday(&tv, NULL);
	ptm = localtime(&tv.tv_sec);

	/* Format the date and time, down to a single second. */
	strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", ptm);
	/* Compute milliseconds from microseconds. */
	milliseconds = tv.tv_usec / 1000;
	/* Print the formatted time, in seconds, followed by a decimal point
	and the milliseconds. */
	sprintf(sBuffer, "%s.%03ld", time_string, milliseconds);

	return sBuffer;
}

const char* CurrentTimestamp()
{
	char* sBuffer = StandardGetBuffer();
	struct timeval tv;
	struct tm* ptm;

	/* Obtain the time of day, and convert it to a tm struct. */
	gettimeofday(&tv, NULL);
	ptm = localtime(&tv.tv_sec);

	/* Format the date and time, down to a single second. */
	strftime(sBuffer, 30, "%Y-%m-%d %H:%M:%S", ptm);
	return sBuffer;
}

void SuppressCrashHandlers()
{
	// A priori, rien a implementer sous Linux
}

longint MemGetAdressablePhysicalMemory()
{
	// On utilise une variable locale pour eviter le warning "conditional expression is constant"
	int nSizeOfSizeT = sizeof(size_t);

	if (nSizeOfSizeT == SIZE_OF_SIZE_T_32_BITS_SYSTEM)
	{
		// Sous Unix, 4 Go de memoire sont utilisables en 32 bits
		// 2^(sizeof(size_t)*8-20)
		return 4 * lGB - 256 * lMB;
	}
	else
	{
		// Assume 40 bits addresses (2^(40-20))
		return lTB;
	}
}

longint MemGetAvailablePhysicalMemory()
{
#ifdef __APPLE__
	int mib[] = {CTL_HW, HW_MEMSIZE};
	int64_t value = 0;
	size_t length = sizeof(value);

	sysctl(mib, 2, &value, &length, NULL, 0);
	return value;
#else  // __APPLE__
	struct sysinfo info;
	longint lByteNumber;
	int nErr;

	nErr = sysinfo(&info);
	if (nErr != 0)
		lByteNumber = 2 * lGB;
	else
		lByteNumber = info.totalram * info.mem_unit;
	assert(lByteNumber >= 0);
	return lByteNumber;
#endif // __APPLE__
}
#ifdef __APPLE__
static size_t get(const char* name)
{
	int nValue;
	size_t len = sizeof(nValue);
	if (sysctlbyname(name, &nValue, &len, NULL, 0) < 0)
	{
	std:
		cerr << "error when acces " << name << std::endl;
		nValue = -1;
	}
	return nValue;
}
#endif // __APPLE__

longint MemGetFreePhysicalMemory()
{
#ifdef __APPLE__
	size_t pagesize = get("vm.pagesize");
	size_t pages = get("vm.pages");

	// Memoire disponible
	size_t pagefree = get("vm.page_free_count");

	// Memoire disponible apres purge
	size_t pagepurge = get("vm.page_pageable_external_count");
	// size_t pagepurge2=get("vm.page_pageable_internal_count");

	// std::cout << "free: " << pagesize * pagefree / 10000000 << " Mo" << endl;
	// std::cout << "purge: " << pagesize * pagepurge / 10000000 << " Mo" << endl;
	// std::cout << "purge2: " << pagesize * pagepurge2 / 10000000 << " Mo" << endl;
	// std::cout << "mem: " << pagesize * pages / 10000000 << " Mo" << endl;
	// std::cout << "available? : " << pagesize * (pagepurge+pagefree) / 10000000 << " Mo" << endl;
	// std::cout << "used? : " << pagesize * (pages -pagepurge - pagefree) / 10000000 << " Mo" << endl;
	if (pagesize == -1 or pagepurge == -1 or pagefree == -1)
		return 0;
	return pagesize * (pagepurge + pagefree);
#else  // __APPLE_

	// Lecture du fichier /proc/meminfo pour extraire la memoire dispoible et la memoire en cache
	// On additionne la memoire disponible et 80% de la memoire cache (borne a 2Go)
	FILE* file;
	const int nLineSize = 4096;
	char sLine[nLineSize];
	int i;
	int j;
	char c;
	longint lMemFree = -1;
	longint lCached = -1;
	int nNumberSize = 9;
	char sNumber[nNumberSize];
	int bOk;

	bOk = true;
	file = p_fopen("/proc/meminfo", "r");
	if (file == NULL)
	{
		bOk = false;
	}
	else
	{
		// Parcours du fichier ligne a ligne
		while (fgets(sLine, nLineSize, file))
		{
			// Si la ligne commence par "MemFree" on extrait la memoire disponible
			if (!memcmp(sLine, "MemFree", 7))
			{
				c = ' ';
				i = 0;

				// Positionnemant apres le ':'
				while (c != ':')
				{
					c = sLine[i];
					i++;
				}

				// Positionnement apres les escpaces
				c = sLine[i];
				while (c == ' ')
				{
					c = sLine[i];
					i++;
				}

				// Recopie de ce qui suit les espaces
				i--;
				j = 0;
				while (c != ' ' and c != '\0' and c != 'k')
				{
					if (j > nNumberSize)
					{
						bOk = false;
						break;
					}
					c = sLine[i];
					sNumber[j] = c;
					j++;
					i++;
				}
				sNumber[j] = '\0';
				lMemFree = atol(sNumber);
				if (lCached != -1)
					break;
			}
			// Si la ligne commence par "Cached" on extrait la memoire disponible
			if (!memcmp(sLine, "Cached", 6))
			{
				c = ' ';
				i = 0;

				// Positionnemant apres le ':'
				while (c != ':')
				{
					c = sLine[i];
					i++;
				}

				// Positionnement apres les escpaces
				c = sLine[i];
				while (c == ' ')
				{
					c = sLine[i];
					i++;
				}

				// Recopie de ce qui suit le ':'
				i--;
				j = 0;
				while (c != ' ' and c != '\0' and c != 'k')
				{
					if (j > nNumberSize)
					{
						bOk = false;
						break;
					}
					c = sLine[i];
					sNumber[j] = c;
					j++;
					i++;
				}
				sNumber[j] = '\0';
				lCached = atol(sNumber);
				if (lMemFree != -1)
					break;
			}
		}
		fclose(file);
	}

	if (bOk)
		if (0.2 * lCached * lKB > 2 * lGB)
			return (lMemFree + lCached) * lKB - 2 * lGB;
		else
			return (lMemFree + 0.8 * lCached) * lKB;
	else
		return 2 * lGB;
#endif // __APPLE__
}

longint MemGetCurrentProcessVirtualMemory()
{
#ifdef __APPLE__
	struct task_basic_info t_info;
	mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

	if (KERN_SUCCESS != task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count))
		return 0;

	// resident size is in t_info.resident_size;
	// virtual size is in t_info.virtual_size;
	return (longint)t_info.resident_size;

#else  // __APPLE__
	FILE* file;
	longint nPageSize;
	longint nResult;
	char sLine[128];
	int i;
	bool bOk;

	bOk = true;
	file = NULL;
	nResult = 0;
	nPageSize = sysconf(_SC_PAGESIZE);
	if (nPageSize == -1)
	{
		bOk = false;
	}

	if (bOk)
		file = p_fopen("/proc/self/statm", "r");

	if (file == NULL)
		bOk = false;
	if (bOk)

		if (fgets(sLine, 128, file) != NULL)
		{
			i = 0;
			while (sLine[i] != ' ')
				i++;
			sLine[i] = '\0';
			nResult = atoi(sLine) * nPageSize;
		}
	return nResult;
#endif // __APPLE__
}

int IsDirectory(const char* sPathName)
{
	struct stat s;
	int nError = stat(sPathName, &s);
	if (nError != 0)
		return 0;
	else
		return s.st_mode & S_IFDIR;
}

int IsVirtual(const char* sInterfaceName)
{
	// Le repertoire /sys/class/net/ contient un repertoire eponyme pour chaque interface reseau
	// Lorsque l'interface est physique le repertoire contient le repertoire symlink device
	const char* sRep = "/sys/class/net/";
	char sDeviceDirectory[1000];

	if (!IsDirectory(sRep))
		// L'heuristique ne fonctionne pas, on ne sait pas si c'est virtuel ou non
		return 0;

	sprintf(sDeviceDirectory, "%s%s/device/", sRep, sInterfaceName);
	if (IsDirectory(sDeviceDirectory))
		// C'est une interface physique
		return 0;

	// Sinon elle est virtuelle
	return 1;
}

const int nMACAddressLength = 18;

#if defined __ANDROID__ || defined __APPLE__
const char* GetMACAddress()
{
	static char sMACAddress[nMACAddressLength];
	strcpy(sMACAddress, "00-00-00-00-00-00");
	return sMACAddress;
}
#else  // __ANDROID__ || defined __APPLE__
const char* GetMACAddress()
{
	struct ifaddrs* ifaddr = NULL;
	struct ifaddrs* ifa = NULL;
	int bDisplayDetails;
	int nBestPriority;
	int nPriority;
	int i;
	const int nPriorityVirtual = 0; // L'interface est virtuelle
	const int nPriorityDummyARP =
	    1; // L'interface est physique et a un type ARP qui ne correspond pas a une interface physique
	const int nPriorityPhysical =
	    2; // L'interface est physique et a un type ARP qui correspond a un type connu de reseau
	static char sMACAddress[nMACAddressLength];
	char sNewMACAddress[nMACAddressLength];

	bDisplayDetails = 0;
	nBestPriority = -1;
	strcpy(sMACAddress, "00-00-00-00-00-00");

	// Utilisation de getifaddrs qui donne la liste chainee des interfaces resesau
	// Lorsque l'interface est du type AF_PACKET on peut la surcharger  avec le type
	// sockaddr_ll qui donne acces aux infos dont on a besoin
	if (getifaddrs(&ifaddr) != -1)
	{
		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
		{
			if ((ifa->ifa_addr) && (ifa->ifa_addr->sa_family == AF_PACKET))
			{
				// Cast en sockaddr_ll
				struct sockaddr_ll* s = (struct sockaddr_ll*)ifa->ifa_addr;

				// Affichage de toutes les informations dont on dispose
				if (bDisplayDetails)
				{
					printf("MAC Address  of %s is ", ifa->ifa_name);
					for (i = 0; i < s->sll_halen; i++)
					{
						printf("%02x%c", (s->sll_addr[i]),
						       (i + 1 != s->sll_halen) ? ':' : '\n');
					}
					printf("\tindex : %d\n", s->sll_ifindex);
					printf("\tARP type : %d", s->sll_hatype);

					if (s->sll_hatype == 1)
						printf(" (ethernet)");
					else if (s->sll_hatype == 6)
						printf(" (wireless)");
					else if (s->sll_hatype > 43)
						printf(" (non ARP hardware)");
					printf("\n");

					if (IsVirtual(ifa->ifa_name))
						printf("\tvirtual interface\n");
					else
						printf("\tphysical interface\n");
				}

				// Si l'adresse mac n' pas la bonne taille on ne la prend pas en compte
				if (s->sll_halen == 6)
				{
					// Calcul des priorites
					if (IsVirtual(ifa->ifa_name))
						nPriority = nPriorityVirtual;
					else
					{
						if (s->sll_hatype > 43)
							nPriority = nPriorityDummyARP;
						else
							nPriority = nPriorityPhysical;
					}
					if (bDisplayDetails)
						printf("\tpriority : %d\n", nPriority);

					// Si l'interface est prioritaire
					if (nPriority >= nBestPriority)
					{
						sprintf(sNewMACAddress, "%02x-%02x-%02x-%02x-%02x-%02x",
							(unsigned char)s->sll_addr[0], (unsigned char)s->sll_addr[1],
							(unsigned char)s->sll_addr[2], (unsigned char)s->sll_addr[3],
							(unsigned char)s->sll_addr[4], (unsigned char)s->sll_addr[5]);

						// On garde la prioritaire
						// En cas d'egalite, on garde l'adresse qui est la plus grande suivant
						// l'ordre lexicographique
						if (nPriority > nBestPriority ||
						    strcmp(sNewMACAddress, sMACAddress) > 0)
						{
							strcpy(sMACAddress, sNewMACAddress);
							if (bDisplayDetails)
								printf("\t=> best adress (till now)\n");
						}

						// Memorisation de la priorite
						nBestPriority = nPriority;
					}
				}
				else
				{
					if (bDisplayDetails)
						printf("\twrong size\n");
				}
				if (bDisplayDetails)
					printf("\n");
			}
		}
		freeifaddrs(ifaddr);
	}
	return sMACAddress;
}
#endif // __ANDROID__

#ifdef __ANDROID__
#include <sys/system_properties.h>
#endif
const char* GetSerialNumber()
{
#ifdef __ANDROID__
	static char sSerialNumber[PROP_VALUE_MAX + 1];
	// ro.boot.serialno or ro.product.serialno or ro.serialno
	if (0 == __system_property_get("ro.boot.serialno", sSerialNumber))
	{
		/* System property is not found or it has an empty value. */
		if (0 == __system_property_get("ro.product.serialno", sSerialNumber))
		{
			/* System property is not found or it has an empty value. */
			if (0 == __system_property_get("ro.serialno", sSerialNumber))
			{
				/* System property is not found or it has an empty value. */
				strcpy(sSerialNumber, "");
			}
		}
	}
	return sSerialNumber;
#else
	static char sSerialNumber[2];

	// On passe par une variable static pour que la memoire de la chaine de caractere
	// sSerialNumber soit conserve apres le retour de la fonction
	strcpy(sSerialNumber, "");
	return sSerialNumber;

#endif // __ANDROID__
}

const char* GetLocalHostName()
{
	int nRet;
#ifdef __APPLE__
	const int HOST_NAME_MAX = 256;
#endif // __APPLE__
	static char sHostName[HOST_NAME_MAX + 1];
	static bool bIsInitialized = false;
	if (!bIsInitialized)
	{
		sHostName[HOST_NAME_MAX] = '\0';
		nRet = gethostname(sHostName, HOST_NAME_MAX);
		if (nRet != 0)
			sHostName[0] = '\0';
		bIsInitialized = true;
	}
	return sHostName;
}

static void GetFileFirstLine(const char* sFileName, char* sContent, int nMaxSize)
{
	FILE* file;

	// Initialisation avec chaine vide
	strcpy(sContent, "");

	// Ouverture du fichier
	file = p_fopen(sFileName, "r");
	if (file != NULL)
	{
		if (fgets(sContent, nMaxSize, file) == NULL)
			strcpy(sContent, "");
		fclose(file);
	}
}

const char* GetMachineGUID()
{
#ifdef __APPLE__
	const int nLineSize = 120;
	static char sLine[nLineSize];
	strcpy(sLine, "not yet implemented");
	return sLine;
#else  // __APPLE__
	const int nLineSize = 40;
	static char sLine[nLineSize];

	// On test successivement 3 fichiers ou devrait se situer le UID
	// Les fichiers peuvent etre presents et vides
	GetFileFirstLine("/etc/machine-id", sLine, nLineSize);
	if (strlen(sLine) == 0)
		GetFileFirstLine("/var/lib/dbus/machine-id", sLine, nLineSize);
	if (strlen(sLine) == 0)
		GetFileFirstLine("/var/db/dbus/machine-id", sLine, nLineSize);

	return sLine;
#endif // __APPLE__
}

int GetMaxOpenedFileNumber()
{
	struct rlimit lim;
	getrlimit(RLIMIT_NOFILE, &lim);
	return lim.rlim_cur;
}

const char* GetSystemInfos()
{
	FILE* file = NULL;
	int i = 0;
	int nLineCount;
	char* sInfo = StandardGetBuffer();
	char c;
	struct utsname buffer;
	bool bOk;

	sInfo[0] = '\0';

#ifndef __APPLE__
	// Parcours du fichier os-release
	file = p_fopen("/etc/os-release", "rb");
	if (file == NULL)
		sInfo[0] = '\0';
	else
	{
		// on ne garde que les 3 premieres lignes
		c = ' ';
		nLineCount = 0;
		while (nLineCount < 3 and i < BUFFER_LENGTH)
		{
			c = fgetc(file);
			if (c == EOF)
				break;
			if (c == '\n')
				nLineCount++;
			sInfo[i] = c;
			i++;
		}
		sInfo[i] = '\0';
		fclose(file);
	}
#endif // __APPLE__

	// Ajout de l'architecture (fonctionne sur Linux et macOS)
	if (uname(&buffer) >= 0)
	{
		bOk = true;
		bOk = bOk and SecureStrcpy(sInfo, "system=", BUFFER_LENGTH);
		bOk = bOk and SecureStrcpy(sInfo, buffer.sysname, BUFFER_LENGTH);
		bOk = bOk and SecureStrcpy(sInfo, "\n", BUFFER_LENGTH);
		bOk = bOk and SecureStrcpy(sInfo, "release=", BUFFER_LENGTH);
		bOk = bOk and SecureStrcpy(sInfo, buffer.release, BUFFER_LENGTH);
		bOk = bOk and SecureStrcpy(sInfo, "\n", BUFFER_LENGTH);
		bOk = bOk and SecureStrcpy(sInfo, "version=", BUFFER_LENGTH);
		bOk = bOk and SecureStrcpy(sInfo, buffer.version, BUFFER_LENGTH);
		bOk = bOk and SecureStrcpy(sInfo, "\n", BUFFER_LENGTH);
	}
	return sInfo;
}

#endif // __linux_or_apple__
