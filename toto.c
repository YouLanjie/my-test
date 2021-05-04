#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <headfile/kbhit.h>

int main(int argc,char * argv[]) {
	int a = 0x31;
	char name[150]="ffmpeg -i ";
	char b[50]="test";
	FILE * fp;

	system("mkdir data");
	system("mkdir data/input");
	system("mkdir data/output");
	system("touch data/input.txt");
	system("touch data/output.log");
	while(a != 0x30) {
		system("clear");
		printf("Welocome\ninput '1' to star\n");
		a = input();
		if(a == 0x31) {
			system("ls -1 > data/input.txt");
			system("echo -e \"\\033\"");
			fp = fopen("data/input","r");
			while(!strcmp(b,"\033")) {
				fgets(b,fp);
				strcat(name,b);
				strcat(name," ");
				strcat(name,b);
				strcat(name,".");
				strcat(name,*argv[1]);
				system(name);
			}
			fclose(fp);
		}
	}
	return 0;
}
