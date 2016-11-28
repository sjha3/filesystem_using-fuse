ptrmake:
	g++ -Wall ramdisk.cpp `pkg-config fuse --cflags --libs` -o ramdisk
