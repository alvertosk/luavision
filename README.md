#vision = require "vision"#

Vision is a lua module for developing nui apps with the MS Kinect. It takes care of buffering the depth frames and making them available to lua states, even on different (native) threads. Lua scripts can access the raw depth data along with blob tracking information. Using a divide and conquer approach, meaningful information can be extracted and serve as stimuli for many kinds of applications.

As long as at least one lua state has required the module, the device will be automatically discovered and initialized. You can plug it in and out at will and it will be reinitialized, use the ir emmiter as an indicator of operation as the led is not currently utilized.  Linux needs setting some udev rules to access the device without root privileges and windows need a set of drivers installed, all of which should be available in the resources that come along with the module. For information on the specifics for each platform check [http://openkinect.org/wiki/Getting_Started](http://openkinect.org/wiki/Getting_Started). Mind that you don't need to build the freenect library, it's already included in the module. Just follow the instructions to install the drivers/udev rules.

To install in linux you need libusb-1.0-0-dev
```
luarocks install https://raw.github.com/LuaVision/lua-vision/master/vision-1.0-1.rockspec
```

For windows you need [libusb-win32](http://libusb-win32.sourceforge.net/)
```
luarocks install https://raw.github.com/LuaVision/lua-vision/master/vision-1.0-1.rockspec LIBUSB_INCDIR=/path/to/libusb/include/folder LIBUSB_LIBDIR=/path/to/libusb/lib/folder
```

###time, device_stamp, frame_count, reset_count =   vision.tap()###
Acquires a new frame, blocking the calling thread until one is available. Calling this, or vision.try() with a non-nil return, is needed before performing any other operation otherwise an error will be thrown. 


Multiple threads can acquire the same frame but subsequent frames will only become available again when the last thread releases the current one held. Thus it would be best to call vision.rel() as soon as you're done with extracting information from the current frame so other threads can progress. 

Tapping will also release the previous frame, if acquired so there's no need for calling rel() if using the device from only a single thread.

The return values are: the time the frame was acquired in seconds passed since the unix epoch as a floating point number, an integer timestamp that comes from the device, the current frame count and the count of times the device was reset due to error or unplugging. Mind that the frames are continuously fetched in the background and you may miss frames if you don't process them fast enough but you'll always get the latest one when you tap. The frame count indicates if frames are getting missed.

###time, device_stamp, frame_count, reset_count =   vision.try()###
Works like tap() but in a non-blocking manner. It will return nils if a new frame isn't available. In any case it will also release the previously acquired frame. It is meant for periodic checking in an event loop.

###vision.rel()###

Explicitly releases the acquired frame.

### lightuserdata | count = vision.raw(_void.view_, _Xmin_, _Xmax_, _Xmin_, _max y_)###

Gets the raw depth values from the current frame.

When called with no arguments it returns a pointer(lightuserdata) to the current frame, a 320*240 array of values in mm in short integer format. You can use this with LuaJit for processing but remember that if you release the frame the values referenced by the pointer may get overwritten by new data.

If a void.view is provided as the first argument, the depth values will be copied to that(refer to the void module for details on using it) and a count of pixels will be returned. 

The optional arguments are the bounding box for copying and default to 1,320,1,240 for the full frame.

### vision.sub(void.view, _labelIndex_)###

Copies a subsampled 40x30 view of the frame into the provided void.view. This is useful for setting the background extraction feature of the blob tracking process but it could also be used with eg. skeltrack. 

If a label index is provided the copy will only include the pixels of the corresponding label of a detected blob which is the order of the blobs as they are placed in the void.view you provide to a runner(starting from 1, see the runner section for details) 
### factor = vision.dev()###

Returns a device specific factor that can be used for converting the x-y properties acquired during the tracking process into real world coordinates. Check the vision.runner section for more detail.

### runs, labels, checked_labels = vision.ptr()###

Returns pointers(lightuserdata) to the internal structures of the tracking process. Consult the module's source code if you desire to use these with LuaJIT for exactring more information than what's provided already.


***
vision.runner
---------

###vision.runner = vision.new(option1,option2,...) ###

Returns a new vision.runner. These objects are used to perform blob tracking on the frames.

Arguments are strings which indicate the kind of information we want to get when we perform scans with this particular runner. For every blob, the tracking engine records 7 points, the edge pixels of the three axes x-y-z, along with the centroid(which is geometrically derived). 

The points tracked are: center,left,right,top,bottom,far & near. 

For every point we can get the x and y in camera pixels(which can be converted to real world coordinates, see vision.dev()) and z which is the orthogonal distance from the camera in mm. 

Each argument is formed like < point > - < property > so "center-z" means the z coordinate of the centroid of the blob, which is also the average depth of all pixels. They will all return short integers in the order defined here when the runner is called(see below)

In addition "pixels" and "runs" will return the pixel count and the number of continuous horizontal lines that compose the blob respectively. The order you pass the arguments in the constructor will be the same you get them returned when performing the tracking passes later. 

The formulas for converting the pixel coordinates to orthographic ones relative to the center of the camera in mm is the following(using the factor from vision.dev() and the z provided from the measurements):

~~~
world_x = (point_x - 160) * factor * z
world_y = (point_y - 120) * factor * z
~~~

###runner.sub _= subsampling_ ###
Sets the runner to do subsampled tracking. This helps tremendously with performance, albeit trading some precision. 0=No subsampling, 1=2x, 2=4x, 3=8x. To understand the difference a level 3 here is ~50x the performance. Mind that the measurements appear normal and quantized to the sub factor.

###runner.far _= millimetres|void.view_ ###
Returns the constant far depth threshold in mm or nil if a bounding map is used. Sets the constant far threshold or copies the values from the provided void.view into the bounding map. Passing a positive number activates a constant value. Passing anything else activates the bounding map.

###runner.near _= millimetres|void.view_ ###
Returns the constant near depth threshold in mm or nil if a bounding map is used. Sets the constant near threshold or copies the values from the provided void.view into the bounding map. Passing a positive number activates a constant value. Passing anything activates the bounding map.

###runner.min _= pixels_ ###
Gets/sets the minimum pixel count a blob must have to be returned.

###runner.max _= pixels_ ###
Gets/sets the maximum pixel count a blob must have to be returned (up to 65535 pixels or 85% of the total frame area).

### #runner (len operator)###
Returns the number of arguments that were provided when creating the runner.

###option = runner[positive index]###
Returns the argument that was set in this position when the runner was created.

###runner[0] _= void.view | millimetres_###
Returns a pointer(lightuserdata) to the depth bounding map of this particular runner. This is a [40x30][2] array of near and far pairs in mm. 

If you provide a positive number every element of the map's far portion will be set to that.

A negative number will set each entry in the near portion of the map to the respective one from the far portion minus the number specified.

If a void.view is provided it will be processed as an array of u16s and each value in the far portion will be set to the respective element from the view if the latter is smaller than the former value, but not zero. 

These operations are useful for doing background extraction. Just grab some frames with vision.sub() and feed them here to set the far bound. Then you can also pass a negative number to adjust the near bound relative to the far for catching objects that are near the background, for a nice touch surface setup.

###count, blobs, runs = runner(_void,view_, _Xmin_,  _Xmax_,  _Ymin_,  _Ymax_)###

Performs the blob tracking operation on the frame and places the results in the provided void.view as packs of short integers in the order that they where declared in the runner's constructor.

If a bounding box is provided the pass will be limited to that. The cost of tracking is roughly O(n) with the area scanned. If no far and/or near properties where declared, the overhead is also lowered by 1/4 to 1/2.

If a nil is passed instead of a view, the tracking will still take place. You can then access the results through the pointers provided from vision.ptr(). Mind that all runners in the same Lua State share the same structures when doing blob tracking so subsequent passes will overwrite the results. 

That's it, have fun :)
