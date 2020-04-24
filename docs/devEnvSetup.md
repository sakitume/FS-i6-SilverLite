
# Setting up your development environment
This page will walk you through the steps of setting up your development environment so that you can build and deploy this project onto your FlySky FS-i6 transmitter.

For editing, compiling and flashing the firmware you have two choices:

* Install and use [MCUXpresso Integrated Development Environment (IDE)](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE)
* Install the indvidual tools/systems (arm compiler toolchain, make, openocd, vscode, etc) and configure them accordingly.

> Note: I once had this project set up to also use Keil MDK (free evaluation version) but the code size of this project now exceeds the 32k limit of Keil. The project files are still in the repo and can probably be updated to work (assuming you have an upgraded Keil without the size limit).

## The easy way
Installing and using MCUXpresso is by far the simplest solution. It is "A free-of-charge, code size unlimited, easy-to-use IDE for Kinetis and LPC MCUs, and i.MX RT crossover MCUs". Visit the NXP website [here](https://www.nxp.com/design/software/development-software/mcuxpresso-software-and-tools/mcuxpresso-integrated-development-environment-ide:MCUXpresso-IDE) to download the latest version.

> Note: You will need to register with NXP in order to download MCUXpresso.

MCUXpresso is based on Eclipse with various plugins preinstalled and customized. The repo contains a `.project` and a `.cproject` file that MCUXpresso should be able to open.
I'll leave it to you to figure out how to use MCUXpresso. With it you should be able to build (compile) the project, flash the firmware (via ST-Link or J-Link), and even debug the code.

## The hard way

The other choice (installing the discrete tools) is a lot more work but something I feel is worth it. By doing so, it sets up your development computer so that you can develop for many more platforms and hardware devices. 

The tools/systems you'll be installing will consist of:
* A code editor (Visual Studio Code is what I'll suggest and describe here)
* A compiler toolchain
* `make` as your build system
* OpenOCD. A software tool for flashing firmware via ST-Link adapter

It should be possible to develop on Mac OS, Linux and Windows platforms. This is due to using a variety of open source tools and technologies that are supported on all of these operating systems.

> Note: The specific examples provided here will often be written from the perspective of using a Windows development PC. I hope to update this document in the future to provide more specific examples for Mac OS and Linux platforms.

## Developing with VSCode

I really like Visual Studio Code. It's readily available for Mac OS, Windows and Linux. Plus it's very fast and extensible!
I use it not only for editing the code, but also the documentation. And because it has integrated terminal windows I also
use it for building and monitoring.

It can be an excellent (and lightweight) development envrironment for embedded devices. I've used it with great success when
developing for ESP8266, ESP32, Arduino (AVR), STM32, etc. I often use it with Platform IO (but not so with this project).

I'll describe how to set up VSCode and associated tools so that we can develop for the (ARM cortex m0+ based) Kinetis KL16 micro used
on the FlySky FS-i6 transmitter.

> Note: Some of what is described here was learned from this (most excellent) article: ["Using Visual Studio Code with STM32CubeMX for ARM Development"](https://hbfsrobotics.com/blog/configuring-vs-code-arm-development-stm32cubemx). It's worth the time to read it for yourself.


## What we'll need
Here are the list of software tools we'll need for this project.

* [ARM gnu toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
* [mingw-w64](https://sourceforge.net/projects/mingw-w64/) (if you will be developing under Windows)
    * This is needed solely for the `make` tool, or more specifically `mingw32-make.exe`
* [OpenOCD](https://gnutoolchains.com/arm-eabi/openocd/)
* [Visual Studio Code](https://code.visualstudio.com/), and the following extensions:
    * [Cortex Debug](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug)
    * [C/C++ for Visual Studio Code](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)
    * [Project Manager](https://marketplace.visualstudio.com/items?itemName=alefragnani.project-manager)


> Note: I've tried using the [STM32 ST-LINK utility](https://www.st.com/en/development-tools/stsw-link004.html) as well as the open source version from [texane](https://github.com/texane/stlink) but was not able to establish a connection using the ST-Link V2 debugger/programmer I have. Thankfully I was able to get OpenOCD working.

You will also need some hardware in order to connect your development PC to your FlySky FS-i6 transmitter. I'm using a (cheapie clone) ST-Link V2 programmer/debugger
purchased from [Amazon](https://www.amazon.com/gp/product/B01EE4WAC8/>) but also readily available on [ebay](https://www.ebay.com/). 

While working on a previous development project I had installed the "ST-Link Utility" from here:
<https://www.st.com/en/development-tools/stsw-link004.html>

I'm pretty sure that's how I ended up getting the st-link drivers installed onto my Windows 10 development machine. I also ended up updating
the firmware on my ST-Link V2 probe using that utility but I'm not sure if that was necessary. I'm noting it here in case it may be useful.

If you don't already have drivers installed for your ST-Link V2 probe, you may be able to use the drivers provided by the OpenOCD installation (described later).


## Installing the tools
If you don't already have VScode installed on your machine please visit:
<https://code.visualstudio.com/>

Follow the install directions. Afterwards we'll need to install some extensions
and also configure VSCode slightly.

After installing Visual Studio Code, launch it and navigate to Extensions tab or press `Ctrl+Shift+X`.

Install the following extensions:

* C/C++ – <https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools>
* Cortex Debug – <https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug>
* Project Manager – <https://marketplace.visualstudio.com/items?itemName=alefragnani.project-manager>

Press `F1` key and type `open settings json`. Select `Open Settings (JSON)`. In the opened file, insert the following property at the top of the list and save.

```
"cortex-debug.armToolchainPath": "${env:VSARM}\\armcc\\bin\\",
```

For example my `settings.json` ends up looking like this:
```json
{
    "cortex-debug.armToolchainPath": "${env:VSARM}\\armcc\\bin\\",
    "terminal.integrated.rendererType": "dom",
    "terminal.integrated.shell.windows": "C:\\Program Files\\Git\\bin\\bash.exe",
    "platformio-ide.forceUploadAndMonitor": true
}
```

> Note: The `${env:VSARM}` is refrencing an enviornment variable that we'll create shortly

For the remaining tools I recommend installing them into into a single folder. On my Windows machine I use `C:\Tools`. You could also choose to use `C:\VSARM`. Keep it simple and avoid having spaces in the path. 

On a Mac or Linux machine you could create a subfolder in your home folder.

```
mkdir ~/vsarm
```


Create an environment variable named `VSARM` and set its value to this folder.

For example on my Mac I added this line to my `~/.zshrc` file:

```
# VSARM
export VSARM="$HOME/vsarm"
```

> Note: If you use `bash` (the default shell for Mac OS) then you would instead add that to your `~/.bash_profile` file.

### Install arm gnu toolchain

Go to: <https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads>
Download the appropriate installer. There are several available. 

#### Windows installation
For my Windows development PC I chose:

```
gcc-arm-none-eabi-8-2019-q3-update-win32-sha2.exe
Windows 32-bit Installer (Signed for Windows 7 and later)
MD5: d44f44b258b203bdd6808752907754be
```

Run the installer. When it prompts you to "Choose Install Location" I changed the default shown in "Destination Folder"
from:

```
C:\Program Files (x86)\GNU Tools ARM Embedded\8 2019-q3-update
```
to 
```
C:\Tools\armcc
```

Make sure these gnu tools are in your path. More specifically you will want to add this to your path:
```
C:\Tools\armcc\bin
```

#### Mac OS installation

For my old MacBook I chose to download this version:

```
Mac OS X 64-bit
File: gcc-arm-none-eabi-8-2019-q3-update-mac.tar.bz2 (105.72 MB)
```

Using a Finder window I then double-clicked on the downloaded `.bz2` file which unpacked the contents into a new folder named:

```
gcc-arm-none-eabi-8-2019-q3-update
```

I then renamed that folder to `armcc` and then moved this `armcc` folder into my `~/vsarm` folder.

Finally I adjusted my path by editing my `.zshrc` file and added this to the bottom of it:

```
# vsarm tools
export PATH="$HOME/vsarm/armcc/bin:$PATH"

```

> Note: If you use `bash` for your shell (which is the default for Mac OS) then add the above to your `~/.bash_profile` instead.


### Install MinGW-W64 (for Windows machines only)

This step is only needed for Windows development machines.

We need MinGW-W64 for the `mingw32-make.exe` program. If you have already have a `make` utility on your machine then you could skip this

Go to: <https://sourceforge.net/projects/mingw-w64/>
Change the install location to:

```
C:\Tools\mingw
```

Make sure the MinGW-W64 tools are in your path. More specifically you will want to add this to your path:
```
C:\Tools\mingw\mingw32\bin
```

### Installing OpenOCD on Windows

For Windows users I recommend obtaining a version of OpenOCD from here: <https://gnutoolchains.com/arm-eabi/openocd/>

Download the most recent 7zip file: `openocd-20190828.7z`
Decompress it into `C:\Tools` folder. Rename the resultant `OpenOCD-20190828-0.10.0` folder to just `OpenOCD`.

Make sure the OpenOCD tools are in your path. More specifically you will want to add this to your path:

```
C:\Tools\OpenOCD\bin
```

> Note: There are several binary versions of OpenOCD available for installation. The one from SysProgs (gnutoolchains.com)
seems to be updated regularly and also provides drivers that may be helpful to you.

### Installing OpenOCD on Mac OS
While it may be possible to install a prebuilt version of OpenOCD using Homebrew, I've learned that 
the OpenOCD project maintainers recommend Mac/Linux users to build it themselves using the latest version
of the source code available from the repository. 

> Note: We will be using the "Cortex Debug" extension for VSCode. The `README.md` file from `https://github.com/Marus/cortex-debug` mentions not using the default version of OpenOCD provided by Homebrew. So...even more reason for us to build it from source.

So to be safe, we'll use Homebrew to ***build*** and install OpenOCD using the latest available source code. Using Terminal, enter the following:

```
brew install open-ocd --HEAD
```

This took a little while to complete, but once it finished I found it was immmediately available for use at `/usr/local/bin/openocd`. No adjustments to my path were needed.


# Setup Visual Studio Code Project
This git repo already contains the necessary configurations we need (see the various files in the `.vscode` folder). 

Howver I'm going to document how I created these configs
so that future adjustments can be performed (such as adding additional source/include folders, etc).

Note: For more detailed information about these configs visit: 

* <https://code.visualstudio.com/docs/editor/variables-reference>
* <https://code.visualstudio.com/docs/cpp/config-msvc>
* <https://code.visualstudio.com/docs/cpp/c-cpp-properties-schema-reference>


## Configure C/CPP
From within VSCode press `F1` key and type `edit config` and choose `C/CPP: Edit Configurations (JSON)`. The default contents of this file will look something like this:

```json
{
    "configurations": [
        {
            "name": "Win32",
            "includePath": [
                "${workspaceFolder}/**"
            ],
            "defines": [
                "_DEBUG",
                "UNICODE",
                "_UNICODE"
            ],
            "windowsSdkVersion": "10.0.17763.0",
            "compilerPath": "C:/Program Files (x86)/Microsoft Visual Studio/2017/Community/VC/Tools/MSVC/14.16.27023/bin/Hostx64/x64/cl.exe",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "msvc-x64"
        }
    ],
    "version": 4
}
```

Replace all of this and enter the following:
```json
{
    "configurations": [
        {
            "name": "Debug",
            "includePath": [
                "${env:VSARM}/armcc/arm-none-eabi/include/c++/8.3.1",
                "${env:VSARM}/armcc/arm-none-eabi/include/c++/8.3.1/arm-none-eabi",
                "${env:VSARM}/armcc/arm-none-eabi/include/c++/8.3.1/backward",
                "${env:VSARM}/armcc/lib/gcc/arm-none-eabi/8.3.1/include",
                "${env:VSARM}/armcc/lib/gcc/arm-none-eabi/8.3.1/include-fixed",
                "${env:VSARM}/armcc/arm-none-eabi/include",

                "${workspaceFolder}/**"
            ],
            "defines": [
                "DEBUG"
            ],
            "intelliSenseMode": "clang-x64",
            "browse": {
                "path": [
                    "${workspaceFolder}",
                    "${env:VSARM}/armcc"
                ],
                "limitSymbolsToIncludedHeaders": false,
                "databaseFilename": ""
            }
        }
    ],
    "version": 4
}
```

Be sure to adjust the arm gnu folders such as: `${env:VSARM}/armcc/arm-none-eabi/include/c++/8.3.1` to match the version
you actually have installed (hint: the `8.3.1` might need to be changed).

For the `"includePath"` property you could replace the `"${workspaceFolder}/**"` wildcard with actual folders (like: `"${workspaceFolder}/include"`) if you wish.

For the `"defines"` property you should add any entries your project may require (examples: `"SOME_VALUE=0x8000"`, `"USE_SOMETHING"`, etc).

To create a "Release" configuration you could duplicate the above config, change the `"name"` property to `"Release"`, change the `"DEBUG"` define to `"NDEBUG"`, etc.

## Configure VSCode Tasks
From within VSCode press `F1` key and type `config task` and choose `Tasks: Configure tasks`. Click on `Create tasks.json file from template` and select the `Other` option. The `tasks.json` file will open. The default contents of this file will look something like this:

```json
{
    // See https://go.microsoft.com/fwlink/?LinkId=733558
    // for the documentation about the tasks.json format
    "version": "2.0.0",
    "tasks": [
        {
            "label": "echo",
            "type": "shell",
            "command": "echo Hello"
        }
    ]
}
```

Replace the contents of this file with the following:

```json
{
    // See https://go.microsoft.com/fwlink/?LinkId=733558
    // for the documentation about the tasks.json format
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Clean Debug",
            "type": "shell",
            "command": "make DEBUG=1 clean",
            "windows": {
                "command": "mingw32-make.exe DEBUG=1 clean"
            },
            "options": {
                "cwd": "${workspaceRoot}"
            },
            "group": "build",
            "problemMatcher": []
        },
        {
            "label": "Clean Release",
            "type": "shell",
            "command": "make DEBUG=0 clean",
            "windows": {
                "command": "mingw32-make.exe DEBUG=0 clean"
            },
            "options": {
                "cwd": "${workspaceRoot}"
            },
            "group": "build",
            "problemMatcher": []
        },
        {
            "label": "Make Debug Firmware",
            "type": "shell",
            "command": "make -j12 DEBUG=1",
            "windows": {
                "command": "mingw32-make.exe -j12 DEBUG=1"
            },
            "options": {
                "cwd": "${workspaceRoot}"
            },
            "group": "build",
            "presentation": {
                "reveal": "always",
                "clear": true
            },
            "problemMatcher": []
        },
        {
            "label": "Make Release Firmware",
            "type": "shell",
            "command": "make -j12 DEBUG=0",
            "windows": {
                    "command": "mingw32-make.exe -j12 DEBUG=0"
            },
            "options": {
                "cwd": "${workspaceRoot}"
            },
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "presentation": {
                "reveal": "always",
                "clear": true
            },
            "problemMatcher": []
        },
        {
            "label": "OpenOCD Flash Debug Firmware",
            "type": "shell",
            "command": "openocd -f interface/stlink.cfg -f target/klx.cfg -c \"program fs-i6.elf verify reset exit\"",
            "options": {
                "cwd": "${workspaceRoot}/gcc_debug"
            },
            "group": "build",
            "problemMatcher": []
        },
        {
            "label": "OpenOCD Flash Release Firmware",
            "type": "shell",
            "command": "openocd -f interface/stlink.cfg -f target/klx.cfg -c \"program fs-i6.elf verify reset exit\"",
            "options": {
                "cwd": "${workspaceRoot}/gcc_release"
            },
            "group": "build",
            "problemMatcher": []
        },
        {
            "label": "JLink Flash Release Firmware",
            "type": "shell",
            "windows": {
                "options": {
                    "shell": {
                        "executable": "cmd.exe",
                        "args": [
                            "/d", "/c"
                        ]
                    }
                },
                "command": "JLink -CommanderScript ../.vscode/load-release.jlink",
            },
            "options": {
                "cwd": "${workspaceRoot}/gcc_release",
            },
            "group": "build",
            "problemMatcher": []
        },
        {
            "label": "JLink Flash Debug Firmware",
            "type": "shell",
            "windows": {
                "options": {
                    "shell": {
                        "executable": "cmd.exe",
                        "args": [
                            "/d", "/c"
                        ]
                    }
                },
                "command": "JLink -CommanderScript ../.vscode/load-release.jlink",
            },
            "options": {
                "cwd": "${workspaceRoot}/gcc_debug",
            },
            "group": "build",
            "problemMatcher": []
        }
    ]
}
```

> Note: Be sure to adjust the `-j` parameter to the `make` if needed.

To run any of these tasks you can use `Ctrl-Alt-T` on Windows, or `Ctrl-Option-T` on Mac to bring up a menu that allows you to choose any of these tasks we've just defined. Alternatively you can use `F1` key and type `run task` and choose `Tasks: Run Task`.

## Configure VSCode shell

**One very important note:** The makefile I'm using uses `rm -rf` as part of building the `clean` target. This `rm` command isn't normally available on windows. So I've configured my VSCode environment to use a `bash` shell whenever it needs to provide a shell for any command.

This `bash` shell came with my [Git for Windows](https://gitforwindows.org/) installation. If you have something similar, then you'll want to configure VSCode to use such a shell. Alternatively you can edit the makefile(s) in this project folder.

To configure VSCode to use a particular shell you need to edit your `settings.json` file. Details are provided here: <https://code.visualstudio.com/docs/editor/integrated-terminal>. 

Basically you need to enter a configuration line like ***one*** of the following:

```json
// Command Prompt
"terminal.integrated.shell.windows": "C:\\Windows\\System32\\cmd.exe"
// PowerShell
"terminal.integrated.shell.windows": "C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\powershell.exe"
// Git Bash
"terminal.integrated.shell.windows": "C:\\Program Files\\Git\\bin\\bash.exe"
// Bash on Ubuntu (on Windows)
"terminal.integrated.shell.windows": "C:\\Windows\\System32\\bash.exe"
```

Obviously you should use one of the `bash` options in the above examples. Personally I prefer the `bash` from my `git` install versus the one from my Windows Subsystem for Linux (WSL) since it loads up much faster.

## Configure Debugger
From within VSCode use `F1` key, type `launch` and select `Debug: Open launch.json` and choose the `Cortex Debug` option. The `launch.json` file will open with contents that should look something like this:

```json
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Cortex Debug",
            "cwd": "${workspaceRoot}",
            "executable": "./bin/executable.elf",
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "jlink"
        }
    ]
}
```

Replace it with this:
``` json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Launch Debug config",
            "type": "cortex-debug",
            "request": "launch",
            "servertype": "openocd",
            "cwd": "${workspaceRoot}",
            "executable": "./Debug/i6.elf",
            "device": "Kinetis",
            "svdFile": "${workspaceRoot}/.vscode/MKL16Z4.svd",
            "configFiles": [
                "interface/stlink.cfg",
                "target/klx.cfg",
            ]
        },
        {
            "name": "Attach Debug config",
            "type": "cortex-debug",
            "request": "attach",
            "servertype": "openocd",
            "cwd": "${workspaceRoot}",
            "executable": "./Debug/i6.elf",
            "device": "Kinetis",
            "svdFile": "${workspaceRoot}/.vscode/MKL16Z4.svd",
            "configFiles": [
                "interface/stlink.cfg",
                "target/klx.cfg",
            ]
        }
    ]
}
```

Some useful info about these configs can be found here: <https://marcelball.ca/projects/cortex-debug/cortex-debug-launch-configurations/>

After these additions have been made you can click on the Debug icon on the left of VSCode and be able to execute one of these configurations. 

Regarding the `.svd` file in the above config. A Google search for `kinetis svd files` led me to this: <https://github.com/posborne/cmsis-svd/issues/29>
which in turn led me to this: <https://github.com/hackrid/KDS_SVD/blob/master/KDS_3_2/MKL16Z4.svd>. 

### "semihosting"
It is possible to direct `printf` messages (`stdout`) to the debug interface. This capability is known as *semihosting*.

> Note: I discovered this via Google and reading about this: <https://dzone.com/articles/semihosting-gnu-arm-embedded> and also <https://bgamari.github.io/posts/2014-10-31-semihosting.html>

I have not yet attempted using this feature but will investigate this soon.

## Add "Shortcuts" extension to VSCode
The ["Shortcuts"](https://marketplace.visualstudio.com/items?itemName=gizak.shortcuts) extension will install clickable icons on the bottom status bar of VSCode. Each icon is a shortcut to a vscode command.

Setting this up was tricky. There's no clear documentation that I could find. 
Press `F1` key and type `preferences workspace`. Select `Preferences: Open Workspace Settings`. Enter `shortcuts` in the search bar. Hover your mouse cursor over "Shortcuts: Buttons" and a gear icon will appear. Click on the gear icon and choose "Copy Setting as JSON" (this will ), then click on "Edit in settings.json". Position the cursor between the curly brackets and then paste (`Ctrl+V`/`Cmd+V`) the copied setting into the file. 

The file should look like this:
```
"shortcuts.buttons": [
  "file-binary , workbench.action.tasks.build , Run build task",
  "beaker , workbench.action.tasks.test , Run test task",
  "terminal , workbench.action.terminal.toggleTerminal , Toggle terminal panel",
  "telescope , workbench.action.showCommands , Show command palette",
  "bug, workbench.action.debug.start, Launch debug"
]
```

This will create 5 new icons on the bottom status bar. I'd prefer to customize these to just 3. So replace the above contents with the following:

```
"shortcuts.buttons": [
  "file-binary , workbench.action.tasks.build , Run Build Task",
  "rocket , workbench.action.tasks.runTask , Run Task",
  "bug, workbench.action.debug.start, Launch Debug"
]
```

The first icon ("file-binary") should launch the "Make Debug Firmware" task (because that is the only one marked as the default build task). The second icon will execute the "Tasks: Run Task" command which reveals a dropdown menu of the available tasks. And the third icon will launch a debugging session.
