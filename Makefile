mandelmake: mandel.c
	gcc -g -fopenmp -o mandel.out mandel.c -Wall -lmpfr -lgmp -lm

rebuild: clean all

video_60:
	ffmpeg -framerate 60 -i frames/%d.ppm -c:v libx264 -crf 25 -vf "scale=1920:1080,format=yuv420p" -preset veryslow -movflags +faststart frames/output.mp4

video_60h:
	ffmpeg -framerate 60 -i frames/%d.ppm -c:v libx264 -crf 25 -vf "scale=1920:1080,format=yuv420p" -preset veryslow -movflags +faststart frames/output.avi

video_30:
	ffmpeg -framerate 30 -i frames/%d.ppm -c:v libx264 -crf 25 -vf "scale=1920:1080,format=yuv420p" -preset veryslow -movflags +faststart frames/output.mp4

video_30h:
	ffmpeg -framerate 30 -i frames/%d.ppm -c:v libx264 -crf 25 -vf "scale=1920:1080,format=yuv420p" -preset veryslow -movflags +faststart frames/output.avi

clean: 
	rm -f ./mandel.out mandel_ptrb.out
	rm -rf ./frames/*
