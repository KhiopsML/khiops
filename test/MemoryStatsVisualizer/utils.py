import os
import plotly.express as px


def build_process_filename(file_name, process_id):
    """Build process file: filename for master or with a suffix '_<processId>' per slave"""
    return (
        os.path.splitext(file_name)[0]
        + ("" if process_id == 0 else "_" + str(process_id))
        + os.path.splitext(file_name)[1]
    )


def extract_task_name(label):
    """Extract task name from a label containing 'Run Begin' or 'Run End'"""
    offset = label.find(" Run")
    task_name = label[len("Task ") : offset]
    return task_name


def get_process_number(file_name):
    """Count number of process, based on existing file with slave extensions"""

    process_number = 0
    for process_id in range(100):
        process_file_name = build_process_filename(file_name, process_id)
        if not os.path.isfile(process_file_name):
            break
        process_number += 1
    return process_number


class ColorGenerator:
    """Utility class which returns a different color at each call
    ------------------
    There are two color types : 'Bold' and 'light'

    Note : There is a round robin to prevent 'out of range' error
    """

    color_light_index = 0
    color_bold_index = 0
    colors_bold = []
    colors_light = []

    def __init__(self):
        self.initialize()
        self.colors_bold = px.colors.qualitative.Vivid + px.colors.qualitative.Alphabet
        self.colors_light = px.colors.qualitative.Pastel2 + px.colors.qualitative.Vivid

    def initialize(self):
        """Initialize indexes, to get colors at the begining"""
        self.color_light_index = 0
        self.color_bold_index = 0

    def get_color_light(self):
        """Returns a light color"""
        if self.color_light_index >= len(self.colors_light):
            self.color_light_index = 0
        color = self.colors_light[self.color_light_index]
        self.color_light_index += 1
        return color

    def get_color_bold(self):
        """Returns a bold color"""
        if self.color_bold_index >= len(self.colors_bold):
            self.color_bold_index = 0
        color = self.colors_bold[self.color_bold_index]
        self.color_bold_index += 1
        return color


if __name__ == "__main__":
    g = ColorGenerator()
    for i in range(100):
        print(g.get_color_bold())
        print(g.get_color_light())
