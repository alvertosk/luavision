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

int run0(uint16_t *frame,track_t *T,runner_t *S,int x, int X, int y, int Y){
	VARRUN();
    for(j=y;j < Y;j+=diff,frame+=fdiff){
			for(i=x;i < X;i+=diff){
				BOUNDSET();
				da=frame[i];
				if(da > Z || da < z){ 
					if(running){
					eol:
						LABEL(i);
					} 
				}else{
					if(running){
						running += da;
					}else{
						running = da;
						run->s = i;
						run->sv = da;
					}
				}
			}
			if(running){ 
				i=i-diff+1; 
				goto eol; 
			}
			JUMP();
	}
    runcount = run - runs;
	while (run-- != runs) {
		DEFCOL();
		if (label->test){
			OLDCOL();
		} else {
			NEWCOL();
		}
		run->s++;
	}
	*(chk) = NULL;
	return runcount;
}

int run1(uint16_t *frame,track_t *T,runner_t *S,int x, int X, int y, int Y){
	VARRUN();
    for(j=y;j < Y;j+=diff,frame+=fdiff){
			for(i=x;i < X;i+=diff){
				BOUNDSET();
				da=frame[i];
				if(da > Z || da < z){ 
					if(running){
					eol:
						run->z = dz;
						run->zv = dzv;
						LABEL(i);
					} 
				}else{
					if(running){
						if(da < dzv){
							dz = i;
							dzv = da;
						}
						running += da;
					}else{
						dzv = da;
						running = da;
						dz=i;
						run->s = i;
						run->sv = da;
					}
				}
			}
			if(running){ 
				i=i-diff+1; 
				goto eol;
			}
			JUMP();
	}
    runcount = run - runs;
	while (run-- != runs) {
		DEFCOL();
		if (label->test){
			if (run->zv < label->z->zv) label->z = run;
			OLDCOL();
		} else {
			label->z = run;
			NEWCOL();
		}
		run->s++;
	}
	*(chk) = NULL;
	return runcount;
}

int run2(uint16_t *frame,track_t *T,runner_t *S,int x, int X, int y, int Y){
	VARRUN();
    for(j=y;j < Y;j+=diff,frame+=fdiff){
			for(i=x;i < X;i+=diff){
				BOUNDSET();
				da=frame[i];
				if(da > Z || da < z){ 
					if(running){
					eol:
						run->Z = dZ;
						run->Zv = dZv;
						LABEL(i);
					} 
				}else{
					if(running){
						if(da > dZv){
							dZ = i;
							dZv = da;
						}
						running += da;
					}else{
						dZv = da;
						running = da;
						dZ=i;
						run->s = i;
						run->sv = da;
					}
				}
			}
			if(running){ 
				i=i-diff+1; 
				goto eol;
			}
			JUMP();
	}
    runcount = run - runs;
	while (run-- != runs) {
		DEFCOL();
		if (label->test){
			if (run->Zv > label->Z->Zv) label->Z = run;
			OLDCOL();
		} else {
			label->Z = run;
			NEWCOL();
		}
		run->s++;
	}
	*(chk) = NULL;
	return runcount;
}

int run3(uint16_t *frame,track_t *T,runner_t *S,int x, int X, int y, int Y){
	VARRUN();
    for(j=y;j < Y;j+=diff,frame+=fdiff){
			for(i=x;i < X;i+=diff){
				BOUNDSET();
				da=frame[i];
				if(da > Z || da < z){ 
					if(running){
					eol:
						run->Z = dZ;
						run->Zv = dZv;
						run->z = dz;
						run->zv = dzv;
						LABEL(i);
					} 
				}else{
					if(running){
						if(da > dZv){
							dZ = i;
							dZv = da;
						}else if(da < dzv){
							dz = i;
							dzv = da;
						}
						running += da;
					}else{
						dZv = da;
						dzv = da;
						running = da;
						dZ=i;
						dz=i;
						run->s = i;
						run->sv = da;
					}
				}
			}
			if(running){
				i=i-diff+1; 
				goto eol;
			}
			JUMP();
	}
    runcount = run - runs;
	while (run-- != runs) {
		DEFCOL();
		if (label->test){
			if (run->Zv > label->Z->Zv) label->Z = run;
			if (run->zv < label->z->zv) label->z = run;
			OLDCOL();
		} else {
			label->Z = run;
			label->z = run;
			NEWCOL();
		}
		run->s++;
	}
	*(chk) = NULL;
	return runcount;
}
