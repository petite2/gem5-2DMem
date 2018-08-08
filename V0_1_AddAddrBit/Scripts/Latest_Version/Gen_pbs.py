import Gen_Run
import Config_Input

cluster_name = "goliath"

# app_list = ["sgemm_512", "ssyr2k_512", "ssyrk_512", "strmm_512", "sobel_512", "htap_anal_512", "htap_tran_512", "calculix", "bzip2", "libquantum", "hmmer", "soplex"]
# app_list = ["bzip2", "hmmer", "libquantum", "mcf", "gobmk"]
app_list = ["calculix_train_3Bff"] # "bzip2_train", "libquantum_train", "calculix_train"
# app_list = ["sobel_512"]

# config_list = ["cpt", "Annotated", "Annotated_cs", "Predicted", "Predict_mshr", "Predict_mshr_stride", "L1L2Same_Predict", "L1L2Same_Annotated"]
# config_list = ["cpt", "Annotated_cs", "Predict_mshr_stride", "L1L2L3Same_Predict", "L1L2L3Same_Annotated"]
# config_list = ["Predict_mshr_stride_pf", "L1L2L3Same_Predict_pf", "Annotated_cs_pf", "L1L2L3Same_Annotated_pf", "Annotated_cs", "Predict_mshr_stride", "L1L2L3Same_Predict", "L1L2L3Same_Annotated", "RowOnly", "RowOnly_pf", "L1L2L3Same_RowOnly", "L1L2L3Same_RowOnly_pf", "Predict_mshr_stride_rowpf", "L1L2L3Same_Predict_rowpf", "Annotated_cs_rowpf", "L1L2L3Same_Annotated_rowpf"]
config_list = ["cpt", "L1L2L3Same_Annotated", "L1L2L3Same_Predict", "L1L2L3Same_Annotated_pf", "L1L2L3Same_Predict_pf", "L1L2L3Same_Annotated_rowpf", "L1L2L3Same_Predict_rowpf"]# "cpt", 
# config_list = ["L1L2L3Same_Predict_pf"]

config_options = Config_Input.generate_configs(app_list, config_list)

output_suffix = ""

### Generate the pbs file
for app in app_list:
    for config in config_list:
        Gen_Run.generate_pbs(cluster_name, app, config, config, config_options[config][app], output_suffix)
