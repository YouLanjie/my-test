" 保存当前脚本的完整绝对路径（包含文件名）
let s:script_path = expand('<sfile>:p')

" 判断函数
function! s:IsALSAFile()
	" 检查前20行是否出现 ":xxx=yyy;" 模式的设置行
	for i in range(1, min([20, line('$')]))
		let line = getline(i)
		if line =~ '^:[a-zA-Z_][a-zA-Z0-9_]*=.*;'
			return 1
		endif
	endfor
	return 0
endfunction

if <SID>IsALSAFile()
	" 初始化模式变量（0:低音区, 1:中音区, 2:高音区）
	if !exists("b:alsa_mode")
		let b:alsa_mode = 1
	endif

	" 定义各模式下的音符映射表
	" 模式0: 低音区小写字母 cdefgab
	" 模式1: 中音区大写字母 CDEFGAB
	" 模式2: 高音区数字 1234567
	let s:note_table = [
		\ ['c', 'd', 'e', 'f', 'g', 'a', 'b'],
		\ ['C', 'D', 'E', 'F', 'G', 'A', 'B'],
		\ ['1', '2', '3', '4', '5', '6', '7']
	\]
endif

" 函数：更新状态栏显示
function! s:UpdateModeStatus()
	let mode_name = ['超低音区', '低音区', '中音区', '高音区', '特高音区'][b:alsa_mode]
	echo "ALSA模式: " . mode_name
	" 也可以设置状态栏变量，例如：
	" let &l:statusline = '%{exists("b:alsa_mode")?"ALSA:".["低","中","高"][b:alsa_mode]." ":"".''}'.&statusline
endfunction

" 函数：切换模式
function! s:SetMode(mode)
	let b:alsa_mode = a:mode
	call s:UpdateModeStatus()
	if a:mode == 0
		inoremap <buffer> 1 cL
		inoremap <buffer> 2 dL
		inoremap <buffer> 3 eL
		inoremap <buffer> 4 fL
		inoremap <buffer> 5 gL
		inoremap <buffer> 6 aL
		inoremap <buffer> 7 bL
	elseif a:mode == 1
		inoremap <buffer> 1 c
		inoremap <buffer> 2 d
		inoremap <buffer> 3 e
		inoremap <buffer> 4 f
		inoremap <buffer> 5 g
		inoremap <buffer> 6 a
		inoremap <buffer> 7 b
	elseif a:mode == 2
		inoremap <buffer> 1 C
		inoremap <buffer> 2 D
		inoremap <buffer> 3 E
		inoremap <buffer> 4 F
		inoremap <buffer> 5 G
		inoremap <buffer> 6 A
		inoremap <buffer> 7 B
	elseif a:mode == 3
		silent! iunmap <buffer> 1
		silent! iunmap <buffer> 2
		silent! iunmap <buffer> 3
		silent! iunmap <buffer> 4
		silent! iunmap <buffer> 5
		silent! iunmap <buffer> 6
		silent! iunmap <buffer> 7
	elseif a:mode == 4
		inoremap <buffer> 1 1U
		inoremap <buffer> 2 2U
		inoremap <buffer> 3 3U
		inoremap <buffer> 4 4U
		inoremap <buffer> 5 5U
		inoremap <buffer> 6 6U
		inoremap <buffer> 7 7U
	endif
endfunction

function! s:LoadALSACfg()
	" 语法
	syn clear
	syn keyword tkey note contained
	syn keyword tkey key_name contained
	syn keyword tkey speed contained
	syn keyword tkey wave_func contained
	syn keyword tkey har contained
	syn keyword tkey beates contained
	syn keyword tkey notes contained
	syn keyword tkey type contained

	syn keyword tkey track contained
	syn keyword tkey amp contained

	syn keyword tkey bq contained
	syn keyword tkey inst contained
	hi tkey guifg=cyan

	syn match tctrl '[\*/.<>=~0]'
	hi tctrl guifg=gray

	syn match tnotel '[cdefgab]'
	hi tnotel guifg=Green
	syn match tnoteh '[1234567]'
	hi tnoteh guifg=Orange

	syn match tcomment ':.*;' contains=tckey
	syn match tckey '[^:][^=]*=' contained contains=tkey nextgroup=tcvalue
	syn match tcvalue '[^;]*' contained
	hi tcomment guifg=gray
	hi tckey guifg=red
	hi tcvalue guifg=white

	inoremap <buffer> ]t :track=0;
	" 普通模式快捷键切换模式（仅在当前缓冲区有效）
	inoremap <buffer> 9` <CMD>call <SID>SetMode(0)<CR>
	inoremap <buffer> 91 <CMD>call <SID>SetMode(1)<CR>
	inoremap <buffer> 92 <CMD>call <SID>SetMode(2)<CR>
	inoremap <buffer> 93 <CMD>call <SID>SetMode(3)<CR>
	inoremap <buffer> 94 <CMD>call <SID>SetMode(4)<CR>

	vnoremap <buffer> <leader>f :!music_synth -p<CR>
	vnoremap <buffer> <leader>l :!gen_music_synth_str.py<CR>
	vnoremap <buffer> <leader>r :!gen_music_synth_str.py -r<CR>

	nnoremap <buffer> <leader>i :r!gen_music_synth_str.py -p<CR>
	nnoremap <buffer> <leader>I :r!gen_music_synth_str.py -p low<CR>

	execute 'nnoremap <buffer> <leader>e :e ' . fnameescape(s:script_path) . '<CR>'
	execute 'nnoremap <buffer> <leader>l :source ' . fnameescape(s:script_path) . '<CR>'
endfunction

" 自动加载配置
if <SID>IsALSAFile()
	call s:LoadALSACfg()
endif
au BufRead,BufNewFile *.txt if <SID>IsALSAFile() | call s:LoadALSACfg() | endif
