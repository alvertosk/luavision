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

#define LUAVISION_VERSION     "lua-vision 2.0"
#define LUAVISION_COPYRIGHT   "Copyright (C) 2013-2014, Yannis Gravezas"
#define LUAVISION_DESCRIPTION "A Lua module for NUI development using depth cameras"

const char *options[34] = {"label","center-x","center-y","center-z",
	"left-x", "left-y","left-z","west-x","west-z","right-x","right-y","right-z","east-x","east-z","top-x",
	"top-y","top-z","bottom-x","bottom-y","bottom-z","near-x","near-y",
	"near-z","close-x","close-z","far-x","far-y","far-z","away-x","away-z","runs","area","spread",NULL};

static int setlimits(lua_State *L,int len, uint16_t limits[240][2]){
	int lx,ly,sx,sy,ax,ay,i,j,nx,inc,pass,ret,t;
	int l=1;
    
    lx = lua_tointeger(L,3);
    ly = lua_tointeger(L,4);
    lua_argcheck(L,sx>0 && sx<321 && sy>0 && sy<241,3,"invalid path");
    sx=lx;
    sy=ly;
    for(i =5;i<len;i+=2){
        ax = lua_tointeger(L,3);
		ay = lua_tointeger(L,4);
		lua_argcheck(L,ax > 0 && ax < 321 && ay > 0 && ay < 241,3,"invalid path");
	last:
		inc = ay < ly ? -1 : 1; 
        pass = 0;
        memset((void *)limits,0,960);
        for(j=ly;j!=ay+inc;j+=inc){
            t = (ay-ly);
            nx = lx+(j-ly)/t*(ax-lx);
            if(!pass){
				pass=1;
                if(!limits[j][0]){
                    l=1;
                }else if(!limits[j][1]){
                    l=0;
                }
            }
            if(l) limits[j][0]=nx; else limits[j][1]=nx;
            
        }
        if(ret > ay) ret=ay;
        lx=ax;
        ly=ay;
    }
    if(sx!=lx || sy!=ly){
		j-=inc;
		ax=sx;
		ay=sy;
		goto last;
	}
	return ret;
}

static int callrunner(lua_State *L){
	uint16_t *sp, *p, limits[30][2];
	int i,cnt,x,X,y,Y,min,max,n;
	label_t **c,**nc,*l;
	runner_t *ud;
	void_t *vd;
	vis_t *V;
	i = lua_gettop(L);
	if(i != 6 && i != 2) LUA_ERROR("Invalid number of arguments");
	V = (vis_t *)lua_touserdata(L,lua_upvalueindex(2));
	if(!V->current && !V->data) LUA_ERROR("you must tap a frame first");
	ud = (runner_t *) lua_touserdata(L,1);
	vd = (void_t *) lua_touserdata(L,2);
	x = (int) luaL_optinteger(L,3,1);
	X = (int) luaL_optinteger(L,4,320);
	y = (int) luaL_optinteger(L,5,1);
	Y = (int) luaL_optinteger(L,6,240);
	x--;y--;
	if(x < 0 || X > 320 || y < 0 || Y > 240) LUA_ERROR("Invalid bounds");
	sp = V->current ? V->current->data : V->data;
	switch(ud->mode){
		case 0: V->tracker->rcount = run0(sp,V->tracker,ud,x,X,y,Y);break;
		case 1: V->tracker->rcount = run1(sp,V->tracker,ud,x,X,y,Y);break;
		case 2: V->tracker->rcount = run2(sp,V->tracker,ud,x,X,y,Y);break;
		case 3: V->tracker->rcount = run3(sp,V->tracker,ud,x,X,y,Y);break;
	}
	nc = c = V->tracker->checked;
	min = ud->min;
	max = ud->max;
	while((l=*(nc)) != NULL){
		n = l->count << ud->sub;
		if(n > min && n < max){
			*(c++) = l;
		}else{
			l->y=NULL;
		}
		l->test=0;
		nc++;
	}
	V->tracker->lcount = (int)(c - V->tracker->checked);
	
	if(vd){
		cnt = vd->size /(ud->len << 1);
		cnt = MIN(cnt,V->tracker->lcount);
		nc = V->tracker->checked + cnt;
		lua_pushinteger(L,(lua_Integer) (cnt * ud->len));
		lua_pushinteger(L,(lua_Integer) cnt);
		sp = (uint16_t *)vd->data;
		p = sp;
		for(i=0;i!=ud->len;i++,sp++){
			switch(ud->opts[i]){
				case VISION_LABEL : OPTFOR(){ *p=l->x->label; }break;
				case VISION_CENTER_X : OPTFOR(){ l = *c; *p = l->cx/(l->count << 1); }break;
				case VISION_CENTER_Y : OPTFOR(){ l = *c; *p = l->cy/l->count; }break;
				case VISION_CENTER_Z : OPTFOR(){ l = *c; *p = l->cz/l->count; }break;
				case VISION_LEFT_X : OPTFOR(){ l = *c; *p = l->x->s; }break;
				case VISION_LEFT_Y : OPTFOR(){ l = *c; *p = l->x->y; }break;
				case VISION_LEFT_Z : OPTFOR(){ l = *c; *p = l->x->sv; }break;
				case VISION_WEST_X : OPTFOR(){ l = *c; *p = l->xx/l->runs; }break;
				case VISION_WEST_Z : OPTFOR(){ l = *c; *p = l->xz/l->runs; }break;
				case VISION_RIGHT_X : OPTFOR(){ l = *c; *p = l->X->e; }break;
				case VISION_RIGHT_Y : OPTFOR(){ l = *c; *p = l->X->y; }break;
				case VISION_RIGHT_Z : OPTFOR(){ l = *c; *p = l->X->ev; }break;
				case VISION_EAST_X : OPTFOR(){ l = *c; *p = l->Xx/l->runs; }break;
				case VISION_EAST_Z : OPTFOR(){ l = *c; *p = l->Xz/l->runs; }break;
				case VISION_TOP_X : OPTFOR(){ l = *c; *p = (l->y->s + l->y->e) >> 1; }break;
				case VISION_TOP_Y : OPTFOR(){ l = *c; *p = l->y->y; }break;
				case VISION_TOP_Z : OPTFOR(){ l = *c; *p = (l->y->sv + l->y->ev) >> 1;}break;
				case VISION_BOTTOM_X : OPTFOR(){ l = *c; *p = (l->Y->s + l->Y->e) >> 1; }break;
				case VISION_BOTTOM_Y : OPTFOR(){ l = *c; *p = l->Y->y; }break;
				case VISION_BOTTOM_Z : OPTFOR(){ l = *c; *p = (l->Y->sv + l->Y->ev) >> 1; }break;
				case VISION_NEAR_X : OPTFOR(){ l = *c; *p = l->z->z; }break;
				case VISION_NEAR_Y : OPTFOR(){ l = *c; *p = l->z->y; }break;
				case VISION_NEAR_Z : OPTFOR(){ l = *c; *p = l->z->zv; }break;
				case VISION_CLOSE_X : OPTFOR(){ l = *c; *p = l->zx/l->runs; }break;
				case VISION_CLOSE_Z : OPTFOR(){ l = *c; *p = l->zz/l->runs; }break;
				case VISION_FAR_X : OPTFOR(){ l = *c; *p = l->Z->Z; }break;
				case VISION_FAR_Y : OPTFOR(){ l = *c; *p = l->Z->y; }break;
				case VISION_FAR_Z : OPTFOR(){ l = *c; *p = l->Z->Zv; }break;
				case VISION_AWAY_X : OPTFOR(){ l = *c; *p = l->Zx/l->runs; }break;
				case VISION_AWAY_Z : OPTFOR(){ l = *c; *p = l->Zz/l->runs; }break;
				case VISION_RUNS : OPTFOR(){ l = *c; *p = l->runs << ud->sub; }break;
				case VISION_AREA : OPTFOR(){ l = *c; *p = l->count << ud->sub; } break;
				case VISION_SPREAD : OPTFOR(){ l = *c; *p = l->spread/l->runs; } break;
			}
		}
	}else{ 
		lua_pushinteger(L,0);
		lua_pushinteger(L,(lua_Integer)(V->tracker->lcount));
	}
	lua_pushinteger(L,V->tracker->rcount);
	return 3;
}

static int lenrunner(lua_State *L){
	runner_t *ud;
	UDCHECK(ud,1);
	lua_pushinteger(L,ud->len);
	return 1;	
}
	
static int indexrunner(lua_State *L){
	runner_t *ud;
	track_t *T;
	uint32_t index;
	int i;
	UDCHECK(ud,1);
	index  = (uint32_t) lua_tointeger(L,2);
	if(index){
		if(index > ud->len) LUA_ERROR("Invalid index");
		lua_pushstring(L,options[ud->opts[index-1]]);
	}else{
		size_t len;
		const char *str;
		switch(lua_type(L,2)){
			case LUA_TNUMBER:
				lua_pushinteger(L,(lua_Integer) ud->mode);
				return 1;
			case LUA_TSTRING:
				str = lua_tolstring(L,2,&len);
				if(len == 3){
					switch(str[0]){
						case 'k':
							if(!strcmp(str+1,"ey")){
								lua_pushinteger(L,ud->key);
								return 1;
							}
						case 'f': 
							if(!strcmp(str+1,"ar")){
								ud->pfar?lua_pushlightuserdata(L,(void *) ud->pfar):lua_pushinteger(L,ud->far);
								return 1;
							}  
							break;
						case 's': 
							if(!strcmp(str+1,"ub")){
								lua_pushinteger(L,ud->sub);
								return 1;
							}  
							break;
						case 'm': 
							if(!strcmp(str+1,"in")){
								lua_pushinteger(L,(lua_Integer) ud->min);
								return 1;
							}else if(!strcmp(str+1,"ax")){
								lua_pushinteger(L,(lua_Integer) ud->max);
								return 1;
							} 
							break;
					}
				}else if(len==4){
					 if(!strcmp(str,"near")){
						ud->pnear ? lua_pushlightuserdata(L,(void *) ud->pnear) : lua_pushinteger(L,ud->near);
						return 1;
					 }else if(!strcmp(str,"mask")){
						ud->pmask ? lua_pushlightuserdata(L,(void *) ud->pmask) : lua_pushnil(L);
						return 1;
					 }else if(!strcmp(str,"this")){
						lua_pop(L,1);
						lua_rawget(L,lua_upvalueindex(1));
						return 1;
					 }
				}		
			
			}
			lua_rawget(L,lua_upvalueindex(1));
	}
	return 1;
}

static int nindexrunner(lua_State *L){
	runner_t *ud;
	track_t *T;
	void_t *vd;
	void *temp;
	uint32_t index;
	uint16_t i,v,d,*vp;
	int iv;
	UDCHECK(ud,1);
	index  = (uint32_t) lua_tointeger(L,2);
	if(index){
		LUA_ERROR("Cannot set runner options after creating it");
	}else{
		size_t len;
		const char *str;
		switch(lua_type(L,2)){
			case LUA_TNUMBER:
				switch(lua_type(L,3)){
					case LUA_TNUMBER:
						iv=(int16_t)lua_tointeger(L,3);
						if(!iv){
							luaL_argcheck(L,ud->pnear || ud->pfar,1,"needs a far or near mask buffer");
							if(ud->pnear) memset(ud->pnear,0,2400);
							if(ud->pnear) for(i=0;i!=1200;i++){
								ud->pfar[i]=8192;
							}
						}else if(iv>0){
							luaL_argcheck(L,ud->pfar,1,"needs a far mask buffer");
							for(i=0;i!=1200;i++){
								d=ud->pfar[i];
								if(d > iv) ud->pfar[i]=iv;
							}
						}else if(v<0){
							luaL_argcheck(L,ud->pnear && ud->pfar,1,"needs near and far mask buffers");
							for(i=0;i!=1200;i++){
								d=ud->pfar[i] + iv;
								ud->pnear[i]=d>0?d:0;
							}
						}
						return 0;
					case LUA_TUSERDATA:
						luaL_argcheck(L,ud->pnear || ud->pfar,1,"needs a far mask buffer");
						vd = (void_t *)luaL_checkudata(L,3,"void.view");
						if(!vd)luaL_argerror(L,3,"userdata must be a void.view");
						if(!vd->data) luaL_argerror(L,3,"neutered void.view");
						if(vd->size < 2400) luaL_argerror(L,3,"void.view size must be >= 2400 bytes");
						vp=(uint16_t *)vd->data;
						for(i=0;i!=1200;i++){
							d=ud->pfar[i];
							if( !d || (vp[i] && vp[i] < d)) ud->pfar[i]=vp[i];
						}
						return 0;
				}
				return 0;
			case LUA_TSTRING:
				str = lua_tolstring(L,2,&len);
				if(len == 3){
					switch(str[0]){
						case 'f': 
							if(!strcmp(str+1,"ar")){
								i = lua_tointeger(L,3);
								if(!i){
									vd = (void_t *)luaL_checkudata(L,3,"void.view");
									if(!vd){ud->far=0;return 0;};
									if(!vd->data) luaL_argerror(L,3,"neutered void.view");
									if(vd->size < 2400) luaL_argerror(L,3,"void.view size must be >= 2400 bytes");
									if(ud->vfar){
										temp = ud->vfar;
										ud->vfar = vd->blob;
										vd->blob=temp;
										ud->pfar = (uint16_t *)vd->data;
										vd->data=(uint8_t *)ud->pfar;
										vd->size=2400;
									}else{
										ud->vfar = vd->blob;
										ud->pfar = (uint16_t *)vd->data;
										vd->data=NULL;
										vd->blob=NULL;
										vd->size=0;
									}
								}else{
									ud->far = i;
								}
								return 0;
							}  
							break;
						case 'm': 
							if(!strcmp(str+1,"in")){
								i = lua_tointeger(L,3);
								luaL_argcheck(L,i>0 && i < 65536 ,3,"Invalid pixel count");
								ud->min=i;
								return 0;
							}else if(!strcmp(str+1,"ax")){
								i = lua_tointeger(L,3);
								luaL_argcheck(L,i>0 && i < 65536,3,"Invalid pixel count");
								ud->max=i;
								return 0;
							} 
							break;
						case 's': 
							if(!strcmp(str+1,"ub")){
								i = lua_tointeger(L,3);
								luaL_argcheck(L,i>=0 && i < 4 ,3,"invalid subsampling");
								ud->sub=i;
								return 0;
							}
							break;
					}
				}else if(len==4){
					if(!strcmp(str,"near")){
						i = lua_tointeger(L,3);
						if(!i){
							vd = (void_t *)luaL_checkudata(L,3,"void.view");
							if(!vd){ud->near=0;return 0;};
							if(!vd->data) luaL_argerror(L,3,"neutered void.view");
							if(vd->size < 2400) luaL_argerror(L,3,"void.view size must be >= 2400 bytes");
							if(ud->vnear){
								temp = ud->vnear;
								ud->vnear = vd->blob;
								vd->blob=temp;
								ud->pnear = (uint16_t *)vd->data;
								vd->data=(uint8_t *)ud->pnear;
								vd->size=2400;
							}else{
								ud->vnear = vd->blob;
								ud->pnear = (uint16_t *)vd->data;
								vd->data=NULL;
								vd->blob=NULL;
								vd->size=0;
							}
						}else{
							ud->near = i;
						}
						return 0;
					}else if(!strcmp(str,"mask")){
						vd = (void_t *)luaL_checkudata(L,3,"void.view");
						if(!vd){luaL_argerror(L,3,"argument must be a void.view");};
						if(!vd->data) luaL_argerror(L,3,"neutered void.view");
						if(vd->size < 2400) luaL_argerror(L,3,"void.view size must be >= 2400 bytes");
						if(ud->vmask){
							temp = ud->vmask;
							ud->vmask = vd->blob;
							vd->blob=temp;
							ud->pmask = (uint16_t *)vd->data;
							vd->data=(uint8_t *)ud->pmask;
							vd->size=2400;
						}else{
							ud->vmask = vd->blob;
							ud->pmask = (uint16_t *)vd->data;
							vd->data=NULL;
							vd->blob=NULL;
							vd->size=0;
						}
						return 0;
					}else if(!strcmp(str,"this")){
						lua_pushvalue(L,1);
						lua_pushvalue(L,-2);
						lua_rawset(L,lua_upvalueindex(1));
						return 0;
					}
				}		
				
			}
			LUA_ERROR("invalid index");
	}
	return 0;
}


static int gcrunner(lua_State *L){
	int error;
	runner_t *ud;
	UDCHECK(ud,1);
	error=0;
	lua_pushlstring(L,"__gc",4);
	lua_rawget(L,lua_upvalueindex(1));
	if(lua_isfunction(L,-1)){
		lua_pushvalue(L,1);
		if(lua_pcall(L,1,0,0)){
			error=1;
		}
	}
	if(ud->vfar){
		free(ud->vfar);
		ud->vfar=NULL;
		ud->pfar=NULL;
	}
	if(ud->vnear){
		free(ud->vnear);
		ud->vnear=NULL;
		ud->pnear=NULL;
	}
	if(ud->vmask){
		free(ud->vmask);
		ud->vmask=NULL;
		ud->pmask=NULL;
	}
	if(error){
		lua_error(L);
	}
	return 0;
}

static int printrunner(lua_State *L){
	runner_t *ud;
	UDCHECK(ud,1);
	lua_pushfstring(L,"vision.runner: %p",(void *)ud);
	return 1;
}

static int newvis(lua_State *L){
	vis_t *V = (vis_t *)lua_touserdata(L,lua_upvalueindex(1));
	runner_t *ud;
	int i,v;
	int n = lua_gettop(L);
	
	if(!V->tracker) V->tracker = (track_t *) calloc(1,sizeof(track_t));

	if(!n) LUA_ERROR("At least one option is required");
	for(i=0;i++!=n;) luaL_checkoption(L,i,lua_tolstring(L,i,NULL),options);
	ud = (runner_t *) lua_newuserdata(L,sizeof(runner_t));
	luaL_getmetatable(L,"vision.runner");
	lua_setmetatable(L,-2);
	ud->len = n;
	ud->mode = 0;
	ud->sub = 0;
	ud->near =1;
	ud->far = 8192;
	ud->vfar = NULL;
	ud->vnear = NULL;
	ud->pfar = NULL;
	ud->pnear = NULL;
	ud->pmask = NULL;
	ud->vmask = NULL;
	ud->key=0;
	ud->min = 1;
	ud->max = 65535;
	ud->opts = (uint8_t *) malloc(n);
	for(i=0;i!=n;i++){
		v = (uint8_t) luaL_checkoption(L,i+1,lua_tolstring(L,i,NULL),options);
		if(v >= VISION_NEAR_X && v <= VISION_CLOSE_Z) ud->mode |= 1;
		if(v >= VISION_NEAR_X && v <= VISION_CLOSE_Z) ud->mode |= 2;
		ud->opts[i]=v;
	}
	return 1;
}

static int gcvis(lua_State *L){
	vis_t *V = (vis_t *)lua_touserdata(L,1);
	if(V->tracker){
		free(V->tracker);
		V->tracker = NULL;
	}
	if(V->current){
		V->current=NULL;
		pthread_mutex_lock(&Vision.mutex);
		Vision.pending--;
		pthread_mutex_unlock(&Vision.mutex);	
	}
	pthread_mutex_lock(&Vision.Mutex);
	Vision.active--;
	if(!Vision.active){
		thread_stop();
	}
	pthread_mutex_unlock(&Vision.Mutex);
	if(V->blob){
		free(V->blob);
		V->blob=NULL;
		V->data=NULL;
	}
	return 0;
}

static int rawvis(lua_State *L){
	vis_t *V;
	void_t *vd;
	run_t *run;
	label_t **cl;
	int i,j,k,x,X,y,Y,md;
	uint16_t *p16,*data,*vdata,label;
	V = (vis_t *)lua_touserdata(L,lua_upvalueindex(1));
	if(!V->current && !V->data) LUA_ERROR("you must tap a frame first");
	data = (V->current ? V->current->data:V->data);
	if(!lua_gettop(L)){
		lua_pushlightuserdata(L,(void *) data);
		return 1;
	}
	VIEWCHECK(vd,1);
	if(vd->size < 153600) LUA_ERROR("void.view needs to be at least 153600 bytes");
	BOUNDOPT();
	vdata = (V->current ? V->current->data:V->data);
	if(!label){
		data = vdata + 320*y;
		p16 = ((uint16_t *)vd->data) + 320*y;
		for(j=y;j<Y;j++,data+=320,p16+=320){
			for(i=x;i<X;i++){
				p16[i] = data[i];
			}
		}
	}else{
		luaL_argcheck(L,V->tracker && label <= V->tracker->lcount,2,"invalid label");
		cl = V->tracker->checked + label - 1;
		run = (*cl)->y;
		if(md<1){
			for(;run;run=run->next){
				j = run->y-1;
				i = run->s-1;
				data = vdata + 320*j;
				p16 = ((uint16_t *) vd->data) + 320*j;
				for(;i < run->e;i++){
					p16[i] = data[i]+md;
				}
			}
		}else{
			for(;run;run=run->next){
				j = run->y-1;
				i = run->s-1;
				data = vdata + 320*j;
				p16 = ((uint16_t *) vd->data) + 320*j;
				for(;i < run->e;i++){
					p16[i] = md;
				}
			}
		}
	}
	lua_pushvalue(L,1);
	return 1;
}

static int rmsvis(lua_State *L){
	vis_t *V;
	void_t *vd;
	run_t *run;
	label_t *cl;
	int i,j,k,x,X,y,Y,md,poly;
	uint16_t *p16,*data,*vdata,label,limits[240][2];
	V = (vis_t *)lua_touserdata(L,lua_upvalueindex(1));
	if(!V->current && !V->data) LUA_ERROR("you must tap a frame first");
	VIEWCHECK(vd,1);
	if(vd->size < 2400) LUA_ERROR("void.view needs to be at least 2400 bytes");
	poly=0;
	BOUNDOPT();
	vdata = (V->current ? V->current->data:V->data);
	if(!label){
		if(poly) memset((void *)limits,0,sizeof(limits));
		data = vdata + 320*y;
		for(j=y;j<Y;j+=8,data+=2560){
			p16 =((uint16_t *) vd->data) + (j<<2) + j + (x>>3);
			if(poly){
				x=limits[y][0];
				X=limits[y][1];
			}
			for(i=x;i<X;i+=8,p16++){
				*p16 = data[i];
			}
		}
	}else{
		luaL_argcheck(L,V->tracker && label < FRAME_PIXELS/2+1,2,"invalid label");
		cl = V->tracker->labels + label;
		run = cl->y;
		luaL_argcheck(L,run,2,"inactive label");
		if(md<1){
			for(;run;run=run->next){
				j = run->y-1;
				i = run->s-1;
				data = vdata + 320*j;
				p16 = ((uint16_t *) vd->data) +  40*(j>>3) + (i>>3);
				for(;i < run->e;i+=8,p16++){
					*p16 = data[i]+md;
				}
			}
		}else{
			for(;run;run=run->next){
				j = run->y-1;
				i = run->s-1;
				data = vdata + 320*j;
				p16 = ((uint16_t *) vd->data) +  40*(j>>3) + (i>>3);
				for(;i < run->e;i+=8,p16++){
					*p16 = md;
				}
			}
		}
	}
	lua_pushvalue(L,1);
	return 1;
}

static int devvis(lua_State *L){
	vis_t *V = (vis_t *)lua_touserdata(L,lua_upvalueindex(1));
	lua_pushnumber(L,(lua_Number)Vision.factor);
	lua_pushnumber(L,(lua_Number)Vision.reset);
	lua_pushnumber(L,(lua_Number)Vision.zap);
	return 3;
}

static int ptrvis(lua_State *L){
	vis_t *V = (vis_t *)lua_touserdata(L,lua_upvalueindex(1));
	if(!V->tracker) V->tracker = (track_t *) calloc(1,sizeof(track_t));
	lua_pushlightuserdata(L,(void *)V->tracker->runs);
	lua_pushlightuserdata(L,(void *)V->tracker->labels);
	return 2;
}


static int tapvis(lua_State *L){
	frame_t *frame;
	void_t *vd;
	void *temp;
	uint32_t itmp;
	vis_t *V = (vis_t *)lua_touserdata(L,lua_upvalueindex(1));
	if(lua_gettop(L)){
		VIEWCHECK(vd,1);
		luaL_argcheck(L,vd->size >= 320*240*2,1,"invalid frame size");
		if(V->current){
			pthread_mutex_lock(&Vision.mutex);
			Vision.pending--;
			V->current=NULL;
			pthread_mutex_unlock(&Vision.mutex);
		}
		if(V->data){
			temp=(void *)V->data;
			V->data=(uint16_t *)vd->data;
			vd->size = 320*240*2;
			vd->data=temp;
			temp = V->blob;
			V->blob = vd->blob;
			vd->blob = temp;
		}else{
			V->data=(uint16_t *)vd->data;
			vd->size = 0;
			vd->data=NULL;
			V->blob = vd->blob;
			vd->blob = NULL;
		
		}
		MSLEEP();
	}else{
		pthread_mutex_lock(&Vision.mutex);
		if(V->current) Vision.pending--;
		while(1){ 
			if(Vision.wait && !Vision.pending){
				Vision.wait--;
				frame = Vision.current;
				Vision.current = Vision.spare;
				Vision.spare = frame;
				pthread_cond_broadcast(&Vision.condition);
				break;
			}else if(Vision.current && Vision.current->count != V->count){
				break;
			}
			pthread_cond_wait(&Vision.condition, &Vision.mutex);
		}
		Vision.pending++;
		V->current = Vision.current;
		pthread_mutex_unlock(&Vision.mutex);
	}
	if(V->current){ 
		V->count = V->current->count;
		lua_pushnumber(L,(lua_Number)V->current->realtime);
		lua_pushnumber(L,(lua_Number)V->current->stamp);
		lua_pushnumber(L,(lua_Number)V->current->factor);
		return 3;
	}else if(V->data){
		lua_pushboolean(L,0);
		return 1;
	}else{
		return 0;
	}
}

static int tipvis(lua_State *L){
	frame_t *frame;
	void_t *vd;
	void *temp;
	vis_t *V = (vis_t *)lua_touserdata(L,lua_upvalueindex(1));
	if(lua_gettop(L)){
		VIEWCHECK(vd,1);
		luaL_argcheck(L,vd->size >= 320*240*2,1,"invalid frame size");
		if(V->current){
			pthread_mutex_lock(&Vision.mutex);
			Vision.pending--;
			V->current=NULL;
			pthread_mutex_unlock(&Vision.mutex);
		}
		if(V->data){
			temp=(void *)V->data;
			V->data=(uint16_t *)vd->data;
			vd->size = 320*240*2;
			vd->data=temp;
			temp = V->blob;
			V->blob = vd->blob;
			vd->blob = temp;
		}else{
			V->data=(uint16_t *)vd->data;
			vd->size = 0;
			vd->data=NULL;
			V->blob = vd->blob;
			vd->blob = NULL;
		}
	}else{
		pthread_mutex_lock(&Vision.mutex);
		if(V->current) Vision.pending--;
		if(Vision.wait && !Vision.pending){
			frame = Vision.current;
			Vision.current = Vision.spare;
			V->current = Vision.current;
			Vision.spare = frame;
			Vision.pending++;
			Vision.wait--;
			pthread_cond_broadcast(&Vision.condition);
		}else if(Vision.current && Vision.current->count != V->count){
			Vision.pending++;
			V->current = Vision.current;
		}else if(V->current){
			V->current=NULL;
		}
		pthread_mutex_unlock(&Vision.mutex);
	}
	if(V->current){ 
		V->count = V->current->count;
		lua_pushnumber(L,(lua_Number)V->current->realtime);
		lua_pushnumber(L,(lua_Number)V->current->stamp);
		lua_pushnumber(L,(lua_Number)V->current->factor);
		return 3;
	}else if(V->data){
		lua_pushboolean(L,0);
		return 1;
	}else{
		return 0;
	}
}

static int zapvis(lua_State *L){
	int i;
	frame_t *frame;
	vis_t *V = (vis_t *)lua_touserdata(L,lua_upvalueindex(1));
	pthread_mutex_lock(&Vision.mutex);
	if(Vision.spare){
		do{ Vision.count++; }while(!Vision.count);
		Vision.spare->count = Vision.count;
		Vision.zap++;
		pthread_cond_broadcast(&Vision.condition);
	}
	pthread_mutex_unlock(&Vision.mutex);
	return 0;
}

static int relvis(lua_State *L){
	void *temp;
	void_t *vd;
	vis_t *V = (vis_t *)lua_touserdata(L,lua_upvalueindex(1));
	if(lua_gettop(L) && V->data){
		vd = (void_t *) luaL_checkudata(L,1,"void.view");
		if(vd->data)
			luaL_argcheck(L,vd->size >= 320*240*2,1,"invalid frame size");
		temp=(void *)V->data;
		V->data=(uint16_t *)vd->data;
		vd->size = temp ? 320*240*2:0;
		vd->data=temp;
		temp = V->blob;
		V->blob = vd->blob;
		vd->blob = temp;
	}
	if(V->current){
		pthread_mutex_lock(&Vision.mutex);
		Vision.pending--;
		if(!Vision.pending) pthread_cond_broadcast(&Vision.condition);
		pthread_mutex_unlock(&Vision.mutex);
		V->current = NULL;
		lua_pushboolean(L,1);
	}else{
		lua_pushboolean(L,0);
	}
	return 1;
}

int LUA_API luaopen_vision (lua_State *L) {
	
	vis_t *V;
	pthread_mutex_lock(&Vision.Mutex);
	Vision.active++;
	if(!Vision.thread){ thread_start();}
	pthread_mutex_unlock(&Vision.Mutex);
	V = (vis_t *) lua_newuserdata(L,sizeof(vis_t));
	
	V->tracker=NULL;
	V->count=0;
	V->current = NULL;
	V->data = NULL;
	V->blob = NULL;
	lua_createtable(L,0,0);
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L,&gcvis);
	lua_settable(L, -3);
	lua_setmetatable(L,-2);
	
	lua_createtable(L,0,0);
	luaL_newmetatable(L,"vision.runner");
	lua_pushstring(L, "__metatable");
	lua_pushvalue(L,-3);
	lua_settable(L, -3);
	DCLOSURE("__gc",gcrunner,-3);
	DCLOSURE("__len",lenrunner,-3);
	DCLOSURE("__index",indexrunner,-3);
	DCLOSURE("__newindex",nindexrunner,-3);
	DCLOSURE("__tostring",printrunner,-3);
	DCLOSURE("__call",callrunner,-3);
	lua_pop(L,1);
	
	lua_newtable(L);
	VCLOSURE("new",newvis,-3);
	VCLOSURE("tap",tapvis,-3);
	VCLOSURE("tip",tipvis,-3);
	VCLOSURE("rel",relvis,-3);
	VCLOSURE("zap",zapvis,-3);
	VCLOSURE("dev",devvis,-3);
	VCLOSURE("ptr",ptrvis,-3);
	VCLOSURE("raw",rawvis,-3);
	VCLOSURE("rms",rmsvis,-3);
	
	lua_pushliteral(L, LUAVISION_VERSION);
    lua_setfield(L, -2, "_VERSION");
    lua_pushliteral(L, LUAVISION_COPYRIGHT);
    lua_setfield(L, -2, "_COPYRIGHT");
    lua_pushliteral(L, LUAVISION_DESCRIPTION);
    lua_setfield(L, -2, "_DESCRIPTION");
	
	return 1;
}
