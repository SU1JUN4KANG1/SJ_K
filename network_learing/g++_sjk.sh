#!/bin/sh



	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/sj_k1/Downloads/ffm_res_with_aac_x264/lib
	export C_INCLUDE_PATH=$C_INCLUDE_PATH:/home/sj_k1/Downloads/ffm_res_with_aac_x264/include/
	
	
	export CPLUS_INCLUDE_PATH=$CPLUS_INCLUDE_PATH:/home/sj_k1/Downloads/ffm_res_with_aac_x264/include/
	export LIBRARY_PATH=$LIBRARY_PATH:/home/sj_k1/Downloads/ffm_res_with_aac_x264/lib




	g++ client1.c -o client1 
	g++ server1.c -o server1 







