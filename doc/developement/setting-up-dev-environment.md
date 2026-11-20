# Setting Up the Development Environment

## System Requirements
- An IDE with support for C++ and CMake
  - [Visual Studio 2026](https://visualstudio.microsoft.com/vs/)
    - Make sure you enable the C/C++ development tools during installation.
      - 2022 version: OK
      - 2026 version: Since version 2026, install only the **English** linguistic module
        to avoid numerous and noisy messages like `Remarque : inclusion du fichier : ...`
    - Install the "Format document on Save" extension.
    - Disable Include Cleanup (since Visual Studio 17.8+) to avoid numerous, noisy warnings about include policies.
      - From the main menu of Visual Studio, choose Tools → Options...
      - In the Options dialog, use the search box (top-left) and type: include cleanup
      - Three options will appear. Set all to "None":
         - "Choose how to highlight #include statements that are unused"
         - "Choose how to highlight unused #include statements that can be optimized"
         - "Choose how to highlight missing #include statements that are transitively included"
  - [Visual Studio Code](https://code.visualstudio.com/)
    - Also install [C/C++ Extension Pack](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools-extension-pack)
    - On Windows, also install [Build Tools for Visual Studio](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022).

- A Java Development Kit (JDK)
  - [Eclipse Temurin (OpenJDK)](https://adoptium.net/temurin/releases/)
- An MPI development kit:
  - Windows: [Intel MPI](https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library-download.html)
  - Linux and macOS: [Open MPI](https://www.open-mpi.org)
- Bison and Flex development tools (optional)
  - Needed only if the Lex and Yacc files in `src/Learning/KWData` need to be recompiled for Khiops dictionary analysis
  - Windows:
    - Install the latest version of [win bison and flex](https://github.com/lexxmark/winflexbison/releases).
    - Add the directory containing the Bison and Flex binaries to `PATH`.
    - Optional: cohabitation with [Gow - The lightweight alternative to Cygwin](https://github.com/bmatzelle/gow).
      - If Gow is installed, its Bison and Flex binaries may be used instead.
      - To fix this, rename `flex.exe` to `NO_flex.exe` and `bison.exe` to `NO_bison.exe` in the Gow `bin` directory.

- A Git installation
  - Windows: [Git for Windows](https://gitforwindows.org/)
    - Includes Git Bash and Terminal
- A Python distribution with `pip` installed
  - For example: [Anaconda Python Distribution](https://anaconda.org)

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

### 2) Configure an authentication method for GitHub

#### Option 1: SSH
- Create and configure a public/private key pair.
```bash
# Use Git Bash in Windows
ssh-keygen -t ed25519 -C "your@email.com"
```
- This creates the files `id_ed25519` and `id_ed25519.pub` in `~/.ssh`. If they are created elsewhere, move them to `~/.ssh`.
- Copy the contents of the public key file: `~/.ssh/id_ed25519.pub`.
- In GitHub, click your profile icon and select **Settings**. Go to the **SSH and GPG keys** section and select **New SSH key**.
- Paste the contents of your public key into the text field.


> [!NOTE] Problems through proxies
> If you have problems connecting through a proxy (`Connection timeout` or `Connection refused`), configure [SSH over HTTPS](https://docs.github.com/en/authentication/troubleshooting-ssh/using-ssh-over-the-https-port).

#### Option 2: HTTPS
- Create an access token.
  - Click your profile icon, then select **Settings** > **Developer Settings** > **Personal Access Tokens** > **Fine-grained Tokens**.

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

# These commands install the hooks in the repository and run them on all files.
# It may take a bit of time the first time
pre-commit install
pre-commit run --all-files
```

 > [!NOTE] for Anaconda on Windows
 >You must add the `Scripts` directory of your Anaconda installation to the `PATH` so that `pre-commit` is available in any shell. With conda, you can run `conda install -c conda-forge identify pre-commit`.
### 5) Install MPI
#### On Windows:

1. Download Intel MPI from [Intel oneAPI](https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library-download.html). Choose the "offline installer" and install it.
2. Set the `I_MPI_ROOT` environment variable to `C:\Program Files (x86)\Intel\oneAPI\mpi\latest`.
3. Add `%I_MPI_ROOT%\bin` to the `Path` environment variable:
    ```cmd
       set path=%path%;%I_MPI_ROOT%\bin
    ```
4. Uninstall MS-MPI or unset the MS-MPI variables: `MSMPI_BIN`, `MSMPI_INC`, `MSMPI_LIB32`, and `MSMPI_LIB64`.
5. Remove the `build` directory from the project directory (e.g., `c:\dev\khiops\build`).

#### On macOS:
```bash
brew install openmpi
```
#### On Linux:
```bash
sudo apt-get install openmpi-bin libopenmpi-dev
```
or
```bash
sudo dnf install openmpi openmpi-devel
```

### Compile

#### In Visual Studio 2026
- Open Visual Studio, choose **Open Local Folder**, and select your local `khiops` folder.
  - This may take a few minutes to complete. You can monitor the progress by selecting the `CMake` output in the output window.
- In the top bar, select a debug or release preset (e.g., _Windows MSVC Release_), then choose **Build > Build All**.

To run the MODL or MODL_Coclustering executable from the IDE, you must have a `launch.vs.json` parameter file in the `.vs` directory. It contains for example:

```json
  "version": "0.2.1",
  "defaults": {},
  "configurations": [
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "MODL.exe (bin\\MODL.exe)",
      "name": "MODL.exe (bin\\MODL.exe)",
      "currentDir" : "C:\\MyData\\LearningTest\\TestKhiops\\Standard\\Iris",
      "args": [
        "-b",
        "-i",
        "test.prm",
        "-e",
        "./results/err.txt"
      ],
      "env":{"KHIOPS_API_MODE":"true", "KhiopsFastExitMode":"false"}
    },
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "MODL_Coclustering.exe (bin\\MODL_Coclustering.exe)",
      "name": "MODL_Coclustering.exe (bin\\MODL_Coclustering.exe)",
      "currentDir" : "C:\\MyData\\LearningTest\\TestCoclustering\\Standard\\Iris",
      "args": [
        "-b",
        "-i",
        "test.prm",
        "-e",
        "./results/err.txt"
      ],
      "env":{"KHIOPS_API_MODE":"true", "KhiopsFastExitMode":"false"}
    },
    {
      "type": "default",
      "project": "CMakeLists.txt",
      "projectTarget": "KNITransfer.exe (bin\\KNITransfer.exe)",
      "name": "KNITransfer.exe (bin\\KNITransfer.exe)",
      "args": [
        "-b",
        "-i",
        "test.prm",
        "-e",
        "./results/err.txt"
      ],
      "currentDir" : "C:\\MyData\\LearningTest\\TestKNI\\Standard\\Iris",
      "env":{"KHIOPS_API_MODE":"true", "KhiopsFastExitMode":"false"}
    }
  ]
}
```
**Note:** This parameter file uses the `currentDir` key to set the current working directory and the `env` key to set the `KHIOPS_API_MODE` and `KhiopsFastExitMode` environment variables.

#### In Visual Studio Code
- Make sure you have installed the extension mentioned [above](#system-requirements).
- Open VS Code, choose **File > Open Folder**, and select your local `khiops` folder.
- In the status bar, select a debug or release preset (e.g., _Windows MSVC Release_), then click the nearby **⚙ Build** button.

#### From the command line
- Go to the `khiops` folder
- For a given preset (e.g., `windows-msvc-release`), run:
```bash
cmake --preset windows-msvc-release         # add --fresh to regenerate
cmake --build --preset windows-msvc-release # add --clean-build to clean
```

