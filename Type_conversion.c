#include "include/include.h"

int main(int argc,char * argv[]) {
	int a = 0x31,error = 0;
	long e = 0;
	char name[450]="ffmpeg -i ";
	char b[200]="test",temp[215];
	FILE * fp;

	system("clear");
	system("mkdir data 2>/dev/null");
	system("mkdir data/input 2>/dev/null");
	system("mkdir data/output 2>/dev/null");
	system("touch data/input.txt");
	if(argc != 2) {
		printf("Erro!!!\n");
		return 0;
	}
	while(a != 0x30) {
		system("clear");
		printf("\033[1;32mWelocome\n\033[33minput '1' to star\n");
		a = input();
		system("clear");
		if(a == 0x31) {
			system("ls data/input/ > data/input.txt");
			fp = fopen("data/input.txt","r");
			if(!fp) {
				perror("\033[1;31m[main]fopen\033[0m");
				return 1;
			}
			fseek(fp,0L,2);
			e = ftell(fp);
			fseek(fp,0L,0);
			while(e != ftell(fp)) {
				strcpy(name,"ffmpeg -i ");
				while (e != ftell(fp)) {
					fgets(b, sizeof(b), fp);
					b[strlen(b) - 1] = '\0';
					strcpy(temp, "data/output/");
					strcat(temp, b);
					strcat(temp, ".");
					strcat(temp, argv[1]);
					if (access(temp, 0) == EOF) {
						break;
					}
					else {
						printf("\033[1;31m[文件已存在，跳过]filename:\033[1;32m%s.%s\033[0m\n",b,argv[1]);
						if (e == ftell(fp)) {
							printf("\033[1;31m没有要转换的文件！\033[0m\n");
							error = 1;
							break;
						}
					}
				}
				if (error == 1) {
					input();
					break;
				}
				strcat(name,"\"data/input/");
				strcat(name,b);
				strcat(name,"\" \"data/output/");
				strcat(name,b);
				strcat(name,".");
				strcat(name,argv[1]);
				strcat(name,"\" >>/dev/null 2>&1");
				error = system(name);
				if (error != 0) {
					printf("\033[1;31mfilename:\033[1;32m%s\033[1;31m\n",b);
					perror("[system]ffmpeg");
					printf("\033[0m");
					return 1;
				}
				printf("\033[1;32m文件\'%s\'转换完成\033[0m\n",b);
			}
			fclose(fp);
			if (error != 1) {
				printf("\033[1;32m所有文件转换完成\033[0m\n");
				input();
			}
			else {
				error = 0;
			}
		}
	}
	return 0;
}
