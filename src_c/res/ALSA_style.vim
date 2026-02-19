" Vim syntax file
" Language:	Text(ALSA music text)

syn keyword tkey speed contained
syn keyword tkey track contained
syn keyword tkey amp contained
syn keyword tkey beates contained
syn keyword tkey notes contained
syn keyword tkey type contained
syn keyword tkey inst contained
syn keyword tkey wfunc contained
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
