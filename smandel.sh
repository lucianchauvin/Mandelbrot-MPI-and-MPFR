#!/bin/bash
#SBATCH --nodes 6
#SBATCH --tasks-per-node 16

mpirun -n 80 ./mandel.out
#export LD_LIBRARY_PATH=/usr/local/lib
#ffmpeg -framerate 45 -i %d.ppm -c:v libx264 -crf 25 -vf "scale=1920:1200,format=yuv420p" -movflags +faststart output.mp4

