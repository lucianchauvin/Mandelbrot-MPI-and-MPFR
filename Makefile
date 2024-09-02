mandelmake: mandel.c
	mpicc -Ofast -g -L/usr/local/lib -L/usr/lib -o mandel.out mandel.c -Wno-incompatible-pointer-types -Wno-int-conversion -Wreturn-mismatch -lmpfr -lgmp -lm

scorep: 
	scorep mpicc -Ofast -g -L/usr/local/lib -L/usr/lib -o mandel.out mandel.c -Wno-incompatible-pointer-types -Wno-int-conversion -Wreturn-mismatch -lmpfr -lgmp -lm

rebuild: clean all

run_simple:
	mpirun -v -n 2 ./mandel.out
run:
	mpirun -v -n 10 ./mandel.out
video_60:
	ffmpeg -framerate 60 -i frames/%d.ppm -c:v libx264 -crf 25 -vf "scale=1920:1080,format=yuv420p" -preset veryslow -movflags +faststart frames/output.mp4
video_60h:
	ffmpeg -framerate 60 -i frames/%d.ppm -c:v libx264 -crf 25 -vf "scale=1920:1080,format=yuv420p" -preset veryslow -movflags +faststart frames/output.avi
video_30:
	ffmpeg -framerate 30 -i frames/%d.ppm -c:v libx264 -crf 25 -vf "scale=1920:1080,format=yuv420p" -preset veryslow -movflags +faststart frames/output.mp4
video_30h:
	ffmpeg -framerate 30 -i frames/%d.ppm -c:v libx264 -crf 25 -vf "scale=1920:1080,format=yuv420p" -preset veryslow -movflags +faststart frames/output.avi
clean: 
	rm ./mandel.out
	rm -r ./frames/*
