# Copyright (c) 2023-2025 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

# Installation of prerequisites using pip
#  python -m pip install "ipywidgets>=7.2"
#  python -m pip install "notebook>=5.3"
#  python -m pip install pandas
#  python -m pip install plotly

# Documentation de plotly:
#  https://plotly.com/python/
#  https://plotly.com/python/getting-started/
#  https://plotly.com/python/reference/
#  https://plotly.com/python/text-and-annotations/

import random
import os
import re
from select import select

import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import plotly.io as pio
import numpy as np
import utils

pio.renderers.default = "browser"

"""
Visualizer for memory stats log produced by the Khiops tool

It takes log file as input and visualize them in a browser, with curves per memory stats and
summary bars per process.
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

# Specify possible views, with subset of variables selected for visualization
view_curves = {
    "Memory": ["Heap mem", "Granted"],
    "Allocs": ["Max alloc", "Alloc"],
    "Heap": ["Max heap mem", "Heap mem"],
    "TotalMemory": ["Total granted size", "Total free size"],
    "TotalAlloc": ["Total alloc", "Total free"],
    "TotalRequestedMemory": ["Total granted size", "Total requested size"],
}


def memory_stats_visualizer(
    file_name, current_view, fig_title=None, save_as_html=False
):
    """Visualize Khiops memory log file
    Parameters:
        file_name: name of the input memory stat log file, without suffix for the master process and
          with a suffix '_<processId>' per slave process
        current_view: chose among the variable or view names which curves to plot
        fig_title: specific figure title
        save_as_html: to save the figure as a html file, that can be used to launch
          the visualization without the log files
    """
    assert os.path.isfile(file_name), "Missing file " + file_name
    assert current_view in view_curves or current_view in memory_stats_column_names, (
        'View "' + current_view + '" is missing in available views or column names'
    )

    #################################################################################
    # Internal functions

    def add_bars(local_process_id, local_family, local_figure, local_process_legends):
        """Add bars on figure
        Input:
            local_process_id: id of process (0 for master)
            local_family: family of annotations
            local_figure: FIG_MEMORY_STATS, to display full bar on main figure
                    FIG_TASKS: to display task bars per process
            local_process_legends: dictionnaire des legendes deja affichees
        """
        x_ref = 0
        annotation_index = 0
        annotation_depth = 0
        bars_x = []
        bars_width = []
        bars_opacity = []
        bars_text = []
        bar_height_delta = 0.01
        if process_number == 1 or local_figure == FIG_MEMORY_STATS:
            local_name_suffix = ""
        else:
            local_name_suffix = (
                " Master"
                if local_process_id == 0
                else " (S" + str(local_process_id) + ")"
            )
        # Retrieve annotation prameters
        annotation_x = annotation_parameters[local_process_id, local_family][X]
        annotation_text = annotation_parameters[local_process_id, local_family][TEXT]

        # Analyse annnotations
        # Stack of annotations, according to the depth of the (Begin, End) couples
        hovertext_stack = []
        for i in range(len(annotation_x)):
            x = annotation_x[i]
            local_label = annotation_text[i]
            # Identify begin and end of task from labels
            is_begin = " Begin" in local_label or i == len(annotation_x) - 1
            is_end = " End" in local_label

            # Add bar per task and per inter-task
            if is_begin or is_end:
                # Increment annotation position
                if is_begin:
                    annotation_depth += 1
                else:
                    annotation_depth -= 1
                    assert annotation_depth >= 0, (
                        "Annotation depth below 0: " + local_label
                    )
                if is_end:
                    annotation_index += 1

                # Compute current hovertext
                hovertext = ""
                if (
                    local_figure == FIG_MEMORY_STATS
                ):  # Task index is added only for memory stats figure
                    hovertext = local_family + " " + str(annotation_index) + "<br>"
                # Clean Begin and End keywords
                if is_begin:
                    hovertext += local_label[: local_label.find(" Begin")]
                else:
                    hovertext += local_label[: local_label.find(" End")]
                # Add time and duration information
                hovertext += local_name_suffix
                hovertext += "<br>Start: " + "{:.3f}".format(x_ref)
                hovertext += "<br>Stop: " + "{:.3f}".format(x)
                hovertext += "<br>Duration: " + "{:.3f}".format(x - x_ref)

                # Specify the hovertext to use
                if is_end:
                    hovertext_stack.pop()  # Pop last hovertext
                else:
                    hovertext_stack.append(hovertext)  # Push new hovertext
                    # In case of nested annotation, use the preceding annotatation related to a Begin
                    if annotation_depth > 1:
                        hovertext = hovertext_stack[len(hovertext_stack) - 2]
                    # Add an inter-annoation label at the deapth 1
                    else:
                        hovertext = "Inter " + local_family
                        hovertext += "<br>Start: " + "{:.3f}".format(x_ref)
                        hovertext += "<br>Stop: " + "{:.3f}".format(x)
                        hovertext += "<br>Duration: " + "{:.3f}".format(x - x_ref)

                # Add bar spec
                if local_figure == FIG_MEMORY_STATS or is_end or annotation_depth > 1:
                    width = x - x_ref
                    bars_x.append(x_ref + width / 2)
                    bars_width.append(width)
                    bars_opacity.append(
                        0
                        if (is_begin and annotation_depth == 1)
                        else (3 + annotation_index % 2) / 4
                    )
                    bars_text.append(hovertext)
                x_ref = x

        # Specify bar parameters
        figure_height = global_max_y - global_min_y
        # Bar on full figure for memory stats figure
        if local_figure == FIG_MEMORY_STATS:
            bar_height = figure_height * (1 + bar_height_delta)
            base = global_min_y - figure_height * bar_height_delta / 2
            color = task_color
            bar_name = "Memory stats " + local_family
        # On bar per process on process figure
        else:
            bar_height = annotation_families[local_family][BAR_PERCENT]
            base = (
                -1.25 * local_process_id
                + (1 - annotation_families[local_family][BAR_PERCENT]) / 2
            )
            color = annotation_families[local_family][COLOR]
            bar_name = local_family
            # Specify bar position and label on y axis for process figure, only once
            if local_family == "Task":
                task_tickvals.append(base + 0.5)
                # add hostname to the chart if they are in the log
                if len(hostnames) > 0:
                    task_ticktext.append(
                        "Master " + hostnames[local_process_id]
                        if local_process_id == 0
                        else "Slave "
                        + str(local_process_id)
                        + " "
                        + hostnames[local_process_id]
                    )
                else:
                    task_ticktext.append(
                        "Master"
                        if local_process_id == 0
                        else "Slave " + str(local_process_id)
                    )
        # Fill missing parameters
        bars_y = []
        bars_color = []
        for i in range(len(bars_x)):
            bars_y.append(bar_height)
            bars_color.append(color)
        # Display bars
        if len(bars_x) > 0:
            fig.add_trace(
                go.Bar(
                    x=bars_x,
                    width=bars_width,
                    y=bars_y,
                    base=base,
                    marker_color=bars_color,
                    marker_opacity=bars_opacity,
                    marker_line_width=0,
                    hovertext=bars_text,
                    hoverinfo="text",
                    legendgroup=bar_name,
                    name=bar_name,
                    showlegend=bar_name not in local_process_legends,
                ),
                row=local_figure,
                col=1,
            )
            local_process_legends[bar_name] = True

    #########################################################################################
    # Main routine

    # Main parameters
    show_annotations = True  # To add annotation ticks on memory stat curves
    # To add a sub panel with the task bars per process
    show_process_figure = True
    process_figure_max_height_ratio = 0.7  # Max height ratio of task figure
    # If true, the figure height can be lower that it max height in case of few processes
    process_figure_auto_scale = False
    # IO annotations are numerous and are not shown by default, to increase readability and performance
    show_io_annotations = False
    # IO process bars could be set to False to increase performance
    show_io_process_bars = True
    # Percentage to keep in curves: all annoation are kept, by the other points can be sampled for scalability issues
    log_file_sampling_rate = 1
    # Color of task panels that are displayed on top of memory stats curves
    task_color = "lightgray"
    line_width = 1  # Width of curves

    cg = utils.ColorGenerator()

    # Huge file parameters: might be set automatically in the future
    huge_file_parameters = False
    if huge_file_parameters:
        show_annotations = False
        log_file_sampling_rate = 0.01
        show_io_process_bars = False

    # Parameters for annotations, defined by tuples for each family of annotation
    #   ANNOTATION_VISIBLE: flag to draw annotation markers on memory stats curves
    #   PROCESS_VISIBLE: flag to draw annotation sub-bars in process bars
    #   SYMBOL: marker used for annotation
    #   COLOR: color used for markers and sub-bars
    #   SIZE: size of markers
    #   BAR_PERCENT: percentage of process bar used by annotation sub-bars
    #   COMPILED_REGEX: compiled version of regex
    #   REGEX: text version of regex
    (
        ANNOTATION_VISIBLE,
        PROCESS_VISIBLE,
        SYMBOL,
        COLOR,
        SIZE,
        BAR_PERCENT,
        COMPILED_REGEX,
        REGEX,
    ) = (
        0,
        1,
        2,
        3,
        4,
        5,
        6,
        7,
    )  # Keys for tuple fields in annotation families
    # The annotation and subs-bars are drawn accroding to their order of declaration
    annotation_families = {}
    # Main tasks
    annotation_families["Task"] = [
        True,
        True,
        "circle",
        cg.get_color_light(),
        8,
        1.0,
        None,
        "Task.* Run [Begin|End]",
    ]
    # Master sub-routines of tasks
    annotation_families["Master"] = [
        True,
        True,
        "diamond",
        cg.get_color_light(),
        3,
        0.7,
        None,
        "Task.* "
        + "[.][ComputeResourceRequirements|MasterInitialize|MasterPrepareTaskInput|MasterAggregateResults].* [Begin|End]",
    ]
    # Slave sub-routines of tasks
    annotation_families["Slave"] = [
        True,
        True,
        "diamond",
        cg.get_color_light(),
        5,
        0.7,
        None,
        "Task.* [.][SlaveInitialize|SlaveFinalize|SlaveProcess].* [Begin|End]",
    ]
    # Anything that do not match any of the regex
    annotation_families["Other"] = [
        True,
        True,
        "triangle-up",
        cg.get_color_bold(),
        5,
        0.5,
        None,
        None,
    ]
    # Reports (all the .xls and .json files output by Khiops)
    annotation_families["Report"] = [
        True,
        True,
        "square",
        cg.get_color_bold(),
        4,
        0.5,
        None,
        ".* Write report [Begin|End]",
    ]
    # All routines related to the management of dictionaries
    annotation_families["Dictionary"] = [
        True,
        True,
        "pentagon",
        cg.get_color_bold(),
        4,
        0.5,
        None,
        "Dictionary .* [Begin|End]",
    ]
    # IO read and write basic routines
    # These annotations are numerous and are not shown by default; even their visibility in the process bars can be set
    # to False to increase the performance
    annotation_families["IO read"] = [
        show_io_annotations,
        show_io_process_bars,
        "triangle-left",
        cg.get_color_bold(),
        2,
        0.4,
        None,
        "(File .*Fill [Begin|End])|(File buffer ReadFromFile [Begin|End])",
    ]
    annotation_families["IO write"] = [
        show_io_annotations,
        show_io_process_bars,
        "triangle-right",
        cg.get_color_bold(),
        2,
        0.4,
        None,
        "(File Flush [Begin|End])|(File buffer WriteToFile [Begin|End])",
    ]

    annotation_families["Driver connect"] = [
        show_io_annotations,
        show_io_process_bars,
        "triangle-right",
        cg.get_color_bold(),
        2,
        0.1,
        None,
        "(driver.*Connect [Begin|End])",
    ]

    annotation_families["Driver ANSI"] = [
        show_io_annotations,
        show_io_process_bars,
        "triangle-right",
        cg.get_color_bold(),
        2,
        0.1,
        None,
        "(driver.*ANSI.*[Begin|End])",
    ]

    annotation_families["Driver hdfs"] = [
        show_io_annotations,
        show_io_process_bars,
        "triangle-right",
        cg.get_color_bold(),
        2,
        0.1,
        None,
        "(driver.*HDFS.*[Begin|End])",
    ]

    # Compile all regex and retrieve default family, with None regex
    none_family = None
    for family in annotation_families:
        if annotation_families[family][REGEX] is None:
            none_family = family
        else:
            annotation_families[family][COMPILED_REGEX] = re.compile(
                annotation_families[family][REGEX]
            )

    # Initialize annotation parameters, defines by tuples of three vectors for X, Y and TEXT
    (X, Y, TEXT) = (0, 1, 2)  # Keys for tuple fields in annotation parameters
    # Dictionary of annotation parameters, per (processId, family)
    annotation_parameters = {}

    # Count number of process, based on existing file with slave extensions
    process_number = utils.get_process_number(file_name)

    # Position and label of process ticks on y axis of bar figure
    task_tickvals = []
    task_ticktext = []

    # Initialize figure with two stacked subplots for memory stats curves and process bars
    FIG_MEMORY_STATS = 1  # Row index for memory stats figure
    FIG_PROCESSES = 2  # Row index for process figure
    if show_process_figure:
        # Proportion of each subplot
        if process_figure_auto_scale:
            task_proportion = min(
                max(0.05, process_figure_max_height_ratio), 0.05 * process_number
            )
        else:
            task_proportion = process_figure_max_height_ratio
        # Initialize figure with two subplots
        fig = make_subplots(
            rows=2,
            cols=1,
            shared_xaxes=True,
            vertical_spacing=0.01,
            row_heights=[1 - task_proportion, task_proportion],
            row_titles=("Memory stats", "Processes"),
        )
    # Initialize one single figure for memory stats curves
    else:
        fig = make_subplots(rows=1, cols=1)

    # Host names list, at index 0 the hostname of process id O
    hostnames = []

    # Limits to countrol the max number of log lines that can be managed by curves
    max_slave_process_number = 100
    max_empty_label_line_number = 10000
    empty_label_line_sampling_frequency = None  # Will be computed once

    # Loop on files to collect curves and annotation vectors
    name_suffix = ""
    extra_width = 0
    global_min_y = 1e100
    global_max_y = -1e100

    for process_id in range(process_number):
        process_file_name = utils.build_process_filename(file_name, process_id)

        # Specific parameters per process
        if process_number > 1:
            name_suffix = (
                " Master" if process_id == 0 else " (S" + str(process_id) + ")"
            )
            extra_width = 1 if process_id == 0 else 0

        # Replace ',' with '.'
        with open(process_file_name, "r") as fProcess:
            lines = fProcess.readlines()
        with open(process_file_name, "w") as fProcess:
            for line in lines:
                fProcess.write(line.replace(",", "."))

        # Read data file
        data = pd.read_csv(process_file_name, delimiter="\t")
        assert "Time" in data, 'Variable "Time" is missing in file ' + process_file_name
        assert "Label" in data, (
            'Variable "Label" is missing in file ' + process_file_name
        )

        # Compute the ratio of lines with empty label to use, only for the main process
        if empty_label_line_sampling_frequency is None:
            line_number = len(data)
            empty_label_line_number = line_number - len(data[data.Label.notnull()])
            empty_label_line_sampling_frequency = 1
            if empty_label_line_number > max_empty_label_line_number:
                empty_label_line_sampling_frequency = (
                    empty_label_line_number // max_empty_label_line_number
                )

        # Add column to allow advance selections
        selection_column = []
        number = 0
        for ind in data.index:
            label = data["Label"][ind]
            # Case where there no label
            if not isinstance(label, str) or label == "":
                number += 1
                selection_column.append(
                    number % empty_label_line_sampling_frequency == 0
                )
            # Case of a non empty label
            else:
                # Search an index in the label (corresponds to a process index)
                # To do: case of a log related to File access
                index = -1
                label_fields = label.split(" ")
                for field in label_fields:
                    try:
                        index = int(field)
                    except ValueError:
                        index = -1
                    if index >= 0:
                        break
                # Select line if non index or index below limits
                selection = index <= max_slave_process_number
                if not selection:
                    # Sampling if not selected, with decreasing sampling rate
                    sampling_base = 1
                    while index > max_slave_process_number * sampling_base:
                        sampling_base *= 2
                        selection = index % sampling_base == 0
                selection_column.append(selection)
        data["Selection"] = selection_column

        # Filter data: max number of line with no label (memory) and max number of task processes
        filtered_data = data[data.Selection]
        if filtered_data.shape != data.shape:
            print(
                "\tFiltered data for visualization : "
                + str(filtered_data.shape)
                + " from "
                + str(data.shape)
            )
        else:
            print("\tUsed data for visualization: " + str(filtered_data.shape))
        data = filtered_data

        # Select one curve or all curves of a view
        if current_view in memory_stats_column_names:
            current_curves = [current_view]
        else:
            current_curves = view_curves[current_view]
        last_curve = current_curves[len(current_curves) - 1]

        # Draw curves of selected view
        for curve in current_curves:
            assert curve in data, (
                'Variable "' + curve + '" is missing in file ' + process_file_name
            )
            assert curve != "Time", "Variable Time is not allowed"
            assert curve != "Label", "Variable Label is not allowed"
            # Update global min and max Y
            global_min_y = min(global_min_y, min(data[curve]))
            global_max_y = max(global_max_y, max(data[curve]))
            # Extract data, with sampling if necessary
            if log_file_sampling_rate == 1:
                x_data = data["Time"]
                y_data = data[curve]
            else:
                assert 0 < log_file_sampling_rate and log_file_sampling_rate < 1, (
                    "Sampling rate "
                    + str(log_file_sampling_rate)
                    + " must be between 0 and 1"
                )
                x_data = []
                y_data = []
                for t in range(len(data["Time"])):
                    if random.random() < log_file_sampling_rate:
                        x_data.append(data["Time"][t])
                        y_data.append(data[curve][t])
            # Add curve on figure
            fig.add_trace(
                go.Scatter(
                    x=x_data,
                    y=y_data,
                    mode="lines",
                    name=curve + name_suffix,
                    legendgroup=(
                        "Labels" + str(process_id) if curve == last_curve else ""
                    ),
                    line_width=line_width + extra_width,
                )
            )

        # Collect annotations, tuples of X, Y and Text vectors
        times = data["Time"]
        # Annotations are positioned only i=n the last curve
        ylabels = data[last_curve]
        labels = data["Label"]
        # Initialize annotation parameters for the processId
        for family in annotation_families:
            annotation_parameters[process_id, family] = ([], [], [])

        # Loop on series
        for ind in data.index:
            label = labels[ind]
            # Test if the annotation is selected
            add_annotation = isinstance(label, str) and label != ""
            # Add annotation
            if add_annotation:
                # Detection of annotation family
                found_family = none_family
                for family in annotation_families:
                    compiled_regex = annotation_families[family][COMPILED_REGEX]
                    if (
                        compiled_regex is not None
                        and compiled_regex.match(label) is not None
                    ):
                        found_family = family
                        break
                # Annotation setting
                annotation_parameter = annotation_parameters[process_id, found_family]
                annotation_parameter[X].append(times[ind])
                annotation_parameter[Y].append(ylabels[ind])
                annotation_parameter[TEXT].append(label + name_suffix)

                # Hostname extraction
                if label.find("hostname") == 0:
                    start = label.index("[")
                    end = label.index("]", start)
                    hostname = label[start + 1 : end]
                    hostnames.append(hostname)

        # Add last time to show a last bar after the last task
        last_ind = data.index[-1]
        for family in annotation_families:
            annotation_parameter = annotation_parameters[process_id, family]
            # Add annotation
            annotation_parameter[X].append(times[last_ind])
            annotation_parameter[Y].append(None)
            annotation_parameter[TEXT].append(labels[last_ind])

    # Add annotation ticks on memory stat curves
    if show_annotations:
        # Loop on curves per process
        for process_id in range(process_number):
            # Add annotation per family
            for family in annotation_families:
                annotation_family = annotation_families[family]
                annotation_parameter = annotation_parameters[process_id, family]

                # Add annotation per curve
                if annotation_family[ANNOTATION_VISIBLE]:
                    fig.add_trace(
                        go.Scatter(
                            x=annotation_parameter[X],
                            y=annotation_parameter[Y],
                            mode="markers",
                            hovertext=annotation_parameter[TEXT],
                            name=family + " labels",
                            legendgroup="Labels" + str(process_id),
                            showlegend=False,
                            marker=dict(
                                symbol=annotation_family[SYMBOL],
                                color=annotation_family[COLOR],
                                size=annotation_family[SIZE] + extra_width,
                            ),
                        )
                    )

    # Global bars on memory stats figure
    process_legends = {}
    add_bars(0, "Task", FIG_MEMORY_STATS, process_legends)

    # Add bars on to show tasks
    if show_process_figure:
        # Dictionnaire des legendes deja affichees
        process_legends = {}
        # Details bars per process, and family, on the process figure
        for family in annotation_families:
            for process_id in range(process_number):
                if annotation_families[family][PROCESS_VISIBLE]:
                    add_bars(process_id, family, FIG_PROCESSES, process_legends)

    # Update axis for the memory stats figure
    yunits_label = "Allocs" if "Alloc" in current_view else "Bytes"
    fig.update_xaxes(title="Time", row=FIG_MEMORY_STATS, col=1)
    fig.update_yaxes(title=yunits_label, zeroline=True, row=FIG_MEMORY_STATS, col=1)

    # Update axis for process figure
    if show_process_figure:
        fig.update_yaxes(
            tickvals=task_tickvals,
            ticktext=task_ticktext,
            zeroline=False,
            row=FIG_PROCESSES,
            col=1,
        )
        fig.update_xaxes(title="Time", row=FIG_PROCESSES, col=1)
        fig.update_xaxes(title="", row=FIG_MEMORY_STATS, col=1)

    # Add a border for the legend
    fig.update_layout(
        legend=dict(
            bordercolor="gray",
            borderwidth=1,
            tracegroupgap=5,
            font=dict(size=12, color="black"),
        )
    )

    # Final layout
    fig.update_layout(
        title="Khiops memory log" if fig_title is None else fig_title,
        xaxis_showgrid=True,
        margin=dict(l=20, r=20, b=20, t=50),
    )

    # Draw figure
    if save_as_html:
        html_file_name = os.path.splitext(file_name)[0] + ".html"
        print("Save visualization file " + html_file_name)
        fig.write_html(html_file_name, auto_open=True)
    else:
        fig.show()
