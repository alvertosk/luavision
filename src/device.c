/*
=========================================================================

 Copyright (C) 2013-2014 Yannis Gravezas <wizgrav@gmail.com>

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 
==========================================================================

*/
 
#include "vision.h"

Vision_t Vision = {{{0},{0},{0}},NULL,NULL,NULL,0,0,0,0,0,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_COND_INITIALIZER,{0},NULL,NULL,0,0,NULL,0};



static void chunk_cb(void *buffer, void *pkt_data, int pkt_num, int pkt_size,void *ud)
{
	int n;
	uint8_t *raw = (uint8_t *) pkt_data;
	uint16_t *frame=(uint16_t *)buffer;
	uint16_t *mm = (uint16_t *) Vision.depthtomm; 
	if(pkt_num == 73 || pkt_num == 146) return;
	if(pkt_num > 219){
		raw += (pkt_num-220) * 12;
		frame += 320 * (pkt_num-2);
	}else if(pkt_num > 146){
		raw += (pkt_num-147) * 12 + 4;
		frame += 320 * (pkt_num-2);
	}else if(pkt_num > 73){
		raw += (pkt_num-74) * 12 + 8;
		frame += 320 * (pkt_num-1);
	}else{
		raw += pkt_num * 12;
		frame += 320 * pkt_num;
	}
	for(n=40;n--;){
		frame[0] =  mm[(raw[0]<<3)  | (raw[1]>>5)];
        frame[1] = mm[((raw[2]<<9)  | (raw[3]<<1) | (raw[4]>>7) ) & 2047];
        frame[2] = mm[((raw[5]<<7)  | (raw[6]>>1) ) & 2047];
        frame[3] = mm[((raw[8]<<5)  | (raw[9]>>3) ) & 2047];
        frame[4] =  mm[(raw[11]<<3)  | (raw[12]>>5)];
        frame[5] = mm[((raw[13]<<9)  | (raw[14]<<1) | (raw[15]>>7) ) & 2047];
        frame[6] = mm[((raw[16]<<7)  | (raw[17]>>1) ) & 2047];
        frame[7] = mm[((raw[19]<<5)  | (raw[20]>>3) ) & 2047];
        frame+=8;
        raw+=22;
    }
}

static void depth_cb(freenect_device* dev, void *v_depth, uint32_t timestamp)
{
  frame_t *frame;
  struct timeval tv;
  double realtime;
  gettimeofday(&tv,NULL);
  realtime = ((double)tv.tv_sec)+(((double)tv.tv_usec)/1000000);
  do{Vision.count++;}while(!Vision.count);
  Vision.working->count = Vision.count;
  Vision.working->realtime = realtime;
  Vision.working->stamp=timestamp;
  pthread_mutex_lock(&Vision.mutex);
  frame = Vision.working;
  Vision.working = Vision.spare;
  Vision.spare=frame;
  if (Vision.wait<2) {
    Vision.wait++;
    pthread_cond_broadcast(&Vision.condition);
  }
  pthread_mutex_unlock(&Vision.mutex);
  freenect_set_depth_buffer(Vision.dev,Vision.working->data);
}

void *thread_func(void *arg){
  int i,j,ni,first,wait;
  uint32_t count;
  double now,last;
  freenect_registration reg;
  Vision.working = &Vision.frames[0];
  Vision.spare = &Vision.frames[1];
  Vision.current = &Vision.frames[2];
  Vision.current->count = 0;
  wait=0;
  if (freenect_init(&Vision.ctx, NULL) < 0){ return NULL;}
  freenect_set_log_level(Vision.ctx, FREENECT_LOG_FATAL);
  freenect_select_subdevices(Vision.ctx, (freenect_device_flags)(FREENECT_DEVICE_CAMERA));

  while(Vision.active > 0){
	if(wait){
		wait--;
		MSLEEP();
		continue;
	}

	i=Vision.dev?freenect_process_events(Vision.ctx):-1;
	if(i < 0){
	  if(Vision.dev){
		freenect_stop_depth(Vision.dev);
		freenect_close_device(Vision.dev);
		Vision.dev=NULL;
	  }
	  i = freenect_open_device(Vision.ctx, &Vision.dev, 0);
	  if(i >= 0){
		Vision.reset++;
		freenect_set_depth_buffer(Vision.dev,Vision.working->data);
		freenect_set_depth_mode(Vision.dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT_PACKED));
		freenect_set_video_mode(Vision.dev, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_BAYER));
		freenect_set_depth_callback(Vision.dev, depth_cb);
		freenect_set_depth_chunk_callback(Vision.dev, chunk_cb);
		freenect_start_depth(Vision.dev);
		reg=freenect_copy_registration(Vision.dev);
		Vision.factor = 4*reg.zero_plane_info.reference_pixel_size/reg.zero_plane_info.reference_distance;
		Vision.depthtomm[0]=0;
		for(i=1; i!=2048; i++) Vision.depthtomm[i] = MIN(reg.raw_to_mm_shift[i],8191);
		freenect_destroy_registration(&reg);
	  }
	  wait=30;
	}
	
  }
  
  if(Vision.dev){
	freenect_stop_depth(Vision.dev);
	freenect_close_device(Vision.dev);
	Vision.dev=NULL;
  }
  
  if(Vision.ctx){
	freenect_shutdown(Vision.ctx);
	Vision.ctx=NULL;
  }
	
  return 0;
}
