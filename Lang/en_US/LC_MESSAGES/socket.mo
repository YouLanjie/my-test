��            )         �     �  	   �     �     �  9   �     .  '   D     l  A   �     �     �  	   �  *   �  �   %     �  	   �  '   �               !     4     P  +   f     �  (   �  $   �     �  =         >     K    R     o     u     |     �  &   �     �  !   �       ?        [  -   l     �  !   �  �   �     U	     g	  #   n	     �	     �	     �	  /   �	     �	  '   
     +
  $   9
     ^
     w
  J   �
     �
     �
         	           
                                                                                                                     输入  输出   (早期功能，已废弃) 使用客户端模式 使用指定的文件作为程序的输入（仅一次） 使用服务器模式 切换按下回车结束消息的功能 创建套接字错误 删除字符(需开启回车结束消息，否则它不会工作) 发送消息 取消使用回车发送消息 客户端 对方已退出，重新开始等待连接 小提示：一般情况下用`sudo lsof -i:$port`检查$port是否有进程占用$port，如果没有那么等一会可能就好了，或者更换端口 显示这条帮助 服务端 检测到退出关键词，退出程序 用法 监听套接字错误 监听端口错误 等待客户端发起连接 绑定套接字错误 结束消息(需要FLAG_ENTER显示为True) 获取信息错误 设置目标IP地址 [默认: 127.0.0.1] 设置目标端口号 [默认: 8080] 输入消息 连接成功，输入`/help`然后按ESC或回车获取帮助 退出程序 选项 Project-Id-Version: PACKAGE VERSION
Report-Msgid-Bugs-To: 
PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE
Last-Translator: FULL NAME <EMAIL@ADDRESS>
Language-Team: LANGUAGE <LL@li.org>
Language: en_US
MIME-Version: 1.0
Content-Type: text/plain; charset=UTF-8
Content-Transfer-Encoding: 8bit
 INPUT OUTPUT program (only once)(obsolete) Usage client mode Use the specified file as input to the Usage server mode toggle allow enter end of message create socket error delete a char(need FLAG_ENTER is True, if not it will not work) Send the message Cancel using carriage return to send messages Client Disconnected. Wait fot connection Tip: In general, use 'sudo lsof - i: $port' to check if there are any processes occupying the $port. If not, wait for a while or replace the port show this message Server Detected exit keyword, exit program Usage connect socket error listen socket error Waiting for the client to initiate a connection bind socket error end of message(need FLAG_ENTER is True) msg get error Set client addr [default: 127.0.0.1] Set port [default: 8080] input message The connection is successful. Type '/help' and press ESC or enter for help Exit Option 