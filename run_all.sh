#!/usr/bin/env bash
set -euo pipefail

image=${1:-deer.jpg}
lis_test=${2:-/shared-folder/lis2.1.13_test/test1}
if [[ ! -x "$lis_test" ]]; then
    printf 'LIS executable not found: %s\nSet LIS_TEST to the path of LIS test1.\n' "$lis_test" >&2
    exit 1
fi
work_dir=$(mktemp -d)
trap 'rm -rf "$work_dir"' EXIT

./challenge1 "$image"
"$lis_test" data/A2.mtx data/w_lis.mtx "$work_dir/x_lis.mtx" "$work_dir/hist_lis.txt" \
    -i bicgstab -p ilu -tol 1.0e-12 -maxiter 5000 | tee "$work_dir/lis_output.txt"

lis_iter=$(awk -F'= ' '/number of iterations/{print $2; exit}' "$work_dir/lis_output.txt")
lis_res=$(awk -F'= ' '/relative residual/{print $2; exit}' "$work_dir/lis_output.txt")
if [[ -z "$lis_iter" || -z "$lis_res" ]]; then
    printf 'LIS output does not contain the expected solver statistics.\n' >&2
    exit 1
fi
rows=$(awk '/^image_rows_m:/{print $2}' outputs/results.txt)
cols=$(awk '/^image_cols_n:/{print $2}' outputs/results.txt)
./challenge1 lis-image "$work_dir/x_lis.mtx" outputs/lis_solution_x.png "$rows" "$cols"
printf 'LIS_solver: BiCGSTAB\nLIS_preconditioner: ILU(0)\nLIS_tolerance: 1e-12\nLIS_iterations: %s\nLIS_final_relative_residual: %s\n' \
    "$lis_iter" "$lis_res" >> outputs/results.txt
