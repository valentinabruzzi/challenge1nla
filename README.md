# Challenge 1

Build and run inside the course container:

```bash
cd /shared-folder/challenge1_delivery
source /u/sw/etc/profile
module load gcc-glibc
make
./challenge1 deer.jpg
```

Solve the LIS system and generate the corresponding image:

```bash
module load lis
export LD_LIBRARY_PATH=${mkLisLib}:$LD_LIBRARY_PATH
/shared-folder/lis2.1.13_test/test1 data/A2.mtx data/w_lis.mtx /tmp/x_lis.mtx /tmp/hist_lis.txt -i bicgstab -p ilu -tol 1.0e-12 -maxiter 5000 | tee /tmp/lis_output.txt
make lis-image
lis_iter=$(awk -F'= ' '/number of iterations/{print $2}' /tmp/lis_output.txt 2>/dev/null)
lis_res=$(awk -F'= ' '/relative residual/{print $2}' /tmp/lis_output.txt 2>/dev/null)
printf "LIS_solver: BiCGSTAB\nLIS_preconditioner: ILU(0)\nLIS_tolerance: 1e-12\nLIS_iterations: %s\nLIS_final_relative_residual: %s\n" "$lis_iter" "$lis_res" >> outputs/results.txt
```
