/*
	Dump PC-9801E/F/M IPL ROM
*/

#include <stdio.h>
#include <stdlib.h>
#include <farstr.h>
#include <dos.h>

void main()
{
	FILE* fp;
	void far *ptr;
	void *buf = malloc(0x8000);
	
	fp = fopen("C0000.ROM", "wb");
	ptr = MK_FP(0xc000, 0);
	far_memcpy(buf, ptr, 0x8000);
	fwrite(buf, 0x8000, 1, fp);
	fclose(fp);
	
	fp = fopen("C8000.ROM", "wb");
	ptr = MK_FP(0xc800, 0);
	far_memcpy(buf, ptr, 0x8000);
	fwrite(buf, 0x8000, 1, fp);
	fclose(fp);
	
	fp = fopen("D0000.ROM", "wb");
	ptr = MK_FP(0xd000, 0);
	far_memcpy(buf, ptr, 0x8000);
	fwrite(buf, 0x8000, 1, fp);
	fclose(fp);
	
	fp = fopen("D8000.ROM", "wb");
	ptr = MK_FP(0xd800, 0);
	far_memcpy(buf, ptr, 0x8000);
	fwrite(buf, 0x8000, 1, fp);
	fclose(fp);
	
	fp = fopen("E8000.ROM", "wb");
	ptr = MK_FP(0xe800, 0);
	far_memcpy(buf, ptr, 0x8000);
	fwrite(buf, 0x8000, 1, fp);
	fclose(fp);
	
	fp = fopen("F0000.ROM", "wb");
	ptr = MK_FP(0xf000, 0);
	far_memcpy(buf, ptr, 0x8000);
	fwrite(buf, 0x8000, 1, fp);
	fclose(fp);
	
	fp = fopen("F8000.ROM", "wb");
	ptr = MK_FP(0xf800, 0);
	far_memcpy(buf, ptr, 0x8000);
	fwrite(buf, 0x8000, 1, fp);
	fclose(fp);
}
