import os
import re
import pandas as pd
import plotly.io as pio
import utils


"""
Aggregation  of the memory stats log produced by the Khiops tool

It takes log file as input and aggregate I/O time.
"""


def extract_driver_method(label):
    """Extract driver type, method Name, and Start/End"""
    driver_type = ""
    method_name = ""
    start_end = ""
    if label.find("driver") == 0:
        offset_s = label.find("[")
        offset_e = label.find("]")
        offset_be = label.find("Begin")
        if offset_be == -1:
            offset_be = label.find("End")
        assert offset_be != -1
        driver_type = label[offset_s + 1 : offset_e]
        method_name = label[offset_e + 2 : offset_be - 1]
        start_end = label[offset_be:]

        # Rename driver type in a more concise way
        schemes = ("hdfs", "ansi", "s3")
        for scheme in schemes:
            if driver_type.lower().find(scheme) != -1:
                driver_type = scheme
                break
    return driver_type, method_name, start_end


def collect_stat_io(file_name):
    """Collect memory stats per task"""
    stats = []

    # Defines offset collected stats per task
    (
        ID,
        SLAVES,
        START,
        TIME,
        PROCESS_NB,
        READ_TIME,
        WRITE_TIME,
        READ,
        WRITE,
        ALLOC,
        GRANTED,
        NAME,
    ) = (
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        11,
    )  # Keys for tuple fields in annotation families

    # Count number of process, based on existing file with slave extensions
    process_number = utils.get_process_number(file_name)

    # Analyse stat files for all processes to collect stats per task (and per inter-task the the master)
    for process_id in range(process_number):
        process_file_name = utils.build_process_filename(file_name, process_id)

        # Read data file
        data = pd.read_csv(process_file_name, delimiter="\t")

        data_time = data["Time"]
        data_label = data["Label"]
        stats_size = len(data_time)
        start_time = 0
        for i in range(stats_size):
            label = data_label[i]

            if isinstance(label, str) and label.find("driver") == 0:
                driver_type, method_name, start_or_end = extract_driver_method(label)
                if start_or_end == "Begin":
                    assert start_time == 0
                    start_time = data_time[i]

                if start_or_end == "End":
                    duration = data_time[i] - start_time
                    start_time = 0
                    stats.append([str(process_id), driver_type, method_name, duration])
    return pd.DataFrame(stats, columns=("rank", "driver", "method", "time"))


def compute_aggregates(file_name):
    """Load data from files
    ----------

    Parameters
    ----------
    file_name: str
        name of the input memory stat log file, without suffix for the master process and
        with a suffix '_<processId>' per slave process

    Returns
    ----------
    4 DataFrames :

        - all statistics
        - statitstics grouped by method
        - statitstics grouped by method and driver
        - statitstics grouped by rank, method and driver
    """
    # Loading data
    df_stats = collect_stat_io(file_name)

    if df_stats.empty:
        raise Exception("no I/O statistics to collect")
    else:
        # Aggregation by ProcessId method and driver
        df_groupby_rank = df_stats.groupby(["rank", "method", "driver"]).agg(
            min=("time", "min"),
            max=("time", "max"),
            mean=("time", "mean"),
            median=("time", "median"),
            stddev=("time", "std"),
            sum=("time", "sum"),
            count=("time", "count"),
        )

        # Aggregation by method and driver
        df_groupby_driver = df_stats.groupby(["method", "driver"]).agg(
            min=("time", "min"),
            max=("time", "max"),
            mean=("time", "mean"),
            median=("time", "median"),
            stddev=("time", "std"),
            sum=("time", "sum"),
            count=("time", "count"),
        )

        # Aggregation by method
        df_groupby_method = df_stats.groupby(["method"]).agg(
            min=("time", "min"),
            max=("time", "max"),
            mean=("time", "mean"),
            median=("time", "median"),
            stddev=("time", "std"),
            sum=("time", "sum"),
            count=("time", "count"),
        )

        df_groupby_rank.sort_values(by=["driver", "method"], inplace=True)
        df_groupby_driver.sort_values(by=["driver", "method"], inplace=True)
        df_groupby_method.sort_values(by=["method"], inplace=True)

        return df_stats, df_groupby_method, df_groupby_driver, df_groupby_rank


def compute_and_write_aggregates(memory_log_file_name):
    """Load data from files and write results in Excel files
    ----------

    Compute and write 4 DataFrames on disk :
       - 'all_stats.xlsx' : all statistics on one file
       - 'aggregate_stats.xlsx' :
            - sheet 'group_by_method' : statitstics grouped by method
            - sheet 'group_by_driver' : statitstics grouped by method and driver
            - sheet 'group_by_rank' : rouped by rank, method and driver

    Parameters
    ----------
    memory_log_file_name: str
        The name of the input memory stat log file, without suffix for the master process and
        with a suffix '_<processId>' per slave process

    """

    # Compute all aggregates
    try:
        df_stats, df_groupby_method, df_groupby_driver, df_groupby_rank = (
            compute_aggregates(memory_log_file_name)
        )
    except Exception as error:
        print("Error in IO analysis of log file " + memory_log_file_name + ":", error)
        return

    # Writes the global statistics in a separate excel file
    dir_name = os.path.dirname(memory_log_file_name)
    file_name = os.path.basename(memory_log_file_name)
    stats_file_name = os.path.splitext(file_name)[0] + ".all_stats.xlsx"
    with pd.ExcelWriter(os.path.join(dir_name, stats_file_name)) as writer:
        df_stats.to_excel(writer)

    # Writes aggregate statistics in excel sheets
    stats_file_name = os.path.splitext(file_name)[0] + ".aggregate_stats.xlsx"
    print("Save aggregate statistics file " + stats_file_name)
    with pd.ExcelWriter(os.path.join(dir_name, stats_file_name)) as writer:
        df_groupby_method.to_excel(writer, sheet_name="group_by_method")
        df_groupby_driver.to_excel(writer, sheet_name="group_by_driver")
        df_groupby_rank.to_excel(writer, sheet_name="group_by_rank")
