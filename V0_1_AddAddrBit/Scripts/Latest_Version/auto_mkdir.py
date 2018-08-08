import os
import shutil
from distutils import dir_util

# app_list = ["sgemm_512", "ssyr2k_512", "ssyrk_512", "strmm_512", "sobel_512", "htap_anal_512", "htap_tran_512"]#, "calculix", "bzip2", "libquantum", "hmmer", "soplex"]
# app_list = ["sgemm_512", "ssyr2k_512", "ssyrk_512", "strmm_512", "sobel_512", "htap_anal_512", "htap_tran_512"]
# app_list = ["calculix", "bzip2", "libquantum", "hmmer", "soplex"]
# app_list = ["bzip2", "hmmer", "libquantum", "mcf", "gobmk"]
# benchmarks failed in baseline: soplex(train, ref), povray, gobmk(input problem), hmmer(ref, train)
app_list = ["calculix_train_3Bff"]# ["bzip2_train", "libquantum_train", "calculix_train"] # , "hmmer_train"

# config_list = ["Predict_mshr_stride", "L1L2L3Same_Predict"]# "cpt", "Predicted", "Predict_mshr", "Annotated_cs", 
# config_list = ["cpt"]
# config_list = ["Annotated_cs", "L1L2L3Same_Annotated"]
# config_list = ["Predict_mshr_stride_pf", "L1L2L3Same_Predict_pf", "Annotated_cs_pf", "L1L2L3Same_Annotated_pf"]
# config_list = ["RowOnly", "RowOnly_pf", "L1L2L3Same_RowOnly", "L1L2L3Same_RowOnly_pf"]
config_list = ["L1L2L3Same_Annotated", "L1L2L3Same_Predict", "L1L2L3Same_Annotated_pf", "L1L2L3Same_Predict_pf", "L1L2L3Same_Annotated_rowpf", "L1L2L3Same_Predict_rowpf"] # "cpt"]# , "Predict_mshr_stride_rowpf", "Annotated_cs_rowpf", 

### Generate folders
mkdir_folders = []
outFilename = "auto_mkdir_out.txt"
content = ""
for config in config_list:
    config_folder = "../" + config
    appout_folder = config_folder + "/appout"
    m5out_folder = config_folder + "/m5out" 
    run_folder = config_folder + "/run"
    mkdir_folders.append(config_folder)
    mkdir_folders.append(appout_folder)
    mkdir_folders.append(m5out_folder)
    mkdir_folders.append(run_folder)
    for app in app_list:
        m5app_folder = m5out_folder + "/" + app
        runapp_folder = run_folder + "/" + app
        mkdir_folders.append(m5app_folder) 
        mkdir_folders.append(runapp_folder) 


for folder in mkdir_folders:
    if not os.path.exists(folder):
        content = content + "mkdir " + folder + "\n"
        os.makedirs(folder)
    else:
        content = content + "Error: " + folder + " already exists" + "\n"

### Copy run required files
cp_src_dst = dict()
for config in config_list:
    cp_src_dst[config] = dict()
    if "calculix" in app_list:
        cp_src_dst[config]["calculix"] = dict()
        cp_src_dst[config]["calculix"]["src"] = "../Binaries_and_Annotation/SPEC2006/calculix/hyperviscoplastic.inp"
        cp_src_dst[config]["calculix"]["dst"] = "../" + config + "/run/calculix"
    if "calculix_train" in app_list:
        cp_src_dst[config]["calculix_train"] = dict()
        cp_src_dst[config]["calculix_train"]["src"] = "../Binaries_and_Annotation/SPEC2006/calculix/stairs.inp"
        cp_src_dst[config]["calculix_train"]["dst"] = "../" + config + "/run/calculix_train"
    if "calculix_train_3Bff" in app_list:
        cp_src_dst[config]["calculix_train_3Bff"] = dict()
        cp_src_dst[config]["calculix_train_3Bff"]["src"] = "../Binaries_and_Annotation/SPEC2006/calculix/stairs.inp"
        cp_src_dst[config]["calculix_train_3Bff"]["dst"] = "../" + config + "/run/calculix_train_3Bff"
        
cp_folder_src_dst = dict()
for config in config_list:
    cp_folder_src_dst[config] = dict()
    if "calculix_train" in app_list:
        if config not in ["cpt"]:
            cp_folder_src_dst[config]["calculix_train"] = dict()
            cp_folder_src_dst[config]["calculix_train"]["src"] = "../cpt/run/calculix_train/"
            cp_folder_src_dst[config]["calculix_train"]["dst"] = "../" + config + "/run/calculix_train"
    if "calculix_train_3Bff" in app_list:
        if config not in ["cpt"]:
            cp_folder_src_dst[config]["calculix_train_3Bff"] = dict()
            cp_folder_src_dst[config]["calculix_train_3Bff"]["src"] = "../cpt/run/calculix_train_3Bff/"
            cp_folder_src_dst[config]["calculix_train_3Bff"]["dst"] = "../" + config + "/run/calculix_train_3Bff"
    # if "calculix_test" in app_list:
    #     cp_src_dst[config]["calculix_test"] = dict()
    #     cp_src_dst[config]["calculix_test"]["src"] = "../Binaries_and_Annotation/SPEC2006/calculix/beampic.inp"
    #     cp_src_dst[config]["calculix_test"]["dst"] = "../" + config + "/run/calculix_test"
    # if "calculix_train" in app_list:
    #     cp_src_dst[config]["calculix_train"] = dict()
    #     cp_src_dst[config]["calculix_train"]["src"] = "../Binaries_and_Annotation/SPEC2006/calculix/stairs.inp"
    #     cp_src_dst[config]["calculix_train"]["dst"] = "../" + config + "/run/calculix_train"
            
for config in cp_src_dst:
    for app in cp_src_dst[config]:
        if not os.path.exists(cp_src_dst[config][app]["src"]):
            content = content + "Error: " + cp_src_dst[config][app]["src"] + " does not exist\n"
        if not os.path.exists(cp_src_dst[config][app]["dst"]):
            content = content + "Error: " + cp_src_dst[config][app]["dst"] + " does not exist\n"
        if os.path.exists(cp_src_dst[config][app]["src"]) and os.path.exists(cp_src_dst[config][app]["dst"]):
            content = content + "cp " + cp_src_dst[config][app]["src"] + " " + cp_src_dst[config][app]["dst"] + "\n"
            shutil.copy(cp_src_dst[config][app]["src"], cp_src_dst[config][app]["dst"])
for config in cp_folder_src_dst:
    for app in cp_folder_src_dst[config]:
        if not os.path.exists(cp_folder_src_dst[config][app]["src"]):
            content = content + "Error: " + cp_folder_src_dst[config][app]["src"] + " does not exist\n"
        if not os.path.exists(cp_folder_src_dst[config][app]["dst"]):
            content = content + "Error: " + cp_folder_src_dst[config][app]["dst"] + " does not exist\n"
        if os.path.exists(cp_folder_src_dst[config][app]["src"]) and os.path.exists(cp_folder_src_dst[config][app]["dst"]):
            content = content + "cp -r " + cp_folder_src_dst[config][app]["src"] + "/* " + cp_folder_src_dst[config][app]["dst"] + "\n"
            dir_util.copy_tree(cp_folder_src_dst[config][app]["src"], cp_folder_src_dst[config][app]["dst"])


fobj = open(outFilename, "w")
fobj.write(content)
fobj.close()

if "Error" in content:
    print "Error occured during folder creation"
