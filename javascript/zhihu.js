// ==UserScript==
// @name         关闭知乎登录提示(适用于手机用户)
// @namespace    https://viayoo.com/
// @version      0.1.1
// @description  自动点击
// @author       水煮木头 Water cook wood
// @match        *://*.zhihu.com/*
// @icon
// @grant        none
// @license      MIT
// @noframes
// ==/UserScript==

(function() {
	function addbot(text) {
		let dom = document.createElement('a');
		dom.style.position = "fixed"
		dom.style.display = "block"
		dom.style.padding = "5px"
		dom.style.border = "1px"
		dom.style.width = "2em"
		dom.style.color = "white"
		dom.style.overflow = "auto"
		dom.style.left = 0
		dom.style['top'] = "50vh"
		dom.style['background-color'] = "#1d99f3";
		dom.style['font-size'] = "20px"
		dom.style['z-index']="999999";
		dom.innerHTML = text;
		return dom
	}
	document.head.insertAdjacentHTML("beforeend", '<meta name="viewport" content="width=device-width, initial-scale=1" />')
	let dom = addbot("点击关闭")
	function del() {
		buttons = document.getElementsByClassName("Modal-closeButton");
		if (buttons.length != 0) {
			buttons[0].click();
		}
		dom.remove()
	}
	dom.onclick = del;
	document.body.appendChild(dom);
	setTimeout(del, 5000)
})();
