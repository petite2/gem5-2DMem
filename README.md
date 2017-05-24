# README #

### What is this repository for? ###

#### Quick summary

This repository is for a modified version of gem5 to support 2D memory. 2D memory is a memory where the data can be accessed in either a row or a column.

#### Version

Ummm... Let's say V0.0 for now.

#### [Learn Markdown](https://bitbucket.org/tutorials/markdowndemo)

A tutorial for editing readme.

### How do I get set up? ###

#### Summary of set up

Follow the documentations of gem5 at http://gem5.org/Documentation.
To get DRAMSim2 support, the "filePath" variable in DRAMSim2.py should be changed to your path that points to the DRAMSim2 source file (absolute path is recommended, a relative path is relative to where the executable is launched).

#### Dependencies

Follow the documentations of gem5 at http://gem5.org/Dependencies. Most important ones are "scons" and "swig" to build gem5.

To use NVMain, a copy of NVMain is also required. See http://wiki.nvmain.org/.

Modified DRAMSim2 is already included in the repository, see http://eng.umd.edu/~blj/dramsim/ for original code.

#### Build

Follow the directions of gem5.
 
An example: Go to the gem5 root directory, and build with the command line

    % scons -j 4 build/X86/gem5.opt

* "X86" is the architecture, can also be ALPHA, ARM, or other architectures that gem5 supports. But this project has only been tested on X86.

* "opt" signifies the mode of executable. Can be "fast" for faster execution without debug information, "opt" for moderately fast execution with some debug information. See http://gem5.org/Build_System for detail.

Note, the target name "gem5.opt", "gem5.fast" etc. should follow the given options from gem5. The target can be renamed afterwards, but cannot have other names for build.

To build with NVMain (still testing), go to the gem5 root directory, and build with the command line option "EXTRAS=path/to/nvmain".
An example:

    % scons -j 4 build/X86/gem5.opt EXTRAS=path/to/nvmain

#### How to run

Follow the instructions on http://gem5.org/Running_gem5. Currently, only Syscall Emulation (SE) mode has been tested. 

Generally, the command line should be:

    % <gem5 binary> [gem5 options] <simulation script> [script options]

An example of command line used for testing: go to gem5's root directory,

    % ./build/X86/gem5.opt --redirect-stdout --stdout-file=stdout.txt --redirect-stderr --stderr-file=stderr.txt --stats-file=stats.txt ./configs/example/se.py --output=app_stdout.txt --errout=app_stderr.txt -c path/to/test/program/test-bin --caches --l2cache --cpu-type=detailed --MJL_row_width=8 --MJL_PC2DirFile=test.txt --mem-size=8192MB -o "1 1 32"

* The options "--redirect-stdout", "--stdout-file", "--redirect-stderr", "--stderr-file", "--stats-file" are gem5 options. The "--redirect" options redirects the gem5 output to files, while the "-file" option defines the file name.

* A "m5out" directory should be created in the current directory. The gem5 output files defined by "--stdout-file", "--stderr-file", "--stats-file" can be found in this directory. A "-d" option can also be given to specify a directory other than the current one to put m5out and the output files. If paths exist in the file name, this will be relative to the current directory or the directory given by that of "-d" option if it exists.

* "configs/example/se.py" is an example of a script for SE mode.

* The "-c", "--output", "--errout" and "-o" options are standard gem5 script options for SE mode. The argument that follows "-c" specifies the application program to run on gem5. And the argument that follow "-o" are the inputs of the application. "--output" redirects the application stdout to specified file, and "--errout" redirects the application stderr to specified file. See http://gem5.org/Running_gem5 for help.

* The "--caches", "--l2cache", "--cpu-type", "--mem-size" are standard gem5 script options for system configuration. See http://gem5.org/Running_gem5 for help. The file gem5/common/Options.py is also a place to look at for help.

* The "--MJL_row_width" and "--MJL_PC2DirFile" are additional command line options added for 2D memory. These will be introduced in more detail in the next section.

#### Additional command line options

Information of these options can also be found using help or in the common/example/Options.py file.

* "--MJL_row_width" specifies the size of a row in number of cache lines.

* "--MJL_PC2DirFile" specifies the filename of the input file with mapping of PC to access direction, row or column (currently untested). If the file is not found, then all accesses use the default direction.

* "--MJL_default_column" sets default access direction preference of all data accesses to column. When not used, the default preference is row.


#### Contribution guidelines ####

No idea.

* Writing tests
* Code review
* Other guidelines

#### Who do I talk to? ####

* Repo owner or admin

Email Minli.Julie.Liao@gmail.com

* Other community or team contact

I'll ask for the email they want to put here first.