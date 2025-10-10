// Copyright (c) 2023-2025 Orange. All rights reserved.
// This software is distributed under the BSD 3-Clause-clear License, the text of which is available
// at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

/////////////////////////////////////////////////////////////////
// Implementation standard pour Linux
// Les define sont explicites, car on n'utile pas ici la librairie Norm
#if defined(__linux__) || defined(__APPLE__)

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv)
{
	printf("This program is used by Khiops, only on Windows.\n");
	return EXIT_FAILURE;
}

#endif //  __linux__ or __APPLE__

////////////////////////////////////////////////////
// Implementation pour Windows
#ifdef _WIN32

#include <windows.h>
#include <iostream>
#include <string>

// Retourne le message systeme pour un code d erreur Win32 (UTF-16).
static std::wstring FormatLastError(DWORD err)
{
	std::wstring wsOutput;
	wchar_t* message;
	DWORD length;

	message = nullptr;
	length =
	    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			   nullptr, err, 0, (LPWSTR)&message, 0, nullptr);
	wsOutput = length ? message : L"Unknown error";
	if (message)
		LocalFree(message);
	return wsOutput;
}

// Convertit une chaine d'octets (std::string) encodee avec codePage en UTF-16 (std::wstring).
// Renvoie une chaine vide si l'entree est vide ou si la conversion echoue.
static std::wstring ToWide(const std::string& sInput, UINT codePage)
{
	std::wstring wsOutput;
	int nLen;

	// Cas trivial : rien a convertir
	if (sInput.empty())
		return L"";

	// Premier passage : longueur UTF-16 requise (en wchar_t)
	nLen = MultiByteToWideChar(codePage, 0, sInput.data(), (int)sInput.size(), nullptr, 0);

	// Second passage : conversion apres allocation de la chaine a la bonne longueur
	if (nLen > 0)
	{
		wsOutput.resize(nLen);
		MultiByteToWideChar(codePage, 0, sInput.data(), (int)sInput.size(), &wsOutput[0], nLen);
	}
	return wsOutput;
}

// Convertit une chaine encodee en UTF-16 (std::wstring) vers un char* (a detruire par l'appelant)
static char* WStringToChar(const std::wstring& wstr)
{
	int nLen;
	char* sBuffer;

	// Taille necessaire
	nLen = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (nLen == 0)
	{
		sBuffer = new char[1];
		sBuffer[0] = '\0';
		return sBuffer;
	};

	// Allocation du buffer
	sBuffer = new char[nLen];

	// Conversion
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, sBuffer, nLen, nullptr, nullptr);

	return sBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////
// Lancement d'une fichier de commande de lancement d'un outil Khiops (ex: khiops.cmd)
//
// Cette fonction lance le fichier de commande, redirige sa sortie standard et
// sa sortie d'erreur via des pipes, puis affiche les resultats ou les erreurs
// dans une boite de dialogue en cas d'erreur.
//
// Cela permet d'intercepter des erreurs de lancement, par exemple liees aux droits des
// utilisateurs de lancer des commandes sur la machine cible.
// L'exemple typique est celui de l'outil AppLocker de Microsoft, qui empeche le lancement
// fichier de commande .cmd, sauf en mode administrateur, avec un message dans le console de type:
//  "Ce programme est bloque par une strategie de groupe. Pour plus d'informations,
//   contactez votre administrateur systeme." provoquee par l'utilisation par une DSI de AppLocker
// Comme ces messages ne sont interceptes que dans la console, tout lancement depuis un raccourci
// bureau echoue sans aucun message visible par l'utilisateur.
// Avec ce "launcher", on peut intercepter les messages d'erreur, et avoir une interaction
// utilisateur intelligible dans ce cas.
//
// A noter:
// - on evite d'avoir deux icones avec un exe windows plutot que console
//   - utilisation de WinMain au lieu de main (cf. propriete WIN32_EXECUTABLE du CMakeList)
// - on masque la console avec le parametre CREATE_NO_WINDOW de CreateProcessA
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	std::wstring swCommandPath;
	char* sCommandPath;
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	SECURITY_ATTRIBUTES saAttr = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
	char sCommandLine[4096];
	UINT codePage;
	BOOL success;
	DWORD exitCode;
	char sBuffer[4096];
	DWORD dwRead;
	std::wstring outputStdOut;
	std::wstring outputStdErr;
	std::wstring message;
	int argc;
	LPWSTR* argvW;

	// Pour indiquer que les parametres ne sont pas references et eviter les warnings
	(void)hInstance;
	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nCmdShow;

	// Parsing de la ligne de commande
	argc = 0;
	argvW = CommandLineToArgvW(GetCommandLineW(), &argc);

	// Il doit y avoir un seul parametre
	if (argc != 2)
	{
		MessageBoxW(nullptr, L"Khiops launcher failed: one operand expected", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	// Construction de la commande, apres avoir converti le parametre principal en char*
	swCommandPath = argvW[1];
	sCommandPath = WStringToChar(swCommandPath);
	sprintf_s(sCommandLine, sizeof(sCommandLine), "cmd.exe /C \"%s\"", sCommandPath);
	delete[] sCommandPath;

	// Handles pour les pipes
	HANDLE hStdOutRead, hStdOutWrite;
	HANDLE hStdErrRead, hStdErrWrite;

	// Code page du systeme d'exploitation, notamment pour les erreurs de cmd.exe
	codePage = GetOEMCP();

	// Creation du pipe pour stdout
	if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0))
	{
		MessageBoxW(nullptr,
			    (L"Error in launching " + swCommandPath + L", CreatePipe failed for stdout: " +
			     FormatLastError(GetLastError()))
				.c_str(),
			    L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);

	// Creation du pipe pour stderr
	if (!CreatePipe(&hStdErrRead, &hStdErrWrite, &saAttr, 0))
	{
		MessageBoxW(nullptr,
			    (L"Error in launching " + swCommandPath + L", CreatePipe failed for stderr: " +
			     FormatLastError(GetLastError()))
				.c_str(),
			    L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0);

	// Configuration du STARTUPINFOA
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdOutput = hStdOutWrite;
	si.hStdError = hStdErrWrite;
	si.hStdInput = NULL; // pas besoin d'entree standard

	// Configuration du PROCESS_INFORMATION
	ZeroMemory(&pi, sizeof(pi));

	// Lance le programme
	// Different de la fonction system du C ansi, qui lance un shell avec une commande en parametre
	// Ici, on lance un exe directement
	// Le parametre CREATE_NO_WINDOW permet de masquer la fenetre de console
	success = CreateProcessA(
	    NULL,                // the path
	    (char*)sCommandLine, // Command line
	    NULL,                // Process handle not inheritable
	    NULL,                // Thread handle not inheritable
	    TRUE, // Set handle inheritance to TRUE (sinon, la redirection de la sortie vers NULL ne marche pas)
	    CREATE_NO_WINDOW, // Creation flags
	    NULL,             // Use parent's environment block
	    NULL,             // Use parent's starting directory
	    &si,              // Pointer to STARTUPINFO structure
	    &pi               // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
	);

	// Fermetrure des handles d'ecriture dans le processus parent
	CloseHandle(hStdOutWrite);
	CloseHandle(hStdErrWrite);

	// Message d'erreur si ko
	if (!success)
	{
		MessageBox(NULL, (L"Error in launching " + swCommandPath + FormatLastError(GetLastError())).c_str(),
			   L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	// Lecture de stdout
	while (ReadFile(hStdOutRead, sBuffer, sizeof(sBuffer) - 1, &dwRead, NULL) && dwRead)
	{
		sBuffer[dwRead] = '\0';
		outputStdOut += ToWide(sBuffer, codePage);
	}

	// Lecture de stderr
	while (ReadFile(hStdErrRead, sBuffer, sizeof(sBuffer) - 1, &dwRead, NULL) && dwRead)
	{
		sBuffer[dwRead] = '\0';
		outputStdOut += ToWide(sBuffer, codePage);
	}

	// Attente de la fin du processus
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Recuperation du code de retour
	GetExitCodeProcess(pi.hProcess, &exitCode);

	// Message d'erreur si code retour non null
	if (exitCode != 0)
	{
		// Titre
		message = L"Error in launching " + swCommandPath + L".\n";

		// Message sur la sortie standards ou d'erreur
		if (!outputStdOut.empty())
			message += L"\n" + outputStdOut + L"\n";
		if (!outputStdErr.empty())
			message += L"\n" + outputStdErr + L"\n";

		// Message d'assistance utilisateur
		message += L"\nThe issue may be caused by Khiops being installed in a location restricted by your "
			   L"organization's IT security policy. In this case, try installing Khiops in a recommended "
			   L"directory or run it as an administrator.\n";

		// Message indiquant d'aller sur le site
		message += L"\nFor help, visit the installation pages at https://khiops.org.";

		// Message utilisateur
		MessageBoxW(NULL, message.c_str(), L"Error", MB_OK | MB_ICONINFORMATION);
	}

	// Fermeture des handles
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(hStdOutRead);
	CloseHandle(hStdErrRead);
	return 0;
}

#endif // _WIN32
