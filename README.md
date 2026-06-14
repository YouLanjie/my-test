# 简介
这是我的一个测试文件仓库，装的多是测试文件，偶尔会有些有用的东西
- 目录解释
  - `./c` 存放c语言测试文件(多数仅限linux使用)
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
  ```bash
  # 获取帮助信息：
  ./python/orgreader2.py -h
  # 示范(输出到stdout)：
  ./python/orgreader2.py -i PATH_TO_ORG_FILE.org
  # 示范(生成latex内容并输出到stdout)：
  ./python/orgreader2.py -i PATH_TO_ORG_FILE.org -m latex
  # 示范(输出到PATH_TO_ORG_FILE.html)：
  ./python/orgreader2.py -Oi PATH_TO_ORG_FILE.org
  ```
- `merge2tex.py` 基于orgreader2.py，根据json配置文件将指定目录和文件合并为一个tex缩印文件（因为参数比较高压缩率比较大，
  大概30多书的文字量才能印满一本书，即大概3~4M每个.tex文件，原生org导出tex太慢还要另外手动合并或者写include，觉得麻烦而作此程序）
  ```bash
  # 温馨提示：可以使用以下命令获取命令行参数帮助
  ./python/merge2tex.py -h

  # 使用流程：
  # 1. 生成模板config.json(或者其他名字)
  ./python/merge2tex.py -C >config.json
  # 2. 编辑config.json(未设置项将使用默认值),
  # 在"filelist"里使用词典键指定扫描目录(自动找出org文件)
  # 词典内使用"ignore"添加黑名单,使用"add"添加额外文件，支持`filepath::start-end`指定始末行
  vim config.json

  # 3. 指定对应配置文件生成tex文件并编译(默认寻找`./config.json`)
  #  a. 或者同时由程序本身调用xelatex编译tex文件
  # 能自动生成填充目录文件并运行两次编译（计算位置生成正确的toc文件、生成有正确目录pdf文件）、
  # 显示简单进度、自动分析日志的Overflow和缺失字体问题
  ./python/merge2tex.py -ri config.json
  #  b. 生成tex文件后手动编译(编译三次(除非没有目录):生成toc,修正有了toc的位移,生成正确pdf)
  # 手动调用辅助功能的方法请使用--help查看帮助
  ./python/merge2tex.py -i config.json
  xelatex output_XXXXX.tex
  xelatex output_XXXXX.tex
  xelatex output_XXXXX.tex
  ```
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

在 `./c/` 目录运行以下命令编译所有程序(单元编译失败会显示但是会被忽略)：
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
# 单文件编译程序，该文件不依赖libtools.a (submodule)
gcc -o music_synth music_synth.c -lm -O2 -DSINGLE_FILE
#  可选（使用ALSA播放）的程序(依赖alsa)
#    gcc -o alsa_play lib/*.c alsa_play.c -lm -lasound -O2
#  可选（使用sdl2播放）的程序(依赖sdl2)
#    gcc -o alsa_play lib/*.c alsa_play.c -lm -lasound -O2
# 查看帮助
./music_synth -h
# 将《小星星》保存到指定文件(自己用文件管理器或者播放器打开)
./music_synth -o "小星星.wav" -i 0
```

在c/src/musicSynth/res/中存有《One Last Kiss》的简谱（自定义格式），（[来源在这](https://www.bilibili.com/read/cv26055182)），如果要
食用请执行以下命令(`-I`指定输入文件或者从stdin读入)
```bash
cd c/src/musicSynth
./music_synth -o "OneLastKiss.wav" -I ./res/ALSA_OneLastKiss.txt
# 或者
cat ./res/ALSA_OneLastKiss.txt|./music_synth -o "OneLastKiss.wav"
# 抑或是
./music_synth -o "OneLastKiss.wav" < ./res/ALSA_OneLastKiss.txt
```

可用示范列表：
- [TryEverything.txt](./c/src/musicSynth/res/ALSA_TryEverything.txt)
- [OneLastKiss.txt](./c/src/musicSynth/res/ALSA_OneLastKiss.txt)
- [OverworldDay.txt](./c/src/musicSynth/res/ALSA_OverworldDay.txt)
- [BeautifulWorld.txt](./c/src/musicSynth/res/ALSA_BeautifulWorld.txt)
- [misc.txt](./c/src/musicSynth/res/ALSA_misc.txt)
- [NeverGonnaGiveYouUp.txt](./c/src/musicSynth/res/ALSA_NeverGonnaGiveYouUp.txt)
- [SuperMario.txt](./c/src/musicSynth/res/ALSA_SuperMario.txt)

语法([建议搭配这个vim配置文件食用](./c/src/musicSynth/res/ALSA_style.vim))：
```txt
:---关于元数据---;
:这种是注释（内部不含等于号，以冒号开头，分号结尾）;
:若注释内含等于号会被解析为元数据设定，例如像下面这样;
:track=1;  :设置轨道为1;
:track=0;  :最后一定要设置轨道为0完成并轨;

:举例，下面的音符会同时播放(时间线与刚切回0轨道的时间对其);
:track=1; C
:track=2; D
:track=3; E
:track=0; F

0*.

:实际上track这种key还可以使用`.`区分出subkey(目前仅支持二级);
:若完全匹配不到合法的key则会自动打印所有可用key，例如;
:=;
:要获得结果需要自己另存为跑一下这个说明,命令：`music_synth -I path_to_file.txt`;
:若有匹配到相同开头的key则会进行猜测推荐，例如;
:n=;
:若有子命令的在`.`后留随意非法值获得提示，例如;
:note. =;
:没有子命令的不会有提示，例如;
:track. =;
:可用值为字符串的可以让值留空以获得提示，例如;
:inst=;
:inst=pian;

:---关于音符---;
:0 是休止符;
:cdefgab 是低音部;
:CDEFGAB 是中音部;
:1234567 是高音部,例如;
c d e f g a b 0 C D E F G A B 0 1 2 3 4 5 6 7
:使用`/`和`*`调节时值,默认为4分音符，使用以下命令设定;
:note.notes=4;
:`/`使n分音符的n翻倍（时值减半），`*`使n分音符的n减半（时值翻倍）,如;
C/D//D// :左边为前八后十六音符;
0*       :左边为二分休止符;
:`.`为附点符号，时值变为1.5倍，应当放在`/`或者`*`后;

:---关于升降调---;
c    :表示低音;
cL   :表示重低音(降低了一个八度);
cLL  :表示超重低音(降低了两个八度);
7    :表示高音;
7U   :表示超高音(升高了一个八度);
7UU  :表示超超高音(升高了两个八度);

Cl   :表示中央C降低了一个半音;
Cu   :表示中央C升高了一个半音;

:---关于演奏---;
C~D  :表示连音（共用ADSR控制,中间有微小过度，基本没有ATTACK声）;
CsD  :表示滑音（频率由前者滑动到后者，不建议和连音一起用，支持不好）;
```
