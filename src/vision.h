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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#if defined _WIN32
#include <windows.h>
#undef near
#undef far
typedef CRITICAL_SECTION pthread_mutex_t;
#define PTHREAD_MUTEX_INITIALIZER {(void *)-1,-1,0,0,0,0}
#define pthread_mutex_init(m,a) InitializeCriticalSection(m)
#define pthread_mutex_destroy(m) DeleteCriticalSection(m)
#define pthread_mutex_lock(m) EnterCriticalSection(m)
#define pthread_mutex_unlock(m) LeaveCriticalSection(m)
typedef CONDITION_VARIABLE pthread_cond_t;
#define PTHREAD_COND_INITIALIZER {0}
#define pthread_cond_init(c,a) InitializeConditionVariable(c)
#define pthread_cond_destroy(c) (void)c
#define pthread_cond_wait(c,m) SleepConditionVariableCS(c,m,INFINITE)
#define pthread_cond_broadcast(c) WakeAllConditionVariable(c)
#define thread_t HANDLE
#define thread_start() Vision.thread = CreateThread(NULL,65536,(LPTHREAD_START_ROUTINE)&thread_func,NULL,0,NULL)
#define thread_stop() WaitForSingleObject(Vision.thread, INFINITE); Vision.thread=NULL
#define SLEEP() Sleep(1000)
#define MSLEEP() Sleep(30)
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};

static int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag = 0;

  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);

    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;

    tmpres /= 10;  /*convert into microseconds*/
    /*converting file time to unix epoch*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }

  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }

  return 0;
}

#else

#include <pthread.h>
#define thread_t pthread_t
#define thread_start()\
	size_t stacksize=65536;\
	pthread_attr_t attr;\
	pthread_attr_init(&attr);\
    pthread_attr_setstacksize (&attr, stacksize);\
    pthread_create(&Vision.thread, &attr, &thread_func, NULL);\
	pthread_attr_destroy(&attr)
#define thread_stop() pthread_join(Vision.thread,NULL); Vision.thread=0
#define SLEEP() sleep(1)
#define MSLEEP() usleep(30000)

#endif

#ifdef __cplusplus
extern "C" {
#endif
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
#ifdef __cplusplus
}
#endif

#include <libfreenect.h>
#include <libfreenect-registration.h>

#define LUA_ERROR(error) luaL_error(L,error)
#define VCLOSURE(name,func,index)\
	lua_pushstring(L, name);\
	lua_pushvalue(L,-4);\
	lua_pushcclosure(L,&func,1);\
	lua_settable(L, index)
#define DCLOSURE(name,func,index)\
	lua_pushstring(L, name);\
	lua_pushvalue(L,-3);\
	lua_pushvalue(L,-5);\
	lua_pushcclosure(L,&func,2);\
	lua_settable(L, index)
#define FRAME_PIXELS 76800
#define FRAME_UNITS 38401
#define SUB_PIXELS 1200
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define UDCHECK(ud,index) ud = (runner_t *) lua_touserdata(L,index)
#define VIEWCHECK(ud,index) \
	ud = (void_t *) luaL_checkudata(L,index,"void.view");\
	if(!ud->data) LUA_ERROR("Neutered void.view")
#define LABEL(id)\
	run->y = j+1;\
	run->sum = running;\
	run->e = id;\
	run->ev = frame[id-diff];\
	if(!rt)	rt=run;\
	if(rs){\
		for(prun = rs;prun != re;prun++){\
			if(prun->e < run->s) continue;\
			else if(prun->s <= id){\
				if(!lbl){\
					lbl = prun->label;\
					rs=prun;\
				}else if(prun->label < lbl) lbl=prun->label;\
				er=prun;\
			}else{ break;}\
		}\
		if(lbl){\
			lbl=replabel[lbl];\
			do{ replabel[rs->label]  = lbl; }while(rs++ != er);\
			run->label = lbl;\
			rs--;\
			lbl=0;\
		}else{\
			rs=prun;\
			run->label = l;\
			replabel[l] = l;\
			l++;\
		}\
	}else{\
		run->label = l;\
		replabel[l] = l;\
		l++;\
	}\
	running=0;\
	run++
#define JUMP()\
	switch(m){\
		case 0: break;\
		case 1: \
			bnear = pnear + 40*(y>>3);\
			break;\
		case 2:\
			bfar = pfar + 40*(y>>3);\
			break;\
		case 3:\
			n = 40*(y>>3);\
			bnear = pnear + n;\
			bfar = pfar + n;\
			break;\
		case 4: \
			bmask = pmask + 40*(y>>3);\
			break;\
		case 5: \
			n = 40*(y>>3);\
			bnear = pnear + n;\
			bmask = pmask + n;\
			break;\
		case 6:\
			n = 40*(y>>3);\
			bfar = pfar + n;\
			bmask = pmask + n;\
			break;\
		case 7:\
			n = 40*(y>>3);\
			bnear = pnear + n;\
			bfar = pfar + n;\
			bmask = pmask + n;\
			break;\
	}\
	rs = rt;\
	re = run;\
	rt = NULL
#define DEFCOL()\
	len = run->e-run->s;\
	run->label = replabel[run->label];\
	label = &labels[run->label]
#define OLDCOL()\
	label->runs++;\
	label->count += len;\
	label->cz += run->sum;\
	label->cx += ((len*(run->s+run->e)));\
	label->cy += len*run->y;\
	if(run->s < label->x->s) label->x = run;\
	if(run->e > label->X->e) label->X = run;\
	run->next = label->y;\
	label->xx += run->s;\
	label->xz += run->sv;\
	label->Xx += run->e;\
	label->Xz += run->ev;\
	label->spread += run->y;\
	label->y = run
#define NEWCOL()\
	 label->count = len;\
	 label->cz = run->sum;\
	 label->cx = ((len*(run->s+run->e)));\
	 label->cy = len*run->y;\
	 label->xx = run->s;\
	 label->xz = run->sv;\
	 label->Xx = run->e;\
	 label->Xz = run->ev;\
	 label->x = run;\
	 label->X = run;\
	 label->y = run;\
	 label->Y = run;\
	 label->runs = 1;\
	 label->test = 1;\
	 run->next=NULL;\
	 label->spread = run->y;\
	 *(chk++)=label
#define VARRUN()\
	  int l = 1;\
	  int lbl = 0;\
	  int running = 0;\
	  int da,len,runcount;\
	  int dz = 0;\
	  int dZ = 0;\
	  int dzv = 0;\
	  int dZv = 0;\
	  run_t *runs = T->runs;\
	  run_t *run = runs;\
	  run_t *prun,*sr,*er;\
	  label_t *label;\
	  label_t *labels=T->labels;\
	  label_t **chk = T->checked;\
	  run_t *rs = NULL;\
	  run_t *rt = NULL;\
	  run_t *re = NULL;\
	  uint16_t k = S->key;\
	  uint16_t z = S->near;\
	  uint16_t Z = S->far;\
	  uint16_t *pmask = S->pmask;\
	  uint16_t *pnear = S->pnear;\
	  uint16_t *pfar = S->pfar;\
	  uint16_t *replabel = T->replabel;\
	  uint16_t *bnear,*bfar,*bmask;\
	  int diff,fdiff,i,j,n;\
	  int m = 0;\
	  if(!z && pnear) m |= 1;\
	  if(!Z && pfar) m |= 2;\
	  if(k && pmask) m |= 4;\
	  if(m) x -= x & 7;\
	  diff =1 << S->sub;\
	  fdiff = 320 << S->sub;\
	  frame += (320*y)
#define BOUNDSET()\
	switch(m){\
		case 0: break;\
		case 1: \
			if(!(i & 7)){ n=i>>3; z = bnear[n] & 8191;if(!z) {if(running){ goto eol;} else{ i+=8-diff;continue;}}}\
			break;\
		case 2:\
			if(!(i & 7)){ n=i>>3; Z = bfar[n] & 8191; if(!Z) {if(running){ goto eol;} else{ i+=8-diff;continue;}}}\
			break;\
		case 3:\
			if(!(i & 7)){\
				n=i>>3;\
				z = bnear[n] & 8191;\
				Z = bfar[n] & 8191;\
				if(!(Z || z)) {if(running){ i++; goto eol;} else{ i+=8-diff;continue;}}\
			}\
			break;\
		case 4:\
			break;\
		case 5:\
			if(!(i & 7)){ n=i>>3; if(bmask[n] == k) z=bnear[n]; else { if(running){ goto eol;} else{ i+=8-diff;continue;}}}\
			break;\
		case 6:\
			if(!(i & 7)){ n=i>>3; if(bmask[n] == k) Z=bfar[n]; else { if(running){ goto eol;} else{ i+=8-diff;continue;}}}\
			break;\
		case 7:\
			if(!(i & 7)){ n=i>>3; if(bmask[n] == k){ Z=bfar[n]; z=bnear[n]; }else { if(running){ goto eol;} else{ i+=8-diff;continue;}}}\
			break;\
	}
#define BOUNDOPT()\
	i=lua_gettop(L);\
	x=0;\
	X=320;\
	y=0;\
	Y=240;\
	if(i==5){\
		label=0;\
		x = (int) luaL_optinteger(L,2,1);\
		X = (int) luaL_optinteger(L,3,320);\
		y = (int) luaL_optinteger(L,4,1);\
		Y = (int) luaL_optinteger(L,5,240);\
		md = 0;\
		x--;y--;\
		if(x < 0 || X > 320 || y < 0 || Y > 240 || md < -8191 || md > 8191) LUA_ERROR("Invalid bounds");\
	}else if(i>0 && i<4){\
		label = luaL_optinteger(L,2,0);\
		md=luaL_optinteger(L,3,0);\
	}else LUA_ERROR("invalid number of arguments")
#define OPTFOR() for(p=sp,c=V->tracker->checked;c != nc;c++,p+=ud->len)

extern const char *options[34];

enum {
	VISION_LABEL=0,
	VISION_CENTER_X,
	VISION_CENTER_Y,
	VISION_CENTER_Z,
	VISION_LEFT_X,
	VISION_LEFT_Y,
	VISION_LEFT_Z,
	VISION_WEST_X,
	VISION_WEST_Z,
	VISION_RIGHT_X,
	VISION_RIGHT_Y,
	VISION_RIGHT_Z,
	VISION_EAST_X,
	VISION_EAST_Z,
	VISION_TOP_X,
	VISION_TOP_Y,
	VISION_TOP_Z,
	VISION_BOTTOM_X,
	VISION_BOTTOM_Y,
	VISION_BOTTOM_Z,
	VISION_NEAR_X,
	VISION_NEAR_Y,
	VISION_NEAR_Z,
	VISION_CLOSE_X,
	VISION_CLOSE_Z,
	VISION_FAR_X,
	VISION_FAR_Y,
	VISION_FAR_Z,
	VISION_AWAY_X,
	VISION_AWAY_Z,
	VISION_RUNS,
	VISION_AREA,
	VISION_SPREAD
};


typedef struct runner_t {
	uint16_t near,far,min,max,key,*pnear,*pfar,*pmask;
	void *vnear,*vfar,*vmask;
	int sub,len,mode;
	uint8_t *opts;
} runner_t;

typedef struct frame_t frame_t;
typedef struct frame_t {
	double factor;
	double realtime;
	uint32_t stamp,count;
	uint16_t data[FRAME_PIXELS];
} frame_t;

typedef struct run_t run_t;
typedef struct run_t {
	uint16_t s,sv,e,ev,z,zv,Z,Zv,y,label;
	int32_t sum;
	run_t *next;
} run_t;

typedef struct label_t {
	run_t *x,*X,*y,*Y,*z,*Z;
	int32_t cx,cy,cz,xx,xz,Xx,Xz,zx,zz,Zx,Zz,count,spread;
	uint16_t runs,test;
} label_t;

typedef struct track_t {
	int rcount,lcount;
	run_t runs[FRAME_UNITS];
	label_t labels[FRAME_UNITS];
	label_t *checked[FRAME_UNITS];
	uint16_t replabel[FRAME_UNITS];
} track_t;

typedef struct vis_t {
	uint32_t count;
	frame_t *current;
	track_t *tracker;
	uint16_t *data;
	void *blob;
} vis_t;

typedef struct void_t {
    int32_t type;
    int32_t size;
    int8_t *data;
    void *blob;
} void_t;


typedef struct Vision_t {
	frame_t frames[3],*current,*working,*spare;
	uint32_t wait,pending,active,reset,zap;
	pthread_mutex_t mutex,Mutex;
	pthread_cond_t condition;
	uint16_t depthtomm[2048];
	freenect_context *ctx;
	freenect_device *dev;
	double factor,delta;
	FILE *rfile;
	uint32_t count;
	thread_t thread;
} Vision_t;
extern Vision_t Vision;

int run0(uint16_t *frame,track_t *T,runner_t *S,int x, int X, int y, int Y);
int run1(uint16_t *frame,track_t *T,runner_t *S,int x, int X, int y, int Y);
int run2(uint16_t *frame,track_t *T,runner_t *S,int x, int X, int y, int Y);
int run3(uint16_t *frame,track_t *T,runner_t *S,int x, int X, int y, int Y);

void *thread_func(void *arg);

void replay(void);

void checkrec(void);

int loadrec(const char *file);

int saverec(const char *file);

