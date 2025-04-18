# my-test

- 这是我的一个测试文件仓库
- 在`src_c`文件夹存放c语言测试文件
  - 大都只适用于Linux系统

> 我英文渣，文件名都是乱取的 :|

## 文件功能简表

| 文件名                     | 测试的功能                                                                 |
|:--------------------------:|:--------------------------------------------------------------------------:|
| **Type_conversion.c**      | Linux下调用ffmpeg来对媒体文件进行格式转换(懒得打命令)                      |
| *ASCII.c*                  | 打印ASCII码所对应的字符（因为懒得查）                                      |
| ***Text_effects.c***       | 测试Linux下printf加上\033的文字显示效果测试                                |
| ***script-helper.c***      | 一条接着以掉执行指定文件的命令（格式固定）方便重装系统                     |
| *filelist.c*               | 查看指定目录下的文件和文件夹                                               |
| *gettime.c*                | 获取时间                                                                   |
| clock.c                    | 用于测试Linux的时钟api函数setitimer函数功能                                |
| readfile.c                 | 不断地查看指定文件的内容（适用于小文件）                                   |
| socket.c                   | 使用套接字的简易本地聊天程序，支持中英文输入，无使用ncruses                |
| chinese.c                  | 辨别输入的内容是中文还是英文                                               |
| input\_to\_hex.c           | 将输入的内容拆分成多个字符并使用十六进制显示，方便查找快捷键对应的输入字符 |
| fork.c                     | 将使用fork创建新的进程执行命令，方便后台执行程序                           |
| history\_note.c            | 正在制作中的历史笔记显示程序                                               |
| prime\_number.c            | 计算质数                                                                   |
| input.c                    | 测试固定类型输入的输入框                                                   |
| farme.c                    | 多线程显示处理测试（已改为走迷宫，可和`maze.c`配合使用）                   |
| maze.c                     | 可视化生成迷宫，数据将保存在`maze.txt`中                                   |
| guess\_number.c            | 猜数字                                                                     |
| **tetris.c**               | 俄罗斯方块                                                                 |
| print\_in\_box.c           | 在一定的限定范围内打印                                                     |
| arch-fast-install.c        | 快速按照Archlinux的测试程序                                                |
| neuro.c                    | 神经网络测试程序，输入2个数判断它们的正负及大小，正确率在90%~94%左右浮动   |

> 注：文件名为斜体的为不再改动的文件，粗体为相对有用的文件

- 这些测试大都没有什么用... :|

## 编译命令

请使用命令

```shell
git submodule init
git submodule update
```

下载子模块仓库，否则会有部分程序无法编译

请在项目目录运行以下命令编译所有程序（注：该脚本需要使用zsh）：

```shell
cd ./src_c
./build.sh
```

更多帮助内容请使用`./build.sh -h`查看

或者使用以下命令编译大部分程序

```shell
cd ./src_c
ls -1|grep ".c$"|sed "/build.c\|gtk.c/d"|sed "s/^\(.*\).c$/gcc \1\.c $(find ./include/lib -name "*.c"|sed ":a;N;s/\n/ /g;b a"|sed 's/\//\\\//g') -lncurses -lm -o bin\/\1/"|sh
```

再或者编译 `build.c` 程序，再通过执行编译出来的可执行文件编译大部分程序

## 部分程序运行效果

> 注：一下介绍的这些程序我都是自认有用的

- Type\_conversion.c
  - 寻找指定文件夹下的文件（单级）并创建多个进程同时调用ffmpge以快速地转换文件（格式）（不用打命令半天）

  - 命令格式

    ```shell
    Type_conversion -d 文件夹 -t 目标格式后缀
    ```

- Text\_effects.c

  - 将终端的颜色、显示效果全部打印出来

- script-helper.c

  - 一个简易的“解析器”，通过读取指定的shell脚本文件逐条（使用绿色）打印并执行对应的命令，能在遇到错误时退出，并打印错误的命令及其返回值，在执行命令后会在文件中作标记，重新执行程序时就不会调用该命令。

  - 灵感来源于我常重装系统，需要一个能够自动处理脚本错误（退出），重新执行命令时能够跳过已执行的命令的程序。

  - 命令格式：

    ```txt
    Usage: script-helper <options>
        -f  指定脚本文件
        -d  执行脚本
        -r  重置脚本
        -h  打印帮助信息并退出
    ```

  - 例子

    - 执行脚本

      ```shell
      $ script-helper -d -f file.sh
      ```

    - 重置脚本文件

      ```shell
      $ script-helper -r -f file.sh
      ```

    - 执行程序后自动重置脚本文件

      ```shell
      $ script-helper -b -f file.txt
      ```

  - 脚本文件“语法”

    - 以`# COMMAND START`为命令的开始，也可作为上一命令的结束（但不建议这么做）。如果要快速禁用某条命令，则只需将开始符更改任意字符即可。
    - 以`# COMMAND END  `为命令的结束（注意：`END`后还有两个空格）
    - 以`# COMMAND DID  `为已执行命令（注意：`DID`后还有两个空格）
    - 在命令中`#`的注释也会被读取，但是它会被shell解释为注释，并且程序会打印整条命令，所以不妨在命令中增加一些注释方便改正文件的错误。
    - 其他的全部遵循脚本的语法，不过命令之间不相通。

- socket.c

  - 利用Socket实现的“聊天”工具，具有最基础的传输文本功能，基于TCP协议。
  
  - 具有服务器和客户端两种方式，需要先启动服务器后再启动客户端以建立连接。
  
  - 支持中英文显示（需在使用命令`msgfmt Lang/en_US/socket.po -o Lang/en_US/LC_MESSAGES/socket.mo`后在项目根目录下执行可执行文件才可显示英文）
