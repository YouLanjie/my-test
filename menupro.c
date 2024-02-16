#include "include/tools.h"

int menupro(char *title, char *text[], int tl);

int main() {
	char *text[] = {
		"1.hello first",
		"2.hello two",
		"3.eleven",
		"4.test",
		"5.hahaha",
		"6.ok ok...",
		"7.next page"
	};
	system("clear");
	printf("chose %d\n", menupro("test", text, 7));
	return 0;
}

int menupro(char *title, char *text[], int tl) {
	struct ctools ctools = ctools_init();
#ifdef __linux
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	int winSizeCol = size.ws_col;
#else
	int winSizeCol = 56;
#endif
	int input = 1, currentPage = 1, count = 1, allPages = (tl - 1) / 6 + 1;

	while (input != 0x30 && input != 0x1B) {
		system("clear");
#ifdef __linux
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		winSizeCol = size.ws_col;
#endif

#ifdef __linux
		printf("\033[5;%dH\033[34m--------------------------------------------------------\033[34m", winSizeCol / 2 - 27);
		for (int i = 0; i < 7; i++) {
			printf("\033[%d;%dH", i + 6, winSizeCol / 2 - 27);
			printf("|\033[54C|");
		}
		printf("\033[13;%dH--------------------------------------------------------\033[0m", winSizeCol / 2 - 27);
		printf("\033[2;32m\033[6;%dH↑\033[10;%dH↓\033[11;%dH\033[2;32m%d/%d\033[1;33m", winSizeCol / 2 - 1, winSizeCol / 2 - 1, winSizeCol / 2 + 25, currentPage,allPages);
		printf("\033[2;%dH\033[1;32m%s\033[0m", winSizeCol / 2 - (int)strlen(title) / 2, title);
#else
		Gotoxy_TER(6, winSizeCol / 2 - 1); printf("↑"); Gotoxy_TER(10, winSizeCol / 2 - 1); printf("↓"); Gotoxy_TER(11, winSizeCol / 2 + 24); printf("%d/%d", currentPage,allPages);
		Gotoxy_TER(2, winSizeCol / 2 - (int)strlen(title) / 2); printf("%s", title);
		Gotoxy_TER(5, winSizeCol / 2 - 27); printf("--------------------------------------------------------");
		for (int i = 0; i < 7; i++) {
			Gotoxy_TER(i + 6, winSizeCol / 2 - 27);
			printf("|");
			Gotoxy_TER(i + 6, winSizeCol / 2 + 27);
			printf("|");
		}
		Gotoxy_TER(13, winSizeCol / 2 - 27); printf("--------------------------------------------------------");
#endif
		for (int i = 1; i <= tl - 6 * (currentPage - 1) && i <= 6; i++) {
			if (i != count) {
#ifdef __linux
				printf("\033[%d;%dH%s", (i + 1) / 2 + 6, winSizeCol / 2 - 20 + ((i + 1) % 2) * 32, text[i - 1 +  6 * (currentPage - 1)]);
#else
				Gotoxy_TER((i + 1) / 2 + 6, winSizeCol / 2 - 20 + ((i + 1) % 2) * 32);
				printf("%s", text[i - 1 +  6 * (currentPage - 1)]);
#endif
			}
			else {
#ifdef __linux
				printf("\033[1;7m\033[%d;%dH%s\033[0m", (i + 1) / 2 + 6, winSizeCol / 2 - 20 + ((i + 1) % 2) * 32, text[i - 1 +  6 * (currentPage - 1)]);
#else
				Gotoxy_TER((i + 1) / 2 + 6, winSizeCol / 2 - 20 + ((i + 1) % 2) * 32 - 2);
				printf("> %s", text[i - 1 +  6 * (currentPage - 1)]);
#endif
			}
			ctools.kbhitGetchar();
		}
		input = ctools.getcha();
		if (input == 0x1B) {
			if (ctools.kbhit()) {
				getchar();
				input = getchar();
				if (input == 'A') {
					if (count > 2) {
						count -= 2;
					}
					else if (currentPage > 1){
						count += 4;
						currentPage--;
					}
				}
				else if (input == 'B') {
					if (count < 5 && count + 6 * (currentPage - 1) <= tl - 2) {
						count += 2;
					}
					else if (currentPage < allPages){
						count -= 4;
						currentPage++;
						if (count + 6 * (currentPage - 1) > tl) {
							count -= 1;
						}
					}
				}
				else if (input == 'C') {
					if (count < 6 && count + 6 * (currentPage - 1) < tl) {
						count++;
					}
					else if (currentPage < allPages){
						count = 1;
						currentPage++;
					}
				}
				else if (input == 'D') {
					if (count > 1) {
						count--;
					}
					else if (currentPage > 1){
						count = 6;
						currentPage--;
					}
				}
			}
			else {
				system("clear");
				return 0;
			}
		}
		else if (input == 'w' || input == 'W') {
			if (count > 2) {
				count -= 2;
			}
			else if (currentPage > 1){
				count += 4;
				currentPage--;
			}
		}
		else if (input == 's' || input == 'S') {
			if (count < 5 && count + 6 * (currentPage - 1) <= tl - 2) {
				count += 2;
			}
			else if (currentPage < allPages){
				count -= 4;
				currentPage++;
				if (count + 6 * (currentPage - 1) > tl) {
					count -= 1;
				}
			}
		}
		else if (input == 'd' || input == 'D') {
			if (count < 6 && count + 6 * (currentPage - 1) < tl) {
				count++;
			}
			else if (currentPage < allPages){
				count = 1;
				currentPage++;
			}
		}
		else if (input == 'a' || input == 'A') {
			if (count > 1) {
				count--;
			}
			else if (currentPage > 1){
				count = 6;
				currentPage--;
			}
		}
		else if (input == 'q' || input == 'Q') {
			system("clear");
			return 0;
		}
		else {
			system("clear");
			return count + 6 * (currentPage - 1);
		}
	}
	return 0;
}

