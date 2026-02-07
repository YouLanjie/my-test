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
	function addsource() {
		alert("11")
		let pre = document.getElementById("HTML_PRE_HEAD")
		if (pre) {
			pre.remove()
		}
		pre = document.getElementById("HTML_PRE_BODY")
		if (pre) {
			pre.remove()
		}
		alert("12")

		style = 'height: 100%; max-height: 100%; font-size: 90%; background-color: "#1e1e1e";; color: #EEE; /*border: 4px solid gray;*/ border-radius: 6px; border: 0px; border-left: 3px solid "#bb86fc";; line-height: 125%; position: static; overflow: auto; padding: 8pt; margin-left: 1em; margin-right: 1em;'
		head = document.head.innerHTML
		body = document.body.innerHTML

		pre = document.createElement("pre")
		pre.id = "HTML_PRE_HEAD"
		pre.style = style
		pre.innerText = head
		document.body.insertAdjacentElement("beforeend", pre)
		alert("13")

		pre = document.createElement("pre")
		pre.id = "HTML_PRE_BODY"
		pre.style = style
		pre.innerText = body
		document.body.insertAdjacentElement("beforeend", pre)
		alert("14")
	}
	let pre = addbot("ViewSource")
	pre.style['top'] = "70vh"
	pre.onclick = addsource;
	document.body.appendChild(pre);
})();
