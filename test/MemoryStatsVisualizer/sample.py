# Copyright (c) 2023-2026 Orange. All rights reserved.
# This software is distributed under the BSD 3-Clause-clear License, the text of which is available
# at https://spdx.org/licenses/BSD-3-Clause-Clear.html or see the "LICENSE" file for more details.

import memory_stats_visualizer as mvs
import collect_stats_per_task as cspt
import collect_stat_io as csio

# Build a synthetic visualization of the logs and open it in a browers
# mvs.memory_stats_visualizer("./AdultSample/KhiopsMemoryStats.log", "Memory", save_as_html=True)
# mvs.memory_stats_visualizer("./AdultParallelSample/KhiopsMemoryStats.log", "Memory", save_as_html=True)

# Build a summary text file with stats per task
# cspt.collect_stats_by_task("./AdultSample/KhiopsMemoryStats.log")
# cspt.collect_stats_by_task("./AdultParallelSample/KhiopsMemoryStats.log")

# Build a summary Excel file with IO stats per task
# csio.compute_and_write_aggregates("./AdultSample/KhiopsMemoryStats.log")
# csio.compute_and_write_aggregates("./AdultParallelSample/KhiopsMemoryStats.log")
