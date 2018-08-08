# import argparse
from collections import OrderedDict

import Input_Path
import Stat_Parser
import Generate_Plots
# import pprint

# parser = argparse.ArgumentParser(description="example of constructing an input parser")
# parser.add_argument("--arg1", default="", help="example argument 1")

# args = parser.parse_args()
# pp = pprint.PrettyPrinter()

# List of all configurations, the first one should be the baseline
# config_list = ["Annotated", "Annotated_cs", "Predicted", "Predict_mshr", "Predict_mshr_stride", "Baseline", "Baseline_pf"]
config_list = ["Baseline_1MBL3_pf", "Baseline_1MBL3", "L1L2L3Same_Annotated_rowpf", "L1L2L3Same_Annotated_pf", "L1L2L3Same_Annotated", "L1L2L3Same_Predict_rowpf", "L1L2L3Same_Predict_pf", "L1L2L3Same_Predict"]# "L1L2Same_Annotated"
pred_config_list = ["L1L2L3Same_Annotated", "L1L2L3Same_Predict", "Baseline_1MBL3", "Baseline_1MBL3_pf"]# "L1L2Same_Annotated"
pf_config_list = ["L1L2L3Same_Annotated", "L1L2L3Same_Annotated_rowpf", "L1L2L3Same_Annotated_pf", "L1L2L3Same_Predict", "L1L2L3Same_Predict_rowpf", "L1L2L3Same_Predict_pf", "Baseline_1MBL3", "Baseline_1MBL3_pf"]# "L1L2Same_Annotated"
# Map of names used for configurations in the output if different from the config_list
config_name_map = dict()
config_name_map["Baseline_1MBL3"] = "Baseline"
config_name_map["Baseline_1MBL3_pf"] = "Baseline_pf"
config_name_map["L1L2L3Same_Predict"] = "Predict"
config_name_map["L1L2L3Same_Predict_pf"] = "Predict_pf"
config_name_map["L1L2L3Same_Predict_rowpf"] = "Predict_rowpf"
config_name_map["L1L2L3Same_Annotated"] = "Annotated"
config_name_map["L1L2L3Same_Annotated_pf"] = "Annotated_pf"
config_name_map["L1L2L3Same_Annotated_rowpf"] = "Annotated_rowpf"

# List of all applications
app_list = ["sgemm", "ssyr2k", "ssyrk", "strmm", "sobel", "htap_anal", "htap_tran", "calculix_train_3Bff", "libquantum_train"]
# Map of names used for applications in the output if different from the app_list
app_name_map = dict()
app_name_map["calculix_train_3Bff"] = "calculix"
app_name_map["libquantum_train"] = "libquantum"

# List of all statistics to gather
stat_map = OrderedDict()
stat_map['Total cycles'] = "system.switch_cpus.numCycles"
stat_map['L1 miss rate'] = "system.cpu.dcache.overall_miss_rate::total"
stat_map['L1 accesses'] = "system.cpu.dcache.overall_accesses::total"
stat_map['L1 row vector accesses'] = "system.cpu.dcache.MJL_overallRowVecAccesses"
stat_map['L1 column vector accesses'] = "system.cpu.dcache.MJL_overallColVecAccesses"
stat_map['L2 miss rate'] = "system.l2.overall_miss_rate::total"
stat_map['L2 accesses'] = "system.l2.overall_accesses::total"
stat_map['L2 writebackdirty accesses'] = "system.l2.WritebackDirty_accesses::total"
stat_map['L2 writebackclean accesses'] = "system.l2.WritebackClean_accesses::total"
stat_map['L2 upgrade accesses'] = "system.l2.UpgradeReq_accesses::total"
stat_map['L2 row accesses'] = "system.l2.MJL_overallRowAccesses"
stat_map['L2 column accesses'] = "system.l2.MJL_overallColumnAccesses"
stat_map['L3 miss rate'] = "system.l3.overall_miss_rate::total"
stat_map['L3 accesses'] = "system.l3.overall_accesses::total"
stat_map['L3 row accesses'] = "system.l3.MJL_overallRowAccesses"
stat_map['L3 column accesses'] = "system.l3.MJL_overallColumnAccesses"
stat_map['Total bytes read from memory'] = "system.mem_ctrls.bytes_read::total"
stat_map['Total bytes written to memory'] = "system.mem_ctrls.bytes_written::total"
stat_map['Memory Prefetch Accesses'] = "system.mem_ctrls.num_reads::l3.prefetcher"
stat_map['Unused Prefetches'] = "system.l3.unused_prefetches"
# stat_map['NoData'] = "MJL_noData"

# List of all statistics to calculate
formula_map = OrderedDict()
formula_map['L2 accesses (row + column)'] = ['(L2 row accesses) + (L2 column accesses)', ['L2 row accesses', '0.0'], ['L2 column accesses', '0.0']]
formula_map['L2 accesses (demand + writeback + upgrade)'] = ['(L2 accesses) + (L2 writebackdirty accesses) + (L2 writebackclean accesses) + (L2 upgrade accesses)', ['L2 accesses', '0.0'], ['L2 writebackdirty accesses', '0.0'], ['L2 writebackclean accesses', '0.0'], ['L2 upgrade accesses', '0.0']]
formula_map['L2 access distribution (column/(demand + writeback + upgrade))'] = ['(L2 column accesses)/((L2 accesses) + (L2 writebackdirty accesses) + (L2 writebackclean accesses) + (L2 upgrade accesses))', ['L2 column accesses', '0.0'], ['L2 accesses', '0.0'], ['L2 writebackdirty accesses', '0.0'], ['L2 writebackclean accesses', '0.0'], ['L2 upgrade accesses', '0.0']]
formula_map['Prefetch Accuracy'] = ['1-(Unused Prefetches)/(Memory Prefetch Accesses)', ['Memory Prefetch Accesses', '0.0'], ['Unused Prefetches', '0.0']]
calc_stats_list = ['Total cycles', 'L1 miss rate', 'L1 accesses', 'L2 miss rate', 'L2 accesses', 'L2 accesses (demand + writeback + upgrade)', 'L2 row accesses', 'L2 column accesses', 'L2 accesses (row + column)', 'L2 access distribution (column/(demand + writeback + upgrade))', 'L3 miss rate', 'L3 accesses', 'L3 row accesses', 'L3 column accesses', 'Total bytes read from memory', 'Total bytes written to memory', 'Memory Prefetch Accesses', 'Unused Prefetches', 'Prefetch Accuracy']

# List of all statistics to normalize
normalize_map = OrderedDict()
normalize_map['Total cycles'] = "Normalized total cycles"
normalize_map['L1 miss rate'] = "Normalized L1 miss rate"
normalize_map['L2 miss rate'] = "Normalized L2 miss rate"
normalize_map['L2 accesses'] = "Normalized L2 accesses"
normalize_map['L2 accesses (demand + writeback + upgrade)'] = "Normalized L2 accesses (demand + writeback + upgrade)"
# normalize_map['L2 row accesses'] = "Normalized L2 row accesses"
# normalize_map['L2 column accesses'] = "Normalized L2 column accesses"
normalize_map['L3 miss rate'] = "Normalized L3 miss rate"
normalize_map['L3 accesses'] = "Normalized L3 accesses"
normalize_map['Total bytes read from memory'] = "Normalized total bytes read from memory"
normalize_map['Total bytes written to memory'] = "Normalized total bytes written to memory"
normalize_map['Prefetch Accuracy'] = 'Normalized Prefetch Accuracy'

# List of statistics to plot
plot_stats = ['Normalized total cycles', 'L1 accesses', 'Normalized L1 miss rate', 'Normalized L2 accesses', 'Normalized L2 accesses (demand + writeback + upgrade)', 'Normalized L2 miss rate', 'Normalized L3 accesses', 'Normalized L3 miss rate', "Normalized total bytes read from memory", "Normalized total bytes written to memory", 'Prefetch Accuracy', 'Normalized Prefetch Accuracy']

# Get input file paths
input_paths = Input_Path.mjl_get_input_file_paths(config_list, app_list)
pred_input_paths = Input_Path.mjl_get_input_file_paths(pred_config_list, app_list)
pf_input_paths = Input_Path.mjl_get_input_file_paths(pf_config_list, app_list)
# pp.pprint(input_paths)

# Get Statistics from file
stats = Stat_Parser.mjl_grep_stats(input_paths, config_list, app_list, stat_map)
pred_stats = Stat_Parser.mjl_grep_stats(pred_input_paths, pred_config_list, app_list, stat_map)
pf_stats = Stat_Parser.mjl_grep_stats(pf_input_paths, pf_config_list, app_list, stat_map)
# Get Statistics with different names
# non_cpt_app_list = ["libquantum", "calculix"]
# non_cpt_stat_map = OrderedDict()
# non_cpt_stat_map['Total cycles'] = "system.cpu.numCycles"
# non_cpt_stats = Stat_Parser.mjl_grep_stats(input_paths, config_list, non_cpt_app_list, non_cpt_stat_map)
# pred_non_cpt_stats = Stat_Parser.mjl_grep_stats(pred_input_paths, pred_config_list, non_cpt_app_list, non_cpt_stat_map)
# pf_non_cpt_stats = Stat_Parser.mjl_grep_stats(pf_input_paths, pf_config_list, non_cpt_app_list, non_cpt_stat_map)
# for stat in non_cpt_stat_map:
#     for config in config_list:
#         for app in non_cpt_app_list:
#             stats[stat][config][app] = non_cpt_stats[stat][config][app]
#     for config in pred_config_list:
#         for app in non_cpt_app_list:
#             pred_stats[stat][config][app] = pred_non_cpt_stats[stat][config][app]
#     for config in pf_config_list:
#         for app in non_cpt_app_list:
#             pf_stats[stat][config][app] = pf_non_cpt_stats[stat][config][app]

# Calculate the statistics based on formula
stats = Stat_Parser.mjl_config_formula_stats(stats, config_list, app_list, formula_map, calc_stats_list)
pred_stats = Stat_Parser.mjl_config_formula_stats(pred_stats, pred_config_list, app_list, formula_map, calc_stats_list)
pf_stats = Stat_Parser.mjl_config_formula_stats(pf_stats, pf_config_list, app_list, formula_map, calc_stats_list)

# Calculate the normalized to baseline statisticts
stats = Stat_Parser.mjl_normalize_stats(stats, config_list, app_list, normalize_map)
pred_stats = Stat_Parser.mjl_normalize_stats(pred_stats, pred_config_list, app_list, normalize_map)
pf_stats = Stat_Parser.mjl_normalize_stats(pf_stats, pf_config_list, app_list, normalize_map)

# Other calculations
post_norm_formula_map = OrderedDict()
post_norm_formula_map["L2 access distribution weighted by normalized total accesses"] = ['(Normalized L2 accesses (demand + writeback + upgrade))*(L2 access distribution (column/(demand + writeback + upgrade)))',["Normalized L2 accesses (demand + writeback + upgrade)", "noData"], ['L2 access distribution (column/(demand + writeback + upgrade))', 'noData']]
post_norm_calc_stats_list = stats.keys() + ["L2 access distribution weighted by normalized total accesses"]

stats = Stat_Parser.mjl_config_formula_stats(stats, config_list, app_list, post_norm_formula_map, post_norm_calc_stats_list)
pred_stats = Stat_Parser.mjl_config_formula_stats(pred_stats, pred_config_list, app_list, post_norm_formula_map, post_norm_calc_stats_list)
pf_stats = Stat_Parser.mjl_config_formula_stats(pf_stats, pf_config_list, app_list, post_norm_formula_map, post_norm_calc_stats_list)

# Plot the results
set_stats_1 = dict()
set_stats_1["results"] = stats
set_stats_1["plot"] = plot_stats
set_stats_1["worksheet_name"] = "Vector 512 1MBL3"
set_stats_2 = dict()
set_stats_2["results"] = pred_stats
set_stats_2["plot"] = plot_stats
set_stats_2["worksheet_name"] = "Vector 512 1MBL3 Predict"
set_stats_3 = dict()
set_stats_3["results"] = pf_stats
set_stats_3["plot"] = plot_stats
set_stats_3["worksheet_name"] = "Vector 512 1MBL3 Prefetch"
multiset_stats = [set_stats_1, set_stats_2, set_stats_3]
Generate_Plots.mjl_plot_multiset_stats(multiset_stats, "DirPredict_3GHz_Aug03_2018_test.xlsx", config_name_map, app_name_map)

print("Done")
