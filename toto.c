#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "headfile/kbhit_input.h"

int main(int argc,char * argv[]) {
	int a = 0x31;
	long e = 0;
	char name[450]="ffmpeg -i ";
	char b[200]="test";
	FILE * fp;

	system("clear");
	system("mkdir data 2>/dev/null");
	system("mkdir data/input 2>/dev/null");
	system("mkdir data/output 2>/dev/null");
	system("touch data/input.txt");
	system("touch data/output.log");
	if(argc != 2) {
		printf("Erro!!!\n");
		return 0;
	}
	while(a != 0x30) {
		system("clear");
		printf("Welocome\ninput '1' to star\n");
		a = input();
		if(a == 0x31) {
			system("dir ./data/input/ > ./data/input.txt");
			fp = fopen("./data/input.txt","r");
			if(!fp) {
				perror("\033[1;31m[main]fopen\033[0m");
				return 0;
			}
			fseek(fp,0L,2);
			e = ftell(fp);
			fseek(fp,0,0);
			while(e != ftell(fp)) {
				strcpy(name,"ffmpeg -i ");
				fgets(b, sizeof(b), fp);
				strcat(name,"./data/input/");
				strcat(name,b);
				strcat(name," ./data/output/");
				strcat(name,b);
				strcat(name,".");
				strcat(name,argv[1]);
				strcat(name," >>./data/output.log 2>&1");
				system(name);
				system("echo #Done--------------------------------------------------------------------------------------- >>data/output.log");
				printf("文件\'%s\'转换完成",b);
			}
			fclose(fp);
		}
	}
	return 0;
}
