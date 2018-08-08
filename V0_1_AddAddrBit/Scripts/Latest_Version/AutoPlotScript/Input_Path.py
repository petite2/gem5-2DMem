import argparse
import os.path

# Example of command line arguments that could be used to define the behaviour of the functions
parser = argparse.ArgumentParser(description="Define input file path templates")
parser.add_argument("--common_prefix", default="", help="Common path prefix to all input files")
parser.add_argument("--config_path_template", default="", help="Configuration specific path template")
parser.add_argument("--app_filename_template", default="", help="Application specific file name template")

args = parser.parse_args()

'''
Generate the correct input file path for each configuration and application combination
Args:
    config_list: The list of names of configurations
    app_list: The list of names of applications
Returns:
    input_file_paths: The dict that has the existing input file names for each config and app combination
'''
def mjl_get_input_file_paths(config_list, app_list):
    common_prefix = "/home/mdl/mjl5868/GEM5/Mem2D/New_Version/2dmem_v0_1_addaddrbit/V0_1_AddAddrBit/Run_test_pbs/DirPredict_3GHz/"
    config_path_template = "MJL_configFolderPath/m5out/MJL_appFile_MJL_configSuffix.stat"
    app_filename_template = "MJL_appFolder/MJL_appName_MJL_dataSize_MJL_appSuffix"
    app_data_size = '512'
    app_type = 'v'
    if args.common_prefix is not "":
        common_prefix = args.common_prefix
    if args.config_path_template is not "":
        config_path_template = args.config_path_template
    if args.app_filename_template is not "":
        app_filename_template = args.app_filename_template
    config_paths = dict()
    app_file_names = dict()
    input_file_paths = dict()
    # Initialize the path to configuration specific folder
    for config in config_list:
        config_paths[config] = config_path_template.replace("MJL_configFolderPath", config).replace("MJL_configSuffix", config)
        if config in ["Baseline_pf"]:
            config_paths[config] = config_path_template.replace("MJL_configFolderPath", "Baseline_256KBL2_pf").replace("MJL_configSuffix", "Baseline_256KBL2_pf")
        if config in ["Baseline"]:
            config_paths[config] = config_path_template.replace("MJL_configFolderPath", "Baseline_256KBL2").replace("MJL_configSuffix", "Baseline_256KBL2")
    # Initialize the application specific file name
    for app in app_list:
        app_file_names[app] = app_filename_template.replace("MJL_appFolder", app + "_" + app_data_size).replace("MJL_appName", app).replace("MJL_dataSize", app_data_size).replace("_MJL_appSuffix", '')
        if app in ["libquantum", "calculix", "libquantum_train", "calculix_train_3Bff"]:
            app_file_names[app] = app_filename_template.replace("MJL_appFolder", app).replace("MJL_appName", app).replace("_MJL_dataSize", "").replace("_MJL_appSuffix", '')
    # Special cases for the config path goes here
    # config_paths["DRAM pfon"] = config_path_template.replace("MJL_configFolderPath", "base_256KBL2/gem5/dir_results/m5out").replace("MJL_configSuffix", "ld_pfon_dram")
    # config_paths["DRAM pfoff"] = config_path_template.replace("MJL_configFolderPath", "base_256KBL2/gem5/dir_results/m5out").replace("MJL_configSuffix", "ld_pfoff_dram")
    # config_paths["NVM pfon"] = config_path_template.replace("MJL_configFolderPath", "base_256KBL2/gem5/dir_results/m5out").replace("MJL_configSuffix", "ld_pfon")
    # config_paths["NVM pfoff"] = config_path_template.replace("MJL_configFolderPath", "base_256KBL2/gem5/dir_results/m5out").replace("MJL_configSuffix", "ld_pfoff")
    # config_paths["1P2L pfoff"] = config_path_template.replace("MJL_configFolderPath", "2D_2MBL2/gem5/dir_1P2L/m5out").replace("MJL_configSuffix", "lc_pfoff")
    # config_paths["2P2L sparse pfoff"] = config_path_template.replace("MJL_configFolderPath", "2D_2MBL2/gem5/dir_2P2L0/m5out").replace("MJL_configSuffix", "lc_pfoff_type0")
    # config_paths["2P2L dense pfoff"] = config_path_template.replace("MJL_configFolderPath", "2D_2MBL2/gem5/dir_2P2L1/m5out").replace("MJL_configSuffix", "lc_pfoff_type1")
    # config_paths["NVM_pfoff"] = config_path_template.replace("MJL_configFolderPath", "Config3")
    # Special cases for the application file name goes here
    # app_file_names["ssyrk_256"] = "ssyrk_256.stat"
    # Initialize the input file paths
    for config in config_paths:
        input_file_paths[config] = dict()
        for app in app_file_names:
            input_file_paths[config][app] = common_prefix + "/" + config_paths[config].replace("MJL_appFile", app_file_names[app])
            if config in ["Baseline_1MBL3_pf", "Baseline_1MBL3"]:
                input_file_paths[config][app] = "/home/mdl/mjl5868/GEM5/Mem2D/New_Version/OrigTest/Run_test_pbs/Jun20_2018_test/" + "/" + config_paths[config].replace("MJL_appFile", app_file_names[app])
                if app in ["calculix_train_3Bff"]:
                    input_file_paths[config][app] = "/home/mdl/mjl5868/GEM5/Mem2D/New_Version/Original/Run_test_pbs/Jun20_2018_test/" + "/" + config_paths[config].replace("MJL_appFile", app_file_names[app])
                if app in ["libquantum_train"]:
                    input_file_paths[config][app] = "/home/mdl/mjl5868/GEM5/Mem2D/New_Version/OrigTest/Run_test_pbs/Jun20_2018_test/" + "/" + config_paths[config].replace("MJL_appFile", app_file_names[app])
            if config in ["Baseline", "Baseline_pf"]:
                input_file_paths[config][app] = "/home/mdl/mjl5868/GEM5/Mem2D/New_Version/OrigTest/Run_test_pbs/May04_2018_test/" + config_paths[config].replace("MJL_appFile", app_file_names[app])
            if config in ["Baseline", "Baseline_pf"] and app in ["soplex", "calculix"]:
                input_file_paths[config][app] = "/home/mdl/mjl5868/GEM5/Mem2D/New_Version/OrigTest/Run_test_pbs/May15_2018_SPEC2006FP/" + config_paths[config].replace("MJL_appFile", app_file_names[app])
            if config in ["Annotated_cs"] and app in ["soplex", "calculix"]:
                input_file_paths[config][app] = common_prefix + "/" + config_path_template.replace("MJL_configFolderPath", "Annotated").replace("MJL_configSuffix", "Annotated").replace("MJL_appFile", app_file_names[app])
            if config in ["Predict_mshr_stride", "L1L2L3Same_Predict"] and app not in ["libquantum_train", "calculix_train_3Bff"]:
                if app in ["calculix", "libquantum"]:
                    input_file_paths[config][app] = input_file_paths[config][app] + "_valtest"
                else:
                    input_file_paths[config][app] = input_file_paths[config][app] + "_Jul24_2018"
            if config in ["L1L2L3Same_Annotated_rowpf", "L1L2L3Same_Predict_rowpf"] and app not in ["libquantum_train", "calculix_train_3Bff"]:
                input_file_paths[config][app] = input_file_paths[config][app] + "_Jul29_2018"
            if config in ["L1L2L3Same_Predict_pf", "L1L2L3Same_Annotated_pf"] and app not in ["libquantum_train", "calculix_train_3Bff"]:
                input_file_paths[config][app] = input_file_paths[config][app] + "_Jul30_2018"
            if config in ["L1L2L3Same_Predict_pf"] and app in ["sobel"]:
                input_file_paths[config][app] = common_prefix + "/" + config_paths[config].replace("MJL_appFile", app_file_names[app]) + "_Aug02_2018"
    # Special cases for config/app combination goes here
    # for config in ["DRAM pfon", "DRAM pfoff", "NVM pfon", "NVM pfoff"]:
    #     for app in app_file_names:
    #         input_file_paths[config][app] = common_prefix + "/" + config_paths[config].replace("MJL_appFile", app_filename_template.replace("MJL_appName", app).replace("MJL_dataSize", app_data_size).replace("MJL_appSuffix", 'r' + app_type))
    # input_file_paths["DRAM pfon"]["sgemm"] = common_prefix + "Config1/sgemm_256.stat"
    for config in config_paths:
        for app in app_file_names:
            if app_data_size in ["512"]:
                if app in ["sgemm", "ssyr2k", "ssyrk", "strmm"]:
                    common_prefix = "/home/mdl/sug241/projects/Minli/micro/multiple_runs/config3_3G/"
                    config_path_template = "MJL_configFolderPath/MJL_appFile_MJL_configSuffix_l3.stat"
                    app_filename_template = "MJL_appName_MJL_dataSize_MJL_appSuffix"
                    if config in ["Baseline_1MBL3_pf"]:
                        config_paths[config] = config_path_template.replace("MJL_configFolderPath", "base_mirco_rev/gem5/dir_results_1.5L3/m5out").replace("MJL_configSuffix", "ld_pfon") 
                    if config in ["Baseline_1MBL3"]:
                        config_paths[config] = config_path_template.replace("MJL_configFolderPath", "base_mirco_rev/gem5/dir_results_1.5L3/m5out").replace("MJL_configSuffix", "ld_pfoff")
                    if config in ["Baseline_1MBL3_pf", "Baseline_1MBL3"]:
                        input_file_paths[config][app] = common_prefix + "/" + config_paths[config].replace("MJL_appFile", app_filename_template.replace("MJL_appName", app).replace("MJL_dataSize", app_data_size).replace("MJL_appSuffix", 'r' + app_type))
    # Check if all file paths are from the config list and the app list
    for config in input_file_paths.keys():
        if config not in config_list:
            print "MJL_Warning: the config " + config + " is not in the list of configs provided"
        for app in input_file_paths[config].keys():
            if app not in app_list:
                print "MJL_Warning: the app " + app + " is not in the list of apps provided"
    # Check if all the file paths exists
    for config in input_file_paths.keys():
        for app in input_file_paths[config].keys():
            path = input_file_paths[config][app]
            if not os.path.isfile(path):
                print "MJL_Error: the path " + path + " does not exist, removing it from the list of input file paths"
                del input_file_paths[config][app]
        if len(input_file_paths[config]) == 0:
            del input_file_paths[config]

    return input_file_paths
