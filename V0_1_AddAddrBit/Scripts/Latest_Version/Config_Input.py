config_options_template = "--caches --l3cache --cpu-clock=3GHz cpuOptions --MJL_row_width=64 --mem-size=4096MB --l1d_size=32kB --l1i_size=32kB --l2_size=256kB --l3_size=1MB --l1d_assoc=4 --l1i_assoc=4 --l2_assoc=8 --l3_assoc=8 --MJL_PC2DirFile=BenchmarkDirAnnotationFile appSpecificOptions configSpecificOptions --mem-type=NVMainMemory --nvmain-config=/home/mdl/sug241/projects/Minli/micro/multiple_runs/config3_3G/2D_2MBL3/NVMain/Config/STTRAM_Everspin_4GB_ChRkBkSaRC_5.config cptOptions"

def generate_configs(app_list, config_list):
    ### Define configuration specific options
    config_specific_options = dict()
    cpu_options = dict()
    for config in config_list:
        config_specific_options[config] = ""
        cpu_options[config] = " --cpu-type=detailed"
        if config in ["Predict_mshr_stride", "Predict_mshr_stride_pf", "L1L2L3Same_Predict", "L1L2L3Same_Predict_pf", "Predict_mshr_stride_rowpf", "L1L2L3Same_Predict_rowpf"]:
            config_specific_options[config] = " --MJL_predictDir --MJL_mshrPredictDir"
        if config in ["cpt"]:
            # config_specific_options[config] = " -I 100000000"
            cpu_options[config] = " --cpu-type=atomic"
        if config in ["L1L2L3Same_Predict", "L1L2L3Same_Annotated", "L1L2L3Same_Predict_pf", "L1L2L3Same_Annotated_pf", "L1L2L3Same_RowOnly", "L1L2L3Same_RowOnly_pf", "L1L2L3Same_Predict_rowpf", "L1L2L3Same_Annotated_rowpf"]:
            config_specific_options[config] = config_specific_options[config] + " --MJL_L1sameSetMapping --MJL_L2sameSetMapping --MJL_L3sameSetMapping"
        if config in ["Predict_mshr_stride_pf", "L1L2L3Same_Predict_pf", "Annotated_cs_pf", "L1L2L3Same_Annotated_pf"]:
            config_specific_options[config] = config_specific_options[config] + " --MJL_Prefetcher --MJL_colPf"
        if config in ["RowOnly_pf", "L1L2L3Same_RowOnly_pf", "Predict_mshr_stride_rowpf", "L1L2L3Same_Predict_rowpf", "Annotated_cs_rowpf", "L1L2L3Same_Annotated_rowpf"]:
            config_specific_options[config] = config_specific_options[config] + " --MJL_Prefetcher"

    ### Define app specific options
    work_dir = "/home/mdl/mjl5868/GEM5/Mem2D/New_Version/2dmem_v0_1_addaddrbit/V0_1_AddAddrBit/Run_test_pbs/DirPredict_3GHz/"
    app_specific_options = dict()
    app_binary_filenames = dict()
    for app in app_list:
        app_binary_filenames[app] = work_dir + "/Binaries_and_Annotation/" + app + "_512_o3_chk.out"
        if app in ["soplex", "calculix", "soplex_pds-50", "soplex_pds-20", "soplex_train", "lbm", "lbm_train", "lbm_test", "bzip2", "hmmer", "libquantum", "mcf", "gobmk", "bzip2_train", "libquantum_train", "calculix_train", "calculix_train_3Bff"]:
            app_binary_filenames[app] = work_dir + "/Binaries_and_Annotation/SPEC2006/" + app.split("_")[0] + "/" + app.split("_")[0] + "-bin"
        app_specific_options[app] = " -c " + app_binary_filenames[app]
        if app in ["htap_anal_512"]:
            app_binary_filenames[app] = work_dir + "/Binaries_and_Annotation/" + "htap_analytic_512_512_o3_chk.out"
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o " + work_dir + "/Binaries_and_Annotation/htap_analytic_input.txt"
        if app in ["htap_tran_512"]:
            app_binary_filenames[app] = work_dir + "/Binaries_and_Annotation/" + "htap_transact_512_512_o3_chk.out"
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o " + work_dir + "/Binaries_and_Annotation/htap_transact_input.txt" 
        if app in ["soplex"]:
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'-m10000 " + work_dir + "/Binaries_and_Annotation/SPEC2006/" + app + "/test.mps\'"  
        if app in ["calculix"]:
            # app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'-i beampic\'"
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'-i hyperviscoplastic\'"
        if app in ["calculix_train", "calculix_train_3Bff"]:
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'-i stairs\'"
        if app in ["bzip2"]:
            # app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'" + work_dir + "/Binaries_and_Annotation/SPEC2006/" + app + "/input/input.program 5\'"
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'" + work_dir + "/Binaries_and_Annotation/SPEC2006/" + app + "/input/input.program 280\'"
        if app in ["bzip2_train"]:
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'" + work_dir + "/Binaries_and_Annotation/SPEC2006/bzip2/input/input.program 10\'"
        # if app in ["sjeng"]:
        #     app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'" + work_dir + "/Binaries_and_Annotation/SPEC2006/" + app + "/test.txt\'"
        if app in ["mcf"]:
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'" + work_dir + "/Binaries_and_Annotation/SPEC2006/" + app + "/inp.in\'"
        if app in ["gobmk"]:
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'--quiet --mode gtp < " + work_dir + "/Binaries_and_Annotation/SPEC2006/" + app + "/capture.tst\'"
        if app in ["hmmer"]:
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'--fixed 0 --mean 325 --num 45000 --sd 200 --seed 0 " + work_dir + "/Binaries_and_Annotation/SPEC2006/" + app + "/bombesin.hmm\'"
        if app in ["libquantum"]:
            # app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'33 5\'"
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'1397 8\'"
        if app in ["libquantum_train"]:
            app_specific_options[app] = " -c " + app_binary_filenames[app] + " -o \'143 25\'"

    ### Define config and app specific options
    app_annotation_filenames = dict()
    cpt_options = dict()
    for config in config_list:
        app_annotation_filenames[config] = dict()
        cpt_options[config] = dict()
        for app in app_list:
            app_annotation_filenames[config][app] = app_binary_filenames[app].replace("_o3_chk.out", "_pc_cv_chk.txt")
            cpt_options[config][app] = " --checkpoint-dir=" + work_dir + "/cpt/m5out/" + app + " --checkpoint-restore=1"
            if config in ["Predict_mshr_stride", "Predict_mshr_stride_pf", "L1L2L3Same_Predict", "L1L2L3Same_Predict_pf", "cpt", "RowOnly", "RowOnly_pf", "L1L2L3Same_RowOnly", "L1L2L3Same_RowOnly_pf", "L1L2L3Same_Predict_rowpf", "Predict_mshr_stride_rowpf"]:
                app_annotation_filenames[config][app] = app_binary_filenames[app].replace("_o3_chk.out", "_pc_rv_chk.txt")  
            if config in ["Annotated_cs", "L1L2L3Same_Annotated", "Annotated_cs_pf", "L1L2L3Same_Annotated_pf", "Annotated_cs_rowpf", "L1L2L3Same_Annotated_rowpf"]:
                app_annotation_filenames[config][app] = app_binary_filenames[app].replace("_o3_chk.out", "_pc_cs_chk.txt")  
            if config in ["cpt"]:
                cpt_options[config][app] = " --checkpoint-dir=" + work_dir + "/" + config + "/m5out/" + app
                if app in ["calculix", "bzip2", "bzip2_train", "libquantum_train", "calculix_train"]:
                    cpt_options[config][app] = cpt_options[config][app] + " -I 6000000000 --checkpoint-at-end"
                elif app in ["calculix_train_3Bff"]:
                    cpt_options[config][app] = cpt_options[config][app] + " -I 3000000000 --checkpoint-at-end"
                else :
                    cpt_options[config][app] = cpt_options[config][app] + " -I 100000000"
            if app in ["soplex", "calculix", "soplex_pds-50", "soplex_pds-20", "soplex_train", "calculix_train", "lbm", "lbm_train", "lbm_test", "bzip2", "hmmer", "libquantum", "mcf", "gobmk", "bzip2_train", "libquantum_train", "calculix_train", "calculix_train_3Bff"]:
                app_annotation_filenames[config][app] = app_binary_filenames[app].replace("-bin", "_pc_rv.txt")
                if config not in ["cpt"]:
                    cpt_options[config][app] = cpt_options[config][app] + " -I 2000000000"

    ### Generate the config options
    config_options = dict()
    for config in config_list:
        config_options[config] = dict()
        for app in app_list:
            config_options[config][app] = config_options_template.replace(" configSpecificOptions", config_specific_options[config]).replace(" appSpecificOptions", app_specific_options[app]).replace("BenchmarkDirAnnotationFile", app_annotation_filenames[config][app]).replace(" cptOptions", cpt_options[config][app]).replace(" cpuOptions", cpu_options[config])

    return config_options

