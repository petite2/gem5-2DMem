# app_list = ["sgemm_512", "ssyr2k_512", "ssyrk_512", "strmm_512", "sobel_512", "htap_anal_512", "htap_tran_512"]#, "calculix", "bzip2", "libquantum", "hmmer", "soplex"]
# app_list = ["sgemm_512", "ssyr2k_512", "ssyrk_512", "strmm_512", "sobel_512", "htap_anal_512", "htap_tran_512"]
# app_list = ["calculix", "bzip2", "libquantum", "hmmer", "soplex"]
# app_list = ["bzip2", "hmmer", "libquantum", "mcf", "gobmk"]
# app_list = ["sobel_512", "htap_anal_512", "htap_tran_512"]
app_list = ["calculix_train_3Bff"]# ["bzip2_train", "libquantum_train", "calculix_train"]

# config_list = ["Predict_mshr_stride", "L1L2L3Same_Predict"]# "cpt", "Predicted", "Predict_mshr", "Annotated_cs", 
# config_list = ["cpt"]
# config_list = ["RowOnly", "RowOnly_pf", "L1L2L3Same_RowOnly", "L1L2L3Same_RowOnly_pf", "Annotated_cs", "L1L2L3Same_Annotated", "Annotated_cs_pf", "L1L2L3Same_Annotated_pf"]
# config_list = ["L1L2L3Same_Annotated_pf", "L1L2L3Same_Predict_pf"] # "Predict_mshr_stride_rowpf", "Annotated_cs_rowpf", "L1L2L3Same_Predict_rowpf", "L1L2L3Same_Annotated_rowpf", 
config_list = ["L1L2L3Same_Annotated", "L1L2L3Same_Predict", "L1L2L3Same_Annotated_pf", "L1L2L3Same_Predict_pf", "L1L2L3Same_Annotated_rowpf", "L1L2L3Same_Predict_rowpf"]#  "cpt"]# 

output_suffix = ""

### Generate the pbs file
# for config in config_list:
    # outFilename = "sub_" + config + output_suffix + ".sh"
    # content = ""
    # for app in app_list:
for app in app_list:
    outFilename = "sub_" + app + output_suffix + ".sh"
    content = ""
    for config in config_list:
        content = content + "qsub ../pbs/" + config + "_" + app + output_suffix + ".pbs\n"
    fobj = open(outFilename, "w")
    fobj.write(content)
    fobj.close()
