#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "..\..\src\vm\disk.h"

void main(int argc, char *argv[])
{
	if(argc < 2) {
		printf("any2d88 : convert teledisk/imagedisk/cpdread/solid image file to d88 format\n");
		printf("\n");
		printf("usage : any2d88 file [output.d88]\n");
	} else if(check_file_extension(argv[1], _T(".d88")) || check_file_extension(argv[1], _T(".d77"))) {
		printf("the source file is d88 format image file\n");
	} else {
		DISK *disk = new DISK();
		disk->open(argv[1], 0);
		if(!disk->inserted) {
			printf("the source file cannot be converted\n");
		} else {
			if(argc < 3) {
				char output[_MAX_PATH];
				sprintf(output, "%s.d88", argv[1]);
				disk->save_as_d88(output);
			} else {
				disk->save_as_d88(argv[2]);
			}
		}
		delete disk;
	}
}
