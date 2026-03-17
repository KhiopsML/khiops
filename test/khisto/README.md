# Basic test for khisto

## Using text format

- Input: `gaussian.txt`
- Output: `gaussian_histogram.series.json` synthetic indicators per histogram produced in the series

The command to reproduce the test is:
```bash
khisto -e -j gaussian.txt gaussian_histogram.series.json
```

## Using binary format

- Input: `gaussian.bin`
- Output: `gaussian_histogram.series.bin.json` synthetic indicators per histogram produced in the series

The command to reproduce the test is:
```bash
khisto -b -e -j gaussian.txt gaussian_histogram.series.bin.json
```

The json result file must be exactly the same if the data in binary format is the same as in  text format.

## Converting Text Format to Binary Format

The following Python code reads data in text format and exports it in binary format:
```python
import numpy as np

data = np.loadtxt('gaussian.txt')
data.tofile('gaussian.bin')
```

Warning: Beware of endianness, which may differ on ARM platforms.