#!/bin/sh
echo "disposable 0.02 0.02 16"
time ./disposable 0.02 0.02 16 < testgrid_400_12206
echo "disposable 0.02 0.02 20"
time ./disposable 0.02 0.02 20 < testgrid_400_12206
echo "disposable 0.02 0.02 24"
time ./disposable 0.02 0.02 24 < testgrid_400_12206
echo "persistent 0.02 0.02 16"
time ./persistent 0.02 0.02 16 < testgrid_400_12206
echo "persistent 0.02 0.02 20"
time ./persistent 0.02 0.02 20 < testgrid_400_12206
echo "persistent 0.02 0.02 24"
time ./persistent 0.02 0.02 24 < testgrid_400_12206
