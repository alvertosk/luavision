package = "Vision"
version = "2.0-1"

source = {
   --url= "/home/wizgrav/luavision/lua-vision"
   url= "git://github.com/alvertosk/luavision.git",
   branch= "dev"

}

description = {
   summary = "Computer vision development using depth cameras and Lua.",
   detailed = [[
   Vision is a lua module that provides functionality for using depth data.
   It's main target is the primesense chipset which powers the MS Kinect.
   More devices will be supported as they become available to the market.
   
   The module features:
   Triple buffering in a background thread handling frame delivery.
   Recording and replaying of depth streams to ease development.
   Simultaneous access to the depth frames from multiple threads.
   A fast & configurable blob tracking engine for scene analysis.
   Histograms and depth masked tracking for background separation.
   Access to the raw depth data for further use eg with skeltrack.
   It can use buffers supplied from the void module or LuaJit FFI 
   ]],
   homepage = "http:///www.luavision.com",
   license = "Apache20",
   maintainer="Yannis Gravezas <wizgrav@gmail.com>"
}

dependencies = {
   "lua >= 5.1","luarocks-build-cpp","void"
}

external_dependencies = {
	platforms ={
		windows = {
			LIBUSB = {
				header = "lusb0_usb.h",
				library = "libusb.lib"
			}
		},
		unix = {
			LIBUSB = {
				header = "libusb-1.0/libusb.h",
			}
		}
	}	
}

build = {
	platforms = {
		unix = { 
			type = "builtin",
			modules = {
				vision = {
					 sources = {"src/vision.c", "src/run.c", "src/device.c", "libfreenect/src/core.c", "libfreenect/src/cameras.c", "libfreenect/src/usb_libusb10.c", "libfreenect/src/registration.c", "libfreenect/src/tilt.c"},
					 libraries = {"usb-1.0","pthread"},
					 incdirs = {"libfreenect/include","libfreenect/src","$(LIBUSB_INCDIR)/libusb-1.0/"}
				}
			}
		},
		windows = {
			type = "cpp",
			modules = {
				vision = {
					 sources = {"src/vision.c", "src/run.c", "src/device.c", "libfreenect/src/core.c","libfreenect/src/cameras.c","libfreenect/src/usb_libusb10.c","libfreenect/src/registration.c","libfreenect/src/tilt.c","libfreenect/platform/windows/libusb10emu/libusb-1.0/libusbemu.cpp","libfreenect/platform/windows/libusb10emu/libusb-1.0/failguard.cpp"},
					 libraries = {"libusb","user32"},
					 incdirs = {"libfreenect/include","libfreenect/src","$(LIBUSB_INCDIR)","libfreenect/platform/windows/","libfreenect/platform/windows/libusb10emu/libusb-1.0"},
					 libdirs = {"$(LIBUSB_LIBDIR)"}
				}
			}
		}		
	}
}
