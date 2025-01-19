/*
 *   Copyright (C) 2023 YouLanjie
 *   
 *   文件名称：arch-fast-install.c
 *   创 建 者：youlanjie
 *   创建日期：2023年08月11日
 *   描    述：合并后的程序
 *
 */


#include "include/tools.h"

struct Package {
	const int type;
	const char *name;
	const char *desrcibe;
	int select;
	int installed;
};
struct Command {
	/* 标记，是否执行过 */
	int mark;
	/* 类型: 0:错误重试 1:出错不重试 */
	int type;
	const char *cmd;
};

/* 选择包 */
int package_select();
/* 通用功能 */
int cmd_query(struct Package *p);
int cmd_pacman();
int cmd_yay();
int cmd_update();
/* 安装后 */
void menu_installed();
/* 选择命令 */
void command_select();
/* 运行命令 */
int command_run();

struct Package package_list[] = {
	/* 0 系统 */
	{ 0, "base", "基本系统", -1, 0},
	{ 0, "efibootmgr", "", 1, 0},
	{ 0, "grub", "分区引导", 1, 0},
	{ 0, "linux", "linux系统内核", -1, 0},
	{ 0, "linux-firmware", "Linux的固件文件", -1, 0},
	{ 0, "linux-headers", "Headers and scripts for building modules for the Linux kernel", 1, 0},
	{ 0, "os-prober", "", 1, 0},
	/* 1 开发 */
	{ 1, "adb", "", 0, 0},
	{ 1, "base-devel", "基本构建软件包工具", -1, 0},
	{ 1, "cgdb", "", 0, 0},
	{ 1, "clang", "", 1, 0},
	{ 1, "cmake", "", 0, 0},
	{ 1, "ctags", "", 1, 0},
	{ 1, "gcc", "", 0, 0},
	{ 1, "gdb", "", 1, 0},
	{ 1, "git", "", 0, 0},
	{ 1, "mingw-w64-gcc", "跨平台编译，然并卵", 0, 0},
	/* 2 CLI工具 */
	{ 2, "arch-install-scripts", "", 0, 0},
	{ 2, "archinstall", "", 0, 0},
	{ 2, "archiso", "", 0, 0},
	{ 2, "at", "", 1, 0},
	{ 2, "bat", "高级的文本查看", 0, 0},
	{ 2, "bc", "计算器", 0, 0},
	{ 2, "duf", "硬盘大小(df升级版)", 0, 0},
	{ 2, "emacs", "", 0, 0},
	{ 2, "eva", "计算器", 0, 0},
	{ 2, "figlet", "ASCII字符画", 0, 0},
	{ 2, "fish", "", 1, 0},
	{ 2, "fzf", "模糊查找软件", 1, 0},
	{ 2, "gdu", "硬盘占用分析程序", 0, 0},
	{ 2, "gnupg", "", 1, 0},
	{ 2, "hexedit", "16进制编辑器", 0, 0},
	{ 2, "highlight", "文本查看高亮", 0, 0},
	{ 2, "htop", "高级的top程序", 0, 0},
	{ 2, "indent", "C语言程序格式化工具", 0, 0},
	{ 2, "lolcat", "将输出变为彩色，需要rust", 0, 0},
	{ 2, "man", "", 1, 0},
	{ 2, "man-pages-zh_cn", "man的中文支持", 1, 0},
	{ 2, "mdcat", "终端渲染markdown文件", 0, 0},
	{ 2, "neofetch", "", 0, 0},
	{ 2, "neovim", "编辑器", 1, 0},
	{ 2, "nodejs", "", 1, 0},
	{ 2, "npm", "", 1, 0},
	{ 2, "ntfs-3g", "", 0, 0},
	{ 2, "openssh", "ssh", 1, 0},
	{ 2, "pacman-contrib", "", 0, 0},
	{ 2, "plantuml", "终端绘图程序", 0, 0},
	{ 2, "playerctl", "媒体播放控制", 0, 0},
	{ 2, "powerline", "状态栏插件for vim / i3", 1, 0},
	{ 2, "python-matplotlib", "", 0, 0},
	{ 2, "python-opencv", "", 0, 0},
	{ 2, "python-pip", "", 0, 0},
	{ 2, "python-pygame", "pygame", 0, 0},
	{ 2, "python3", "", 1, 0},
	{ 2, "ranger", "", 0, 0},
	{ 2, "rsync", "", 0, 0},
	{ 2, "sshfs", "", 0, 0},
	{ 2, "syslog-ng", "", 0, 0},
	{ 2, "texlive", "latex的软件包组", 0, 0},
	{ 2, "texlive-lang", "latex多语言支持的包组", 0, 0},
	{ 2, "texlive-langchinese", "latex的中文支持包", 0, 0},
	{ 2, "tmux", "", 1, 0},
	{ 2, "tree", "", 1, 0},
	{ 2, "unrar", "解压RAR文件", 0, 0},
	{ 2, "unzip", "解压ZIP文件", 0, 0},
	{ 2, "vim", "", 0, 0},
	{ 2, "yarn", "", 0, 0},
	{ 2, "yay", "AUR管家", 1, 0},
	{ 2, "zip", "压缩ZIP文件", 0, 0},
	{ 2, "zsh", "shell", 1, 0},
	/* 3 网络工具 */
	{ 3, "aria2", "下载器", 1, 0},
	{ 3, "axel", "下载器", 0, 0},
	{ 3, "curl", "下载器", 1, 0},
	{ 3, "dhcpcd", "", -1, 0},
	{ 3, "firewalld", "防火墙", 1, 0},
	{ 3, "iwd", "连接无线网络用", -1, 0},
	{ 3, "net-tools", "ifconfig", 1, 0},
	{ 3, "nginx", "本地的服务器，但建议手动安装", 0, 0},
	{ 3, "systemd-resolvconf", "", 1, 0},
	{ 3, "v2ray", "代理", 0, 0},
	{ 3, "v2raya", "web界面", 0, 0},
	{ 3, "w3m", "终端的浏览器", 0, 0},
	{ 3, "wget", "下载器", 1, 0},
	{ 3, "zerotier-one", "内网穿透", 0, 0},
	/* 4 图形界面 */
	{ 4, "plasma-meta", "安装kde依赖", 0, 0},
	{ 4, "sddm", "会话管理器", 0, 0},
	{ 4, "xorg", "X服务器", 0, 0},
	{ 4, "xorg-xinit", "", 0, 0},
	/* 5 字体 */
	{ 5, "powerline-fonts", "", 1, 0},
	{ 5, "powerline-fonts", "", 1, 0},
	{ 5, "ttf-fira-code", "", 1, 0},
	{ 5, "ttf-liberation", "", 1, 0},
	{ 5, "ttf-nerd-fonts-symbols", "", 1, 0},
	{ 5, "wqy-zenhei", "中文字体", 1, 0},
	/* 6 GUI工具 */
	{ 6, "alacritty", "第三方终端", 0, 0},
	{ 6, "ark", "压缩包查看", 0, 0},
	{ 6, "conky", "悬浮状态指示器", 0, 0},
	{ 6, "dolphin", "kde文件管理器", 0, 0},
	{ 6, "dunst", "通知程序", 0, 0},
	{ 6, "fcitx5-chinese-addons", "for中文支持", 0, 0},
	{ 6, "fcitx5-im", "包组，fcitx5输入法", 0, 0},
	{ 6, "filelight", "文件占用指示程序", 0, 0},
	{ 6, "flatpak", "flatpak程序支持", 0, 0},
	{ 6, "gnuplot", "", 0, 0},
	{ 6, "gparted", "分区工具", 0, 0},
	{ 6, "grub-customizer", "GRUB菜单图形编辑程序", 0, 0},
	{ 6, "kdeconnect", "链接你的手机", 0, 0},
	{ 6, "konsole", "kde终端", 0, 0},
	{ 6, "kvantum", "kde样式", 0, 0},
	{ 6, "latte-dock", "状态栏", 0, 0},
	{ 6, "qt5ct", "非DE的样式控制程序", 0, 0},
	{ 6, "rofi", "程序菜单", 0, 0},
	{ 6, "screenkey", "", 0, 0},
	{ 6, "slop", "", 0, 0},
	{ 6, "spectacle", "截图软件", 0, 0},
	{ 6, "tigervnc", "VNC远程桌面", 0, 0},
	{ 6, "utools", "", 0, 0},
	{ 6, "virtualbox", "虚拟机", 0, 0},
	{ 6, "wine", "运行Windows程序", 0, 0},
	{ 6, "xcompmgr", "窗口透明", 0, 0},
	{ 6, "xdotool", "终端控制键鼠", 0, 0},
	/* 7 多媒体 */
	{ 7, "alsa-utils", "(*CLI)音频管理", 1, 0},
	{ 7, "blender", "3D建模", 0, 0},
	{ 7, "cmus", "(*CLI)音乐播放器", 1, 0},
	{ 7, "ffmpeg", "(*CLI)媒体处理", 1, 0},
	{ 7, "gimp", "图片编辑", 0, 0},
	{ 7, "gwenview", "图片查看软件", 0, 0},
	{ 7, "imagemagick", "", 0, 0},
	{ 7, "inkscape", "svg图片编辑", 0, 0},
	{ 7, "mpv", "媒体播放器", 0, 0},
	{ 7, "musescore", "谱子播放器", 0, 0},
	{ 7, "obs-studio", "", 0, 0},
	{ 7, "okular", "文档查看", 0, 0},
	{ 7, "shotcut", "视频剪辑", 0, 0},
	{ 7, "vlc", "", 0, 0},
	/* 8 Java */
	{ 8, "jdk8-openjdk", "", 0, 0},
	{ 8, "jdk11-openjdk", "", 0, 0},
	{ 8, "jdk17-openjdk", "", 0, 0},
	{ 8, "java17-openjfx", "", 0, 0},
	/* 9 游戏 */
	{ 9, "cataclysm-dda", "类nethack的rougerlike游戏，有官中", 0, 0},
	{ 9, "flightgear", "飞行模拟器", 0, 0},
	{ 9, "flightgear-data", "飞行模拟器数据", 0, 0},
	{ 9, "hmcl", "'Hello Minecraft! Launcher' MC启动器", 1, 0},
	{ 9, "nethack", "", 0, 0},
	{ 9, "powder-toy", "沙子游戏", 0, 0},
	{ 9, "steam", "", 0, 0},
	{ 9, "unciv", "高仿文明5", 0, 0},
	/* 10 AUR */
	{10, "fbterm", "tty显示中文", 1, 0},
	{10, "feh", "非DE图形界面壁纸", 0, 0},
	{10, "picom-animations-git", "AUR上已经无了，但是本地有备份", 0, 0},
	{10, "transset-df", "手动调节某个窗口的透明度", 0, 0},
	{10, "netease-cloud-music", "网易云音乐", 0, 0},
	{10, "microsoft-edge-stable-bin", "edge浏览器", 1, 0},
	{10, "siji-git", "字体显示", 1, 0},
	{10, "ttf-unifont", "字体", 1, 0},
	{10, "linuxqq", "linux QQ，使用electron", 0, 0},
	{10, "xmind", "思维导图软件", 0, 0},
	{10, "wps-office-cn", "wps", 0, 0},
	{10, "wps-office-mui-zh-cn", "wps", 0, 0},
	{10, "gameconqueror", "游戏内存修改器", 0, 0},
	{10, "cmcl", "mc启动器", 1, 0},
	{10, "visual-studio-code-bin", "VScide", 0, 0},
	{10, "2048.c", "", 0, 0},
	{10, "bsd-games2", "atc", 0, 0},
	{10, "sakura-frp", "内网穿透", 0, 0},
	{10, "ttf-ms-fonts", "", 0, 0},
	{10, "sunloginclient", "向日葵远程控制", 0, 0},
	{10, "baidunetdisk-bin", "百度网盘", 0, 0},
	{10, "pcurses", "用C++写成使用ncurese的alpm前端", 0, 0},
	{10, "fsearch", "快速搜索（然并卵）", 0, 0},
	{10, "bilibili-bin", "", 0, 0},
	/* END */
	{99, NULL, NULL, 0, 0},
};
struct Command command[] = {
	/* 服务类 */
	{0, 1, "sudo systemctl enable atd"},
	{0, 0, "sudo systemctl enable dhcpcd"},
	{0, 1, "sudo systemctl enable firewalld"},
	{0, 0, "sudo systemctl enable iwd"},
	{0, 1, "sudo systemctl enable syslog-ng@default.service"},
	{0, 1, "sudo systemctl enable systemd-resolvconf"},
	/* Java链接 */
	{0, 1, "sudo ln -s /usr/lib/jvm/java-8-openjdk/bin/java /usr/bin/java8"},
	{0, 1, "sudo ln -s /usr/lib/jvm/java-11-openjdk/bin/java /usr/bin/java11"},
	{0, 1, "sudo ln -s /usr/lib/jvm/java-17-openjdk/bin/java /usr/bin/java17"},
	{0, 1, "sudo rm /usr/lib/jvm/default /usr/lib/jvm/default-runtime"},
	{0, 1, "sudo ln -s /usr/lib/jvm/java-11-openjdk/ /usr/lib/jvm/default-runtime"},
	/* npm包 */
	{0, 0, "sudo npm install -g http-server"},
	{0, 0, "sudo npm install -g neovim"},
	/* pip包 */
	{0, 0, "sudo pip install neovim"},
	/* 用户 */
	{0, 0, "sudo useradd -m Chglish"},
	{0, 0, "sudo groupadd sudo"},
	{0, 0, "sudo usermod Chglish -aG video"},
	{0, 0, "sudo usermod Chglish -aG sudo"},
	{0, 0, "sudo usermod Chglish -aG games"},
	{0, 0, "sudo usermod root -aG video"},
	/* 美化资源 */
	{0, 0, "# 请确认资源存在！"},
	{0, 0, "sudo tar -xzf res/Data.tar.gz -C /"},
	{0, 0, "sudo tar -xzf res/root_after.tar.gz -C /"},
	{0, 0, "# Tips: Documentation Pictures Videos"},
	{0, 0, NULL},
};

static void install_tips()
{
	printf("\033[0;1;32m==> \033[0;1m安装提示\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(1) iwd联网并ping检查\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(2) timedatectl set-ntp true\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(3) vim /etc/pacman.conf\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(4) 分区格式化挂载\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(5) pacstrap -i /path/ <package_name...>\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(6) genfstab -U /mnt/ >> /path/etc/fstab\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(7) arch-chroot /path/ /path/to/shell\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m(8) 或者使用快速脚本\033[0m\n"
	       "\033[0;1;32m==> \033[0;1m按下任意按键返回\033[0m\n"
	       );
	_getch();
}

static void menu()
{
	int input = 0;
	int flag = 1;
	while (input != 'q') {
		if (flag) {
			printf("\033[0;1;32m==> \033[0;1m主菜单\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m0) 备份\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m1) Archiso配置\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m2) 新系统的安装配置\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1mq) 退出程序\033[0m\n"
			       );
		}
		input = _getch();
		flag = 1;
		switch (input) {
		case '0':
			printf("\033[0;1;32m==> \033[0;1m建议使用备份脚本\033[0m\n");
			return;
			break;
		case '1':
			install_tips();
			return;
			break;
		case '2':
			menu_installed();
			return;
			break;
		case 'q':
		case 'Q':
			return;
			break;
		default:
			flag = 0;
			break;
		}
	}
	return;
}

static void help(int ret)
{
	printf("Usage: main [options]\n"
	       "Options:\n"
	       "     -h  帮助信息\n"
	       "     -l  包选择列表\n"
	       "     -m  主菜单\n"
	       );
	exit(ret);
	return;
}

int main(int argc, char *argv[])
{
	int ch = 0;
	while ((ch = getopt(argc, argv, "hlm")) != -1) {
		switch (ch) {
		case 'h':
			help(0);
			break;
		case '?':
			help(-1);
			break;
		case 'l':
			package_select();
			break;
		case 'm':
			menu();
			break;
		default:
			break;
		}
	}
	return 0;
}


static void help_in()
{
	printf("\033[0;0H\033[2J\033[0;0H");
	printf("\033[0;1;32m==> \033[0;1m内部界面帮助\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mh k 上一项\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mj l 下一项\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mg G 第一与最后一项\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mH   本帮助信息\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mq   退出软件包选择界面\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m` ` <空格>切换软件包选择状态\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m    灰色的不能更改，红色可更改\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1mA D 选择全部与取消全部(可更改的)\033[0m\n"
	       "\033[0;1;32m==> \033[0;1m按下任意按键返回\033[0m\n"
	       );
	_getch();
	printf("\033[0;0H\033[2J\033[0;0H");
}

int package_select()
{
#define list_max 11
	struct winsize size;
	const char *ch_list[list_max] = {
		"系统",
		"开发",
		"CLI工具",
		"网络工具",
		"图形界面",
		"字体",
		"GUI工具",
		"多媒体",
		"Java",
		"游戏",
		"AUR"
	};
	struct Package *tmp = package_list,
		       *mark = tmp;
	int flag = 1;
	int count = 0, lines = 0;
	int select_num = 1;
	int select_line = 1;
	int count_select = 0;
	int hide = 0;
	int input = 0;
	printf("\033[2J");
	while (input != 'q' && input != 'Q') {
		count = 0;
		lines = 0;
		count_select = 0;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		printf("\033[0;0H\033[2J\033[0;0H");
		for (int i = 0; i < list_max ; i++) {
			tmp = package_list;
			flag = 1;
			while (tmp != NULL && tmp->name != NULL) {
				if (tmp->type != i) {
					tmp++;
					continue;
				}
				if (lines - hide <= size.ws_row - 4 && count >= hide) {    /* 打印 */
					if (flag) {
						flag = 0;
						printf("\033[0;1;32m==> \033[0;1m%s\033[0m\n", ch_list[i]);
						lines++;
					}
					char *marker = NULL;
					if (tmp->select == 1) marker = "\033[0;1;31mx";
					else if (tmp->select == -1) marker = "\033[0;1;30mx";
					else marker = " ";
					if (tmp->installed == 1) marker = "\033[0;1;32mx";
					printf("\033[0;1;34m  -> \033[0;1m[%s\033[0;1m] %s - ", marker, tmp->name);
					if (tmp->desrcibe == NULL || strcmp(tmp->desrcibe, "") == 0) printf("<NULL>\033[0m\n");
					else printf("%s\033[0m\n", tmp->desrcibe);
				}
				if (tmp->select != 0) count_select++;
				if (count + 1 == select_num)
					select_line = lines + 1;
				tmp++;
				count++;
				lines++;
			}
		}
		// printf("C:%d; L:%d; SN:%d; SL:%d; HD:%d; IN:%c\n", count, lines, select_num, select_line, hide, input);
		printf("\033[0;1;32m==> \033[0;1m[%03d/%03d] | 已选：%3d | 按`H`获取帮助\033[0m\n", select_num, count, count_select);
		printf("\033[%d;7H", select_line - hide);
		kbhitGetchar();
		input = _getch();
		switch (input) {
		case 'j':
		case 'l':
			if (select_num < count) {
				while (select_line - hide > size.ws_row - 4) hide++;
				mark++;
				select_num++;
			}
			break;
		case 'k':
		case 'h':
			if (select_num > 1) {
				while (select_line - hide <= 2) hide--;
				mark--;
				select_num--;
			}
			break;
		case ' ':
			if (mark != NULL && mark->name != NULL) {
				if (mark->select == 1) mark->select = 0;
				else if (mark->select == 0) mark->select = 1;
			}
			break;
		case 'A':
			tmp = package_list;
			while (tmp != NULL && tmp->name != NULL) {
				if (tmp->select == 0) tmp->select = 1;
				tmp++;
			}
			break;
		case 'D':
			tmp = package_list;
			while (tmp != NULL && tmp->name != NULL) {
				if (tmp->select == 1) tmp->select = 0;
				tmp++;
			}
			break;
		case 'g':
			select_num = 1;
			select_line = 1;
			hide = 0;
			mark = package_list;
			break;
		case 'G':
			for (int i = select_num; i < count; i++) mark++;
			select_num = count;
			select_line = lines;
			while (select_line - hide > size.ws_row - 4) hide++;
			break;
		case 'H':
			help_in();
			break;
		default:
			break;
		}
	}
	printf("\033[0;0H\033[2J\033[0;0H");
	// printf("\033[2J\033[0;0H%d packages and %d lines total.\n", count, lines);
	return 0;
#undef list_max
}


int cmd_query(struct Package *p)
{
	char command[1024] = "pacman -Q ";
	if (p != NULL && p->name != NULL) {
		strcat(command, p->name);
		return (system(command) == 0 ? 1 : 0);
	}
	return -1;
}

int cmd_pacman()
{
	char command[8192] = "sudo pacman -S ";
	struct Package *tmp = package_list;
	int count = 0;
	while (tmp != NULL && tmp->name != NULL) {
		if (tmp->type != 10 && (tmp->select == -1 || tmp->select == 1) && tmp->installed == 0) {
			strcat(command, tmp->name);
			strcat(command, " ");
			count++;
		}
		tmp++;
	}
	if (count) {
		printf("\033[0;1;32m==> \033[0;1m输出命令\033[0m\n"
		       "\033[0;1;34m  -> \033[0;1m%s\033[0m\n",
		       command
		       );
		int result = system(command);
		if (result) {
			printf("\033[0;1;33m==> \033[0;1m命令执行错误\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m退出码：%d\033[0m\n", result);
		}
		return result;
	}
	printf("\033[0;1;32m==> \033[0;1m没有需要安装的包\033[0m\n");
	return 0;
}

int cmd_yay()
{
	int exit_code = 0;
	char command[8192] = "yay -S ";
	struct Package *tmp = package_list;
	int count = 0;
	while (tmp != NULL && tmp->name != NULL && exit_code == 0) {
		if (tmp->type == 10 && (tmp->select == -1 || tmp->select == 1) && tmp->installed == 0) {
			strcpy(command, "yay -S ");
			strcat(command, tmp->name);
			printf("\033[0;1;32m==> \033[0;1m输出命令\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m%s\033[0m\n",
			       command
			       );
			exit_code = system(command);
			count++;
			if (exit_code == 0) tmp->installed = 1;
		}
		tmp++;
	}
	if (count == 0) printf("\033[0;1;32m==> \033[0;1m没有需要安装的包\033[0m\n");
	if (exit_code) {
		printf("\033[0;1;33m==> \033[0;1m命令执行错误\033[0m\n"
		       "\033[0;1;34m  -> \033[0;1m退出码：%d\033[0m\n", exit_code);
	}
	return exit_code;
}

int cmd_update()
{
	printf("\033[0;1;32m==> \033[0;1m更新Pacman缓存\033[0m\n");
	return system("sudo pacman -Sy");
}


static int cmd_prepare()
{
	int result = 0;
	printf("\033[0;1;32m==> \033[0;1m安装archlinuxcn密钥\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m注意：需要启用archlinuxcn源\033[0m\n");
	result = system("sudo pacman -S archlinuxcn-keyring");
	if (result) {
		printf("\033[0;1;33m==> \033[0;1m命令执行错误\033[0m\n"
		       "\033[0;1;34m  -> \033[0;1m退出码：%d\033[0m\n", result);
	}
	return result;
}

static void cmd_mark_install()
{
	struct Package *tmp = package_list;
	int result = 0;
	while (tmp != NULL && tmp->name != NULL) {
		result = cmd_query(tmp);
		if (result != -1) tmp->installed = result;
		tmp++;
	}
	return;
}

void menu_installed()
{
	int input = 0;
	int flag = 1;
	printf("\033[0;1;32m==> \033[0;1m提示\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m请在开始前将`root_before.tar.gz`文件解压至根目录\033[0m\n"
	       "\033[0;1;34m  -> \033[0;1m示例：`tar -xzf root_before.tar.gz -C /`\033[0m\n"
	       );
	while (input != 'q') {
		if (flag) {
			printf("\033[0;1;32m==> \033[0;1m系统安装后菜单\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m0) 默认流程\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m1) 包选择界面\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m2) 更新软件包缓存\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m3) 安装archlinuxcn源的公钥\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m4) 更新软件包安装信息\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m5) 安装库软件\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m6) 安装AUR软件(需要AUR)\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m7) 选择命令\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1m8) 执行命令\033[0m\n"
			       "\033[0;1;34m  -> \033[0;1mq) 退出程序\033[0m\n"
			       );
		}
		input = _getch();
		flag = 1;
		switch (input) {
		case '0':
			if (package_select()) break;
			if (cmd_update()) break;
			if (cmd_prepare()) break;
			cmd_mark_install();
			if (cmd_update()) break;
			if (package_select()) break;
			if (cmd_pacman()) break;
			if (cmd_yay()) break;
			command_select();
			command_run();
			break;
		case '1':
			package_select();
			break;
		case '2':
			cmd_update();
			break;
		case '3':
			cmd_prepare();
			break;
		case '4':
			cmd_mark_install();
			break;
		case '5':
			cmd_pacman();
			break;
		case '6':
			cmd_yay();
			break;
		case '7':
			command_select();
			break;
		case '8':
			command_run();
			break;
		case 'q':
		case 'Q':
			return;
			break;
		default:
			flag = 0;
			break;
		}
	}
	return;
}


void command_select()
{
	struct winsize size;
	struct Command *tmp = command,
		       *mark = command;
	int count = 0;
	int select_num = 1;
	int hide = 0;
	int input = 0;
	printf("\033[2J");
	while (input != 'q' && input != 'Q') {
		count = 0;
		tmp = command;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
		printf("\033[0;0H\033[2J\033[0;0H");
		printf("\033[0;1;32m==> \033[0;1m请标记无需执行的命令\033[0m\n");
		while (tmp != NULL && tmp->cmd != NULL) {
			if (count - hide <= size.ws_row - 4 && count >= hide) {    /* 打印 */
				char *marker = NULL;
				if (tmp->mark == 1) marker = "\033[0;1;31mx";
				else marker = " ";
				printf("\033[0;1;34m  -> \033[0;1m[%s\033[0;1m] `%s`\n", marker, tmp->cmd);
			}
			tmp++;
			count++;
		}
		// printf("C:%d; L:%d; SN:%d; SL:%d; HD:%d; IN:%c\n", count, lines, select_num, select_line, hide, input);
		printf("\033[0;1;32m==> \033[0;1m[%03d/%03d]\033[0m\n", select_num, count);
		printf("\033[%d;7H", select_num - hide + 1);
		kbhitGetchar();
		input = _getch();
		switch (input) {
		case ' ':
			if (mark != NULL && mark->cmd != NULL) {
				if (mark->mark == 1) mark->mark = 0;
				else if (mark->mark == 0) mark->mark = 1;
			}
		case 'j':
		case 'l':
			if (select_num < count) {
				while (count - hide > size.ws_row - 4) hide++;
				mark++;
				select_num++;
			}
			break;
		case 'k':
		case 'h':
			if (select_num > 1) {
				while (count - hide <= 2) hide--;
				mark--;
				select_num--;
			}
			break;
		case 'g':
			select_num = 1;
			hide = 0;
			mark = command;
			break;
		case 'G':
			for (int i = select_num; i < count; i++) mark++;
			select_num = count;
			while (count - hide > size.ws_row - 4) hide++;
			break;
		default:
			break;
		}
	}
	printf("\033[0;0H\033[2J\033[0;0H");
}

int command_run()
{
	struct Command *tmp = command;
	int flag = 0;
	int count = 0;
	printf("\033[0;1;32m==> \033[0;1m执行命令\033[0m\n");
	while (tmp != NULL && tmp->cmd != NULL) {
		if (tmp->mark != 1) {
			printf("\033[0;1;34m  -> \033[0;1m%s\n", tmp->cmd);
			flag = system(tmp->cmd);
			if (!flag) tmp->mark = 1;
			else {
				printf("\033[0;1;33m==> \033[0;1m错误，退出码：%d\n", flag);
				return flag;
			}
			count++;
		}
		tmp++;
	}
	if (!count) printf("\033[0;1;32m==> \033[0;1m无可执行命令\033[0m\n");
	return 0;
}

