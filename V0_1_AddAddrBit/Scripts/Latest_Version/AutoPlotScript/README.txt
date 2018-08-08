A special package is needed to write excel files, I've installed this package through miniconda

1. Setup the python environment
    $ bash Miniconda2-latest-Linux-x86_64.sh
    Follow the instructions, if you do not want to add this python to your PATH in .bashrc (become the default python), I've made a "env_setup.sh" that will add this version of python to PATH when called
2. Install the xlsxwriter package
    $ conda install xlsxwriter
3. Run the script to collect data
    $ python example_script.py

Input_Path.py contains the definition of functions that will generate the paths of input files following a certain rule. An example function is given for getting the stat files of the 256KB L2, pfoff for 1P2L and 2P2L, pfon and pfoff for baselines runs. Please duplicate/modify to get support for more input files. Note that if there is not a simple rule for input files, it is possible to define each one of them seperately as well.

Stat_Parser.py contains the definition of functions that will get the value of a corresponding statistic from the input files. Simple calculation between stats is also defined here, with normalization given as an example.

Generate_Plots.py contains the definition of functions that generates an excel file, puts the collected stats inside, and generate plots for the stats. Only clustered column bar chart is being generated for now, it is possible to add other types of charts as well.

example_script.py is an example of the executable script. The different configurations and applications should be defined here, the statistics to collect and plot are also defined here. This example will collect the data for the pfon and pfoff DRAM and NVM baseline runs, pfoff 1P2L and 2P2L runs.
