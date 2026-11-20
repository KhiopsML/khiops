# Setting Up the Development Environment

## System Requirements
- An IDE with support for C++ and CMake
  - [Visual Studio 2026](https://visualstudio.microsoft.com/vs/)
    - Make sure you enable C/C++ development tools at installation
      - 2022 version: OK
      - 2026 version: Since version 2026, install only the **English** linguistic module
        to avoid numerous and noisy messages like `Remarque : inclusion du fichier : ...`
    - Install the extension 'Format document on Save'
    - Disable Include Cleanup (since Visual Studio 17.8+) to avoid numerous and noisy warnings regarding include policy
      - From the main menu of Visual Studio, choose Tools → Options...
      - In the Options dialog, use the search box (top-left) and type: include cleanup
      - Three options will appear. Set all to "None":
         - "Choose how to highlight #include statements that are unused"
         - "Choose how to highlight unused #include statements that can be optimized"
         - "Choose how to highlight missing #include statements that are transitively included"
  - [Visual Studio Code](https://code.visualstudio.com/)
    - Also install [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
    - In Windows: Also install [Build Tools for Visual Studio](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)

- A Java development kit (JDK)
  - [Eclipse Temurin (OpenJDK)](https://adoptium.net/temurin/releases/)
- A MPI Development Kit:
  - Windows: [Intel MPI](https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library-download.html)
  - Linux and MacOs: [Open MPI](https://www.open-mpi.org)
- A bison and flex Development Kit: optional
  - Necessary only if the src/Learning/KWData lex and yac files need to be recompiled for Khiops dictionary analysis
  - Windows
    - install last version of [win bison and flex](https://github.com/lexxmark/winflexbison/releases)
    - set the the directory for the bison and flex binaries in the path
    - optional: cohabitation with [Gow - The lightweight alternative to Cygwin](https://github.com/bmatzelle/gow)
      - when Gow is installed, Gow's bison and flex binaries are used by mistake
      - fix: in the gow bin directory, rename flex.exe to NO_flex.exe and bison.exe to NO_bison.exe

- A Git installation
  - Windows: [Git for Windows](https://gitforwindows.org/)
    - Includes Git Bash and Terminal
- A Python distribution with `pip` tool installed
  - For example: [Anaconda Python Distribution](https://anaconda.org/)

## Quick Setup

### 1) Configure Git for portable line endings
- Windows: make sure you have enabled `core.autocrlf`
```bash
git config --global core.autocrlf true
```

- Linux/macOS: set `core.autocrlf` to `input` so any files with CRLF endings
  will be committed to the repo with only LF endings.
```bash
git config --global core.autocrlf input
```

### 2) Configure an authentication method for Github

#### Option 1: SSH
- Create and configure a public-private key pair
```bash
# Use Git Bash in windows
ssh-keygen -t ed25519 -C "your@email.com"
```
- This creates the files `id_ed25519` and `id_ed25519.pub` in `~/.ssh`. If they are create elsewhere move them to `.ssh`.
- Copy the contents of the file containing the public key: `~/.ssh/id_ed25519.pub`
- In GitHub, click your icon and go to **Settings**. There go to the **SSH and GPG** section and click on **New SSH key**
- Then copy the contents of your public key in the input text.


**Note:** If you have problems with a proxy (`Connection timeout` or `Connection refused`) configure to connect [SSH over HTTPS](https://docs.github.com/en/authentication/troubleshooting-ssh/using-ssh-over-the-https-port)

#### Option 2: HTTPS
- Create an access token
  - Click on your icon, then **Settings**, then **Developer Settings**, then **Personal Access Tokens** and finally **Fine-grained Tokens**

_TODO_

### 3) Clone the repository
Open a terminal and execute
```bash
git clone git@github.com:KhiopsML/khiops.git
cd khiops
```

### 4) Install pre-commit in the repository
```bash
# Install the pre-commit Python package
pip install pre-commit

# This installs the hooks on the repo and runs them on all files
# It may take a bit of time the first time
pre-commit install
pre-commit run --all-files
```

**Note for Anaconda + Windows:** You must add the `Scripts` directory of your Anaconda installation to the `PATH`. That way pre-commit will be available in any shell. With conda you can use `conda install -c conda-forge identify pre-commit`
### 5) Install MPI
#### On windows:

1. Download intel MPI at [intel OneAPI](https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library-download.html). Choose the "offline installer" and install it.
2. Set environment variable `I_MPI_ROOT=C:\Program Files (x86)\Intel\oneAPI\mpi\latest`
3. Add `%I_MPI_ROOT%\bin` to the variable `path`:
    ```cmd
       set path=%path%;%I_MPI_ROOT%\bin
    ```
4. Uninstall MS-MPI or unset the MS-MPI variables: `MSMPI_BIN`, `MSMPI_INC`, `MSMPI_LIB32`, `MSMPI_LIB64`
5. Remove the directory `build` from the project directory (e.g. `c:\dev\khiops\build`)

#### On macOs:
```bash
brew install openmpi
```
#### On Linux
```bash
sudo apt-get install openmpi-bin libopenmpi-dev
```
or
```bash
sudo dnf install openmpi openmpi-devel
```

### 6) Compile

**Note:** To exploit the test suites of LearningTest from the IDE, the following environnement variables must be set:
- `KHIOPS_API_MODE`: set to `true` to apply API mode during development, with paths relative to the current working directory (otherwise, paths are relative to the data directory, in GUI mode),
- `KhiopsFastExitMode`: set to `false` in order to use the scenario files and execute all instructions without an exit after the first error.

They are set in the kht_test script of the LearningTestTool.

#### 6.1) In Visual Studio 2026
- Open Visual Studio and select **Open Local Folder** and select your local `khiops` folder
  - This may take a few minutes to finish, you can see the current status by selecting the `CMake` output in the output sub-window.
- At the top bar you can select a debug or release preset (ex: _Windows MSVC Release_) and select the menu **Build -> Build All**.

To run the MODL or MODL_Coclustering executable from the IDE, you must have a `launch.vs.json` parameter file under the `.vs`directory :
[launch.vs.json](https://github.com/user-attachments/files/29968344/launch.vs.json)


**Note:** This parameter file exploits the "currentDir" key to set the current working directory and the "env" key to set the `KHIOPS_API_MODE` and `KhiopsFastExitMode` environnement variables.

#### 6.2) In Visual Studio Code
- Make sure you have installed the extension mentioned [above](#system-requirements).
- Open VS Code go to the menu **File -> Open Folder** and select your local `khiops` folder
- At the bottom bar you can select a debug or release preset (ex: _Windows MSVC Release_) and then click on the "⚙ Build" button nearby.

#### 6.3) From the command line
- Go to the `khiops` folder
- For a given preset (ex: `windows-msvc-release`) execute
```bash
cmake --preset windows-msvc-release         # add --fresh to regenerate
cmake --build --preset windows-msvc-release # add --clean-build to clean
```

