def generate_pbs(cluster_name, benchmark_name, config_name, config_folder_name, config_options, output_suffix):
    job_name = config_name + "_" + benchmark_name + output_suffix
    job_queue = cluster_name + "@shai"

    #### Simulator and output specification ####
    project_prefix = "/home/grads/mjl5868/mdl_mjl5868/GEM5/Mem2D/New_Version/2dmem_v0_1_addaddrbit/V0_1_AddAddrBit"
    Program_location = project_prefix + "/gem5/build/X86/gem5.opt_Aug03_2018"
    Simulator_filename = project_prefix + "/gem5/configs/example/se.py"

    config_dir = project_prefix + "/Run_test_pbs/DirPredict_3GHz/" + config_folder_name
    dump_dir = config_dir + "/m5out/"
    stdout_filename = benchmark_name + "/" + benchmark_name + "_" + config_name + ".gemout" + output_suffix
    stderr_filename = benchmark_name + "/" + benchmark_name + "_" + config_name + ".gemerr" + output_suffix
    appout_filename = config_dir + "/appout/" + benchmark_name + "_" + config_name + ".appout" + output_suffix
    apperr_filename = config_dir + "/appout/" + benchmark_name + "_" + config_name + ".apperr" + output_suffix
    stats_filename = benchmark_name + "/" + benchmark_name + "_" + config_name + ".stat" + output_suffix
    # Output_dump_directory = pbs_o_workdir + "/m5out/" + benchmark_name ### stats, config, and other written files will be dumped here, CHECKPOINTS are here too
    pbs_o_workdir = config_dir + "/run/" + benchmark_name

    #### Generated file's filename ####
    OutFilename = job_name + ".pbs"

    #### Don't change ####
    Content = \
    "#!/bin/sh\n" + \
    "#PBS -l nodes=1:ppn=1\n" + \
    "#PBS -l walltime=96:00:00\n" + \
    "#PBS -q " + job_queue + "\n" + \
    "#PBS -N " + job_name + "\n" + \
    "#PBS -m ae\n" + \
    "#PBS -M mjl5868@cse.psu.edu\n" + \
    "#\n" + \
    "\n" + \
    "PROG=" + Program_location + "\n" + \
    "ARGS=(-d " + dump_dir + " --redirect-stdout --stdout-file=" + stdout_filename + " --redirect-stderr --stderr-file=" + stderr_filename + " --stats-file=" + stats_filename + " " + Simulator_filename + " --output=" + appout_filename + " --errout=" + apperr_filename + " " +  config_options + ")\n" + \
    "#\n" + \
    "export LD_LIBRARY_PATH=/home/software/gcc/gcc-4.8.3/lib:/home/software/gcc/gcc-4.8.3/lib64:$LD_LIBRARY_PATH\n" + \
    "cd " + pbs_o_workdir + "\n" + \
    "# run the program\n" + \
    "#\n" + \
    "$PROG \"${ARGS[@]}\"\n" + \
    "exit 0\n\n"

    #### Generate pbs file ####
    # print Content
    fobj = open(OutFilename, "w")
    fobj.write(Content)
    fobj.close()



# app_binary_filename = "/home/mdl/sug241/projects/Minli/micro/binary_chk/Vec_O3_chk" + ...
