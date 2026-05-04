# 简介
这是我的一个测试文件仓库，装的多是测试文件，偶尔会有些有用的东西
- 目录解释
  - `./src_c` 存放c语言测试文件(多数仅限linux使用)
  - `./shell` 存放自制shell(zsh)脚本，偏实用向，目前在努力改用python实现一些功能
  - `./python` 存放自制python程序，有部分是shell程序更高效率兼跨平台的移植
  - `./manim` 存放写的一些垃圾manim动画
  - `./javascript` 存些js脚本（油猴脚本）

> 我英文渣，文件名都是乱取的 :|

# 部分可能有用的文件
## Python脚本

> 讲真的，python写完一个文件马上就能在另一个程序里import真的是太爽了

- `pytools.py` 一堆该项目的python程序的依赖，实现了相对路径计算、简易有类型要求的词典合并器（配合json用来读取存储配置）、
  将多级词典依照给出的连接符合并为单一同级词典（依旧用于加载配置）、读取文件后依照内置编码列表解码。
- `bilibili_build.py` 自制b站缓存查找、读取程序，可生成shell脚本或使用ncurses交互式地选择导出为mp3（自动添加metadata）
- `orgreader2.py` 自制org文件解析、导出器（支持生成html和tex，但主要注重于生成和Emacs结构基本相同的html页面）
- `merge2tex.py` 基于orgreader2.py，根据json配置文件将指定目录和文件合并为一个tex缩印文件（因为参数比较高压缩率比较大，
  大概30多书的文字量才能印满一本书，即大概3~4M每个.tex文件，原生org导出tex太慢还要另外手动合并或者写include，觉得麻烦而作此程序）
- `org2blogs.py` 根据json配置将.org原地转为html文件并生成首页等以构建静态博客
- `split_novel.py` 根据json配置文件将一个类轻小说文库（暂未知道对应的规范名称）的合集小说（单一txt文件）分卷分割为不同的.org文件并
  生成目录(Headline)
- `renames.py` 自动按照文件修改时间重命名文件(默认: `{前缀}%Y%m%d_%H%M%S{后缀}`重复了会尝试使用md5区分)并在临时目录生成恢复脚本(Py)，
  会自动避开已经具有显性年月日的文件，方便整理文件，效率比shell脚本高且更可控（有逆操作脚本且不易因为特殊字符炸掉）
- `move_media.py` 批量自动找出特定后缀名文件的文件名含有的时间（年月日）并移动/复制到对应路径（默认为 `%Y/%Y_%m/%Y_%m_%d`）
  （效率比shell脚本大大的高）应与renames.py配合使用
- `music_download.py` 网易云音乐下载器（仅限可免费播放，音质没保障）
- `talking_local.py` 兼容至Python3.8的单文件CLI/http聊天室软件

## c语言程序
- `tetris.c`
  单文件终端俄罗斯方块游戏(不使用ncruses)
- `Text_effects.c`
  打印ANSI转义颜色、显示效果(可供查询)
- `ASCII.c`
  打印ASCII字符表
- `print_in_box.c`
  在限制范围内打印文本（类似于窗口）(不使用ncruses)
- `socket.c`
  套接字程序（服务端+客户端）+本地化测试(locale)
- `script-helper.c`
  按照特定格式执行shell脚本，会标记执行进度并跳过已执行片段，遇到错误自动停止
- `Type_conversion.c`
  调用ffmpeg转换媒体文件(实际上使用shell实现或许更好)
- `alsa_play.c` (原名`ALSA.c`)
  原本是想要学习下Linux怎么让程序发出声音，一通ai问出来个ALSA，索性抄了代码自己
  摸索，然后根据自己的想法和需求逐渐增加了一些功能。比如说将声音保存为wav文件，
  根据自定义语法规则（类似于简谱）生成声音，读取wav文件并用ALSA播放等等。目前最
  有用的应该是用来生成纯音（毕竟合成音响还是太干了，又还不支持读写midi）（捂脸）

  更新：已经添加了简易滤波器、不同波函数、不同泛音列（模拟乐器），支持多轨
  音频合成，声音已经没有那么干了（有txt简谱示例，个人认为听起来还行）。不过
  效果还有待优化，所以应该还会持续更新（应该）

  更新：把ALSA.c拆分成了多文件以备复用

# 部分程序食用方法
## c语言程序
### 编译命令

在 `./c/` 目录运行以下命令编译所有程序：
```shell
gcc build.c -o build && ./build
```

### script-helper.c 脚本文件格式要求
```sh
# COMMAND START
# 上面这行作为命令的开始（不得改变），也可作为上一命令的结束（但不建议这么做）。
# 如果要快速禁用某条命令，则只需将开始符更改任意字符即可。
# 中间的字符会随着命令一起被打印出来（类似make）
# COMMAND DID  
# 上面这行为命令的开始，但是被标记为已执行状态（注意`DID`后还有两个空格）
# COMMAND END  
# 上面这行为命令的结束（注意`END`后还有两个空格）

# 补充：命令开始后的到结束前的所有内容都会传给system()函数执行
```
### socket.c
利用Socket实现的“聊天”工具，具有最基础的传输文本功能，基于TCP协议。

具有服务器和客户端两种方式，需要先启动服务器后再启动客户端以建立连接。

支持中英文显示（需在使用下列命令后在项目根目录下执行可执行文件才可显示英文）

```shell
msgfmt Lang/en_US/socket.po -o Lang/en_US/LC_MESSAGES/socket.mo
```
### musicSynth/music\_synth.c ALSA.c
使用以下命令进行食用
```bash
cd c/src/musicSynth
# 编译好程序，该文件不依赖libtools.a (submodule)
gcc -o music_synth lib/*.c music_synth.c -lm -fopenmp -O2
# 可选（使用ALSA播放）的程序
# gcc -o alsa_play lib/*.c alsa_play.c -lm -lasound -fopenmp -O2
# 查看帮助
./music_synth -h
# 将《小星星》保存到指定文件(自己用文件管理器或者播放器打开)
./music_synth -o "小星星.wav" -i 0
```
在c/src/musicSynth/res/中存有《One Last Kiss》的简谱（自定义格式），（[[https://www.bilibili.com/read/cv26055182][来源在这]]），如果要
食用请执行以下命令
```bash
cd c/src/musicSynth
./music_synth -o "OneLastKiss.wav" -I ./res/ALSA_OneLastKiss.txt
# 或者
cat ./res/ALSA_OneLastKiss.txt|./music_synth -o "OneLastKiss.wav"
# 抑或是
./music_synth -o "OneLastKiss.wav" < ./res/ALSA_OneLastKiss.txt
```
