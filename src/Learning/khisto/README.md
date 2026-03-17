khisto: Khiops Histograms Tool
==============================

## How to use

Usage: `khisto [OPTIONS] INPUT_FILE HISTOGRAM`
The output HISTOGRAM contains bin details: lower bound, upper bound, bin width, frequency, probability, and density.

Options:
- -b: Read input as binary (8-byte doubles) instead of text
- -e: Generate a series of histograms with increasing accuracy for exploratory analysis
- -j: Save all outputs in a single JSON file
- -h: Display help message and exit
- -v: Show version information and exit

The histogram aims to be as accurate and interpretable as possible.
Using -e, all histograms are exported in order of increasing precision.
Each histogram in the series has an index suffix (e.g., ".1"), and a .series file provides indicators for each.
Combining -j with -e consolidates all outputs into one file.

The indicators produced for the series of histograms are detailed in [(Boulle, 2024)](#Reference):
- File name: histogram file name,
- Granularity: size (by power of two) of the elementary bins on which the intervals are built,
- Interval number: number of intervals of the histogram,
- Peak interval number: a "peak" is an interval whose density is greater than that of its preceding
  and following intervals,
- Spike interval number: a "spike" is a peak containing a single value,
- Empty interval number: an empty interval contains no instances,
- Level: indicator between 0 and 1, which evaluates the quality of the density estimate,
- Information rate: normalized level, between 0 and 1, with 1 for the most accurate and
  interpretable histogram,
- Truncation epsilon: difference between two closest consecutive values (ex: 1 for integer data),
  used in the "truncation heuristic",
- Removed singularity number: number of intervals removed during the "singularity removal
  heuristic", where a "singularity" is a spike whose preceding and following intervals are empty,
- Raw: indicates the histogram is not "interpretable", as it was obtained before the "truncation
  "heuristic" and the "singularity removal heuristic" were applied.

Using the JSON format, the "truncation epsilon" and "removed singularity number" indicators are
given only once, since they are the same along the histogram series. The "raw" indicator, which
concerns at most the last histograms of the series, can be deduced from the "histogramNumber" and
"interpretableHistogramNumber" fields in the JSON format.

## Examples

Basic use:
`khisto gaussian.txt gaussian_histogram.csv`

Using the `-b` option with a binary input data file:
`khisto -b gaussian.bin gaussian_histogram.csv`

Using the `-e` option:
`khisto -e gaussian.txt gaussian_histogram.csv`

In complement to the resulting histogram gaussian_histogram.csv, the following file are output:
- gaussian_histogram.series.csv: synthetic indicators per histogram produced in the series
- gaussian_histogram.1.csv: first histogram of the series
- gaussian_histogram.2.csv: second histogram of the series
- ...

Using the `-j` option:
```bash
khisto -j gaussian.txt gaussian_histogram.json
khisto -j -e gaussian.txt gaussian_histogram.json
```

## Technical Limits

For human readability reasons, all input values exploit a value domain with a 10-digit mantissa and an exponent between 10^-100 and 10^100. The total number of input values cannot exceed 2^31, that is about two billions.


## Reference

Marc Boulle 2024 [Floating-point histograms for exploratory analysis of large scale real-world data sets](https://journals.sagepub.com/doi/abs/10.3233/IDA-230638)
