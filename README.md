# Challenge 1

Build and run the complete workflow inside the course container:

```bash
cd /shared-folder/challenge1_delivery
source /u/sw/etc/profile
module load gcc-glibc
module load lis
export LD_LIBRARY_PATH=${mkLisLib}:$LD_LIBRARY_PATH
make run
```

`make run` builds the program, generates the filtered images and Eigen solution,
exports the system for LIS, runs LIS with BiCGSTAB and ILU(0), converts its solution
to `outputs/lis_solution_x.png`, and appends the LIS statistics to
`outputs/results.txt`. Image dimensions are read automatically from the first run.
Temporary LIS files are removed when the workflow exits. Each complete run rewrites
`results.txt`, so LIS statistics are not duplicated.

The default input is `deer.jpg` and the default LIS executable is
`/shared-folder/lis2.1.13_test/test1`. Override them when needed:

```bash
make run IMAGE=another.jpg LIS_TEST=/path/to/lis/test1
```

To run only the image processing and Eigen part:

```bash
make eigen-run
```

For a separately computed LIS vector saved as `/tmp/x_lis.mtx`, `make lis-image`
converts it to PNG using the dimensions in `outputs/results.txt`. This standalone
target does not append LIS statistics.
