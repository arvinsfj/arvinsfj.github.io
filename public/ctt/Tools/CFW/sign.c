/*
 * DESC: 加工bootloader二进制代码使之成为bios可以加载的格式（512字节，最后2字节为55aa）
 * Author: Arvin
 * Date: 2019年03月21日
 * Usage: ./sign in.bin out.bin
 */

#include <stdio.h>

int main(int argc, char**argv)
{
    if (argc < 3) {
        printf("%s\n", "please enter a input filename and a output filename!");
        return -1;
    }
    FILE* ifile = fopen(argv[1], "rb+");
    if (ifile == NULL) {
        printf("this is no input file named: %s.!\n", argv[1]);
        return -1;
    }
    unsigned char buff[1024];
    int n = fread(buff, 1, 1024, ifile);
    fclose(ifile);
    if (n > 510) {
        printf("boot block too large: %d bytes.\n", n);
        return -1;
    }
    printf("boot block is %d bytes (max 510)\n", n);
    for (int i = n; i < 510; i++) buff[i] = '\0';
    buff[510] = '\x55';
    buff[511] = '\xAA';
    //
    FILE* ofile = fopen(argv[2], "wb+");
    if (ofile == NULL) {
        printf("this is no output file named: %s.!\n", argv[2]);
        return -1;
    }
    fwrite(buff, 1, 512, ofile);
    fclose(ofile);
    //
    return 0;
}
