# Copyright (c) 2023-2025 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

import os
import re
import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import plotly.io as pio
import utils

pio.renderers.default = "browser"

"""
Study granted/requested and memHeap/granted ratio from memory log files
Aggregation of stats collected from memory log files
"""

# Variables of Khiops memory log file, that can be choosen for visualisation
#   Time  is used for the X axis of the curves
#   Label is used to annotated somme position on the curves and process, to eaase the interpretation
#   Each other variable can be choosen to select curves to visualize
memory_stats_column_names = [
    "Time",
    "Heap mem",
    "Max heap mem",
    "Total heap mem",
    "Alloc",
    "Max alloc",
    "Total alloc",
    "Total free",
    "Granted",
    "Max granted",
    "Total requested size",
    "Total granted size",
    "Total free size",
    "Label",
]


def memory_ratio_study(root_dir):
    """Collect memory stats from all Khiops memory log file in sub-directories of root dir
    Study granted/requested and memHeap/granted ratio from stat log files

    Unmaintained routine
    """
    # Browse sub-directories
    fig = go.Figure()
    for dir_name in os.listdir(root_dir):
        dir_path = os.path.join(root_dir, dir_name)
        if (
            "Win32" not in dir_name
            and "Adult" not in dir_name
            and "_" not in dir_name
            and os.path.isdir(dir_path)
        ):
            print(dir_path)
            all_granted = []
            all_heap_mem = []
            # Browse files per subdirectory
            for file_name in os.listdir(dir_path):
                file_path = os.path.join(dir_path, file_name)
                if file_name.find("KhiopsMemoryStats") == 0 and file_name.find(
                    ".log"
                ) == len(file_name) - len(".log"):
                    # Read data file
                    data = pd.read_csv(file_path, delimiter="\t")
                    if (
                        "Heap mem" in data
                        and "Granted" in data
                        and "Total requested size"
                        and "Total granted size" in data
                    ):
                        print("\t" + file_name)
                        data_granted = data["Granted"]
                        data_heap_mem = data["Heap mem"]
                        for i in range(len(data_granted)):
                            if i % 20 == 0:
                                all_granted.append(data_granted[i])
                                all_heap_mem.append(data_heap_mem[i])
            print("Values: " + str(len(all_granted)))
            fig.add_trace(
                go.Scatter(
                    x=all_granted,
                    y=all_heap_mem,
                    mode="markers",
                    name=dir_name,
                    marker=dict(symbol="circle", size=2),
                )
            )
    fig.show()


def collect_stats_by_task(file_name):
    """Collect memory stats per task"""

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
    stats_field_labels = [
        "Id",
        "Slaves",
        "Start",
        "Time",
        "Process",
        "Read T",
        "Write T",
        "Read",
        "Write",
        "GetFileSize",
        "Alloc",
        "Granted",
        "Name",
    ]
    stats_field_number = len(stats_field_labels)

    # Count number of process, based on existing file with slave extensions
    process_number = utils.get_process_number(file_name)

    # Analyse stat files for all processes to collect stats per task (and per inter-task the the master)
    all_processes_tasks = []
    for process_id in range(process_number):
        process_file_name = utils.build_process_filename(file_name, process_id)
        process_tasks = []
        all_processes_tasks.append(process_tasks)
        # Read data file
        data = pd.read_csv(process_file_name, delimiter="\t")
        data_time = data["Time"]
        data_total_alloc = data["Total alloc"]
        data_total_granted = data["Total granted size"]
        data_label = data["Label"]
        stats_size = len(data_time)
        # Collect stats per task
        inter_task_start_index = 0
        inter_task_index = 0
        total_read_time = 0
        total_write_time = 00
        total_read_number = 0
        total_write_number = 0
        total_get_file_size_number = 0
        task_name = ""
        start_index = -1
        task_process_number = 0
        start_read_index = -1
        start_write_index = -1
        for i in range(stats_size):
            label = data_label[i]
            if isinstance(label, str) and label != "":
                if "Run Begin" in label or i == stats_size - 1:
                    # Collect inter-task stats, only for process 0
                    if process_id == 0:
                        inter_task_index += 1
                        process_tasks.append(
                            [
                                "Master",
                                0,
                                data_time[inter_task_start_index],
                                data_time[i] - data_time[inter_task_start_index],
                                0,
                                total_read_time,
                                total_write_time,
                                total_read_number,
                                total_write_number,
                                total_get_file_size_number,
                                data_total_alloc[i]
                                - data_total_alloc[inter_task_start_index],
                                data_total_granted[i]
                                - data_total_granted[inter_task_start_index],
                                " Inter task " + str(inter_task_index),
                            ]
                        )
                        total_read_time = 0
                        total_write_time = 0
                        total_read_number = 0
                        total_write_number = 0
                        total_get_file_size_number = 0
                    # Initialize task stats
                    task_name = utils.extract_task_name(label)
                    start_index = i
                    task_process_number = 0
                if "Run End" in label:
                    # Collect task stats
                    assert task_name == utils.extract_task_name(label)
                    assert start_index != -1
                    process_tasks.append(
                        [
                            "Master" if process_id == 0 else "Slave" + str(process_id),
                            0 if process_id == 0 else 1,
                            data_time[start_index],
                            data_time[i] - data_time[start_index],
                            task_process_number,
                            total_read_time,
                            total_write_time,
                            total_read_number,
                            total_write_number,
                            total_get_file_size_number,
                            data_total_alloc[i]
                            - data_total_alloc[inter_task_start_index],
                            data_total_granted[i]
                            - data_total_granted[inter_task_start_index],
                            task_name,
                        ]
                    )
                    total_read_time = 0
                    total_write_time = 0
                    total_read_number = 0
                    total_write_number = 0
                    total_get_file_size_number = 0
                    # Initialize inter-task stats
                    if process_id == 0:
                        inter_task_start_index = i
                if " .SlaveProcess " in label and " Begin" in label:
                    task_process_number += 1
                if label.startswith("driver "):
                    if " fread " in label:
                        if " Begin" in label:
                            start_read_index = i
                        elif " End" in label:
                            assert start_read_index != -1
                            total_read_time += (
                                data_time[i] - data_time[start_read_index]
                            )
                            total_read_number += 1
                    elif " fwrite " in label:
                        if " Begin" in label:
                            start_write_index = i
                        elif " End" in label:
                            assert start_write_index != -1
                            total_write_time += (
                                data_time[i] - data_time[start_write_index]
                            )
                            total_write_number += 1
                    elif " GetFileSize " in label:
                        if " Begin" in label:
                            total_get_file_size_number += 1

        # Clear stats per process if no task
        if len(process_tasks) < 1:
            process_tasks.clear()
        # Collect overall stats on all the run if at least one task
        else:
            overall_stats = []
            for i in range(stats_field_number):
                overall_stats.append(0)
            overall_stats[ID] = process_tasks[0][ID]
            overall_stats[NAME] = "Overall"
            for task_stats in process_tasks:
                for i in range(stats_field_number):
                    if not isinstance(overall_stats[i], str) and not isinstance(
                        task_stats[i], str
                    ):
                        overall_stats[i] = overall_stats[i] + task_stats[i]
            overall_stats[SLAVES] = process_tasks[0][SLAVES]
            overall_stats[START] = process_tasks[0][START]
            process_tasks.append(overall_stats)

    ##########################################################
    # Collect synthetic stats over all processes

    # Initialize slave lists for each master task
    master_process_tasks = all_processes_tasks[0]
    master_process_tasks_all_slaves = []
    for _ in master_process_tasks:
        master_process_tasks_all_slaves.append([])

    # Collect slave stats for each master task
    for i, master_task in enumerate(master_process_tasks):
        # Loop on slave to collect task stats relative to master task
        for process_id in range(process_number):
            if process_id == 0:
                continue
            slave_process_tasks = all_processes_tasks[process_id]

            # Search closest master task
            best_time_distance = 1e100
            best_slave_task = None
            for slave_task in slave_process_tasks:
                if slave_task[NAME] == master_task[NAME]:
                    time_distance = abs(slave_task[START] - master_task[START])
                    if time_distance < best_time_distance:
                        best_time_distance = time_distance
                        best_slave_task = slave_task
            # Store closest slave task
            if best_slave_task is not None:
                master_process_tasks_all_slaves[i].append(best_slave_task)

    # Compute summary stats: total and mean
    if process_number > 1:
        all_processes_total_stats = []
        all_processes_mean_stats = []
        all_processes_tasks.append(all_processes_total_stats)
        all_processes_tasks.append(all_processes_mean_stats)
        for i, master_task in enumerate(master_process_tasks):
            # Initialise total with master stats
            total_stats = master_task.copy()
            all_processes_total_stats.append(total_stats)

            # Update total stats with stats collected from slaves
            all_slaves = master_process_tasks_all_slaves[i]
            for slave_task in all_slaves:
                for n in range(stats_field_number):
                    if not isinstance(total_stats[n], str):
                        total_stats[n] += slave_task[n]
            total_stats[ID] = "Total"
            total_stats[START] = master_task[START]

            # Initialise mean with master stats
            mean_stats = master_task.copy()
            all_processes_mean_stats.append(mean_stats)
            for n in range(stats_field_number):
                if not isinstance(mean_stats[n], str):
                    mean_stats[n] = 0

            # Update total stats with stats collected from slaves
            all_slaves = master_process_tasks_all_slaves[i]
            for slave_task in all_slaves:
                for n in range(stats_field_number):
                    if not isinstance(mean_stats[n], str):
                        mean_stats[n] += slave_task[n]
            for n in range(stats_field_number):
                if not isinstance(mean_stats[n], str):
                    mean_stats[n] /= max(1, len(all_slaves))
            mean_stats[ID] = "Mean(S)"
            mean_stats[START] = master_task[START]

    # Display collected stats
    stats_file_name = os.path.splitext(file_name)[0] + ".Summary.txt"
    print("Save stats summary file file " + stats_file_name)
    file_stats = open(stats_file_name, "w")
    title = ""
    for i in range(stats_field_number):
        if i > 0:
            title += "\t"
        title += stats_field_labels[i]
    # print(title)
    file_stats.write(title + "\n")
    for process_id in range(len(all_processes_tasks)):
        process_tasks = all_processes_tasks[process_id]
        for p in range(len(process_tasks)):
            task_stats = process_tasks[p]
            line = ""
            file_line = ""
            for i in range(stats_field_number):
                if i > 0:
                    line += "\t"
                    file_line += "\t"
                if isinstance(task_stats[i], str):
                    line += task_stats[i]
                elif task_stats[i] >= 100000:
                    line += "%.1E" % task_stats[i]
                elif task_stats[i] > int(task_stats[i]):
                    line += "%.1f" % task_stats[i]
                else:
                    line += str(task_stats[i])
                file_line += str(task_stats[i])
            # print(line)
            file_stats.write(file_line + "\n")
    file_stats.close()
