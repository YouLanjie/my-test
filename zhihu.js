// ==UserScript==
// @name         关闭知乎登录提示
// @namespace    https://viayoo.com/
// @version      0.1.0
// @description  自动点击
// @author       水煮木头 Water cook wood
// @match        *://*.zhihu.com/*
// @icon
// @grant        none
// @license      MIT
// @noframes
// ==/UserScript==

(function() {
    'use strict';
    function load() {
        // button = document.getElementsByClassName("Modal-closeButton");
        setTimeout(function (){
            // button[0].click();
            document.getElementsByClassName("Modal-closeButton")[0].click()
        }, 5000);
    }
    window.onload = load;
})();


