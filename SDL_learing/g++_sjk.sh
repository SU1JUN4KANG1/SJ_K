#!/bin/sh



	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/sj_k1/Downloads/ffm_res_with_aac_x264/lib
	export C_INCLUDE_PATH=$C_INCLUDE_PATH:/home/sj_k1/Downloads/ffm_res_with_aac_x264/include/
	
	
	export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/home/sj_k1/Downloads/ffm_res_with_aac_x264/include/
	export LIBRARY_PATH=$LIBRARY_PATH:/home/sj_k1/Downloads/ffm_res_with_aac_x264/lib

	#g++ yuv2jpg2.cpp -o yuv2jpg2  -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale 


	#g++ h.264_display.cpp -o h.264_display  -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale -lSDL2 -lSDL2main -lSDL2_test


	g++ ./7.cpp -o 7test -L/usr/lib/x86_64-linux-gnu -lSDL -lSDL_ttf -lSDL_mixer -lSDL_image







