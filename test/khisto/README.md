Basic test for khisto
-----------------

- Input: `gaussian.txt`
- Output: `gaussian_histogram.series.json` synthetic indicators per histogram produced in the series

The command to reproduce the test is:
```bash
khisto -e -j gaussian.txt gaussian_histogram.series.json
```