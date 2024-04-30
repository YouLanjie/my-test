#include <stdio.h>
#include <gtk/gtk.h>

int main(int argc, char *argv[]) {
	int a = 1;
	//初始化
	gtk_init(&argc,&argv);
	//创建窗口
	GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	//创建安水平容器
	GtkWidget * hbox = gtk_hbox_new(TRUE, 10);
	//创建垂直容器
	GtkWidget * vbox = gtk_vbox_new(TRUE, 10); 
	//创建表格容器
	GtkWidget * table = gtk_table_new(2, 2, TRUE);
	//创建固定容器
	GtkWidget * fixed = gtk_fixed_new();
	//创建按钮
	GtkWidget * button;
	while(a != 0) {
		printf("请选择窗口类型（按下Ctrl + C 退出）：\n0.退出\t1.水平\t2.垂直\n3.水平加垂直\t4.表格\n5.固定布局\t\n");
		scanf("%d",&a);
		switch(a) {
			case 1:
				//把容器放进窗口
				gtk_container_add(GTK_CONTAINER(window), hbox);

				//按钮1
				button = gtk_button_new_with_label("button1");
				//把按钮装进水平容器里
				gtk_container_add(GTK_CONTAINER(hbox), button);
	
				//按钮2
				button = gtk_button_new_with_label("button2");
				//把按钮装进水平容器里
				gtk_container_add(GTK_CONTAINER(hbox), button);

				//按钮3
				button = gtk_button_new_with_label("button3");
				//把按钮装进水平容器里
				gtk_container_add(GTK_CONTAINER(hbox), button);

				//显示所有控件
				gtk_widget_show_all(window);
				break;
			case 2:
				//把容器装进窗口
				gtk_container_add(GTK_CONTAINER(window), vbox);

				//按钮1
				button = gtk_button_new_with_label("button1");
				//把按钮装进水平容器里
				gtk_container_add(GTK_CONTAINER(vbox), button);

				//按钮2
				button = gtk_button_new_with_label("button2");
				//把按钮装进水平容器里
				gtk_container_add(GTK_CONTAINER(vbox), button);

				//按钮3
				button = gtk_button_new_with_label("button3");
				//把按钮装进水平容器里
				gtk_container_add(GTK_CONTAINER(vbox), button);

				//显示所有控件
				gtk_widget_show_all(window);
				break;
			case 3:
				//把容器装进窗口
				gtk_container_add(GTK_CONTAINER(window), vbox);
				//把容器放进容器
				gtk_container_add(GTK_CONTAINER(vbox), hbox);

				//按钮1
				button = gtk_button_new_with_label("button1");
				//把按钮装进水平容器里
				gtk_container_add(GTK_CONTAINER(hbox), button);

				//按钮2
				button = gtk_button_new_with_label("button2");
				//把按钮装进水平容器里
				gtk_container_add(GTK_CONTAINER(hbox), button);

				//按钮3
				button = gtk_button_new_with_label("button3");
				//把按钮装进水平容器里
				gtk_container_add(GTK_CONTAINER(vbox), button);

				//显示所有控件
				gtk_widget_show_all(window);
				break;
			case 4:
				//将容器加入窗口
				gtk_container_add(GTK_CONTAINER(window), table);

				// button 1
				button = gtk_button_new_with_label("buttton 1");
				gtk_table_attach_defaults(GTK_TABLE(table), button, 0, 1, 0, 1);// 把按钮加入布局

				// button 2
				button = gtk_button_new_with_label("button 2");
				gtk_table_attach_defaults(GTK_TABLE(table), button, 1, 2, 0, 1);

				// close button
				button = gtk_button_new_with_label("close");
				gtk_table_attach_defaults(GTK_TABLE(table), button, 0, 2, 1, 2);

				gtk_widget_show_all(window);	// 显示窗口控件
				break;
			case 5:
				gtk_container_add(GTK_CONTAINER (window), fixed); // 固定放进窗口
				button = gtk_button_new_with_label("^_^");
				// 按钮添加到固定布局
				gtk_fixed_put(GTK_FIXED(fixed), button, 0, 0);

				button = gtk_button_new_with_label("@_@");  // 创建按钮
				gtk_fixed_put(GTK_FIXED(fixed), button, 0, 0); // 按钮添加到固定布局

				gtk_fixed_move(GTK_FIXED(fixed), button, 150, 150); // 移动控件的位置

				gtk_widget_set_size_request(button, 100, 50); // 设置控件的大小

				gtk_widget_show_all(window);  // 显示所有控件
				break;
			default:
				break;
		}
		if(a != 0) {
			//主事件循环
			gtk_main();
		}
	}
	return 0;
}

