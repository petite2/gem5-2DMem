from collections import OrderedDict

'''
Collects statistics from input files and put them in a python structure
Args:
    input_file_paths: The dict structure with the path of input stat file for each config and app combination
    config_list: The list of names of configurations
    app_list: The list of names of applications
    stat_map: The dict structure with the name of the statistics as key, and the keyword to find the stat in the file as value
Returns:
    stats_table: The dict structure that contains the interested statistics for each input file
'''
def mjl_grep_stats(input_file_paths, config_list, app_list, stat_map):
    stats_table = OrderedDict()
    for stat in stat_map:
        stats_table[stat] = OrderedDict()
        for config in config_list:
            stats_table[stat][config] = OrderedDict()
            for app in app_list:
                stats_table[stat][config][app] = "noData"

    for config in input_file_paths:
        for app in input_file_paths[config]:
            with open(input_file_paths[config][app]) as input_file:
                for line in input_file:
                    for stat in stat_map:
                        if stat_map[stat] in line:
                            stats_table[stat][config][app] = line.split()[1]

    return stats_table


'''
Add statistics normalized to baseline
Args:
    stats_table: The dict structure with the interested statistics for each config and app combinations
    config_list: The list of names of configurations, the first one in the list is the baseline
    app_list: The list of names of applications
    normalize_map: The dict structure that indicates statistics that need normalized version
Returns:
    normalize_stats_table: The new dict structure that has the interested statistics and the normalized version of some statistics
'''
def mjl_normalize_stats(stats_table, config_list, app_list, normalize_map):
    normalize_stats_table = OrderedDict()
    for stat in stats_table:
        normalize_stats_table[stat] = OrderedDict()
        for config in config_list:
            normalize_stats_table[stat][config] = OrderedDict()
            for app in app_list:
                normalize_stats_table[stat][config][app] = stats_table[stat][config][app]
        if stat in normalize_map:
            normalized_stat_name = normalize_map[stat]
            normalize_stats_table[normalized_stat_name] = OrderedDict()
            for config in config_list:
                normalize_stats_table[normalized_stat_name][config] = OrderedDict()
                for app in app_list:
                    try:
                        base_value = float(stats_table[stat][config_list[0]][app])
                        value = float(stats_table[stat][config][app])
                        normalize_stats_table[normalized_stat_name][config][app] = value/base_value
                    except ValueError:
                        normalize_stats_table[normalized_stat_name][config][app] = "noData"

    return normalize_stats_table
    
'''
Add statistics with basic point to point operation(+, -, *, /, etc) based on formula
Args:
    stats_table: The dict structure with the interested statistics for each config and app combinations 
    config_list: The list of names of configurations, the first one in the list is the baseline
    app_list: The list of names of applications
    formula_map: The dict structure with output statistic name for key, and a list of the operation formula and input statistic names for value. The first element in the list of the value should be the formula, where the statistic names parentheses are used in place of specific value, and the subsequent elements should be a complete list of pairs of the statistic names used as input and the default value used if the value does not exist in the stats table
                 e.g. formula_map = {"a": [(b)+(c), ["b", "0.0"], ["c", "1.0"]]}
                 The result of a formula cannot be used as variable for another formula, a new invocation of this function with it's output is needed.
    stats_list: Order of the statistics in the output, with statistic names of calculated results included.
Returns:
    result_stats_table: The new dict structure that has the interested statistics and the result of the point to point operation added
'''
def mjl_config_formula_stats(stats_table, config_list, app_list, formula_map, stats_list):
    result_stats_table = OrderedDict()
    for stat in stats_list:
        result_stats_table[stat] = OrderedDict()
        for config in config_list:
            result_stats_table[stat][config] = OrderedDict()
            for app in app_list:
                if stat not in formula_map:
                    result_stats_table[stat][config][app] = stats_table[stat][config][app]   
                else:
                    formula = formula_map[stat][0]
                    for variable in formula_map[stat][1:]:
                        try: 
                            value = float(stats_table[variable[0]][config][app])
                            formula = formula.replace("(" + variable[0] + ")", str(value))
                        except ValueError:
                            formula = formula.replace("(" + variable[0] + ")", variable[1])
                    try:
                        result_stats_table[stat][config][app] = eval(formula)
                    except:
                        result_stats_table[stat][config][app] = "noData"

    return result_stats_table
