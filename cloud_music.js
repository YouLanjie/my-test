// ==UserScript==
// @name         网易云音乐信息快速获取
// @namespace    http://tampermonkey.net/
// @version      0.1.6
// @description  解析下载链接
// @author       水煮木头
// @match        *://music.163.com/*
// @icon         data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAABHNCSVQICAgIfAhkiAAABYxJREFUWEfFV3tQVGUU/9172YWBZXktGIEWPjKzGKRGiElHG61MTQNimMlS7KFkOk2TWc7wRy91+sNpJhMqG5seNlYYY6YpTTZWvnqIOL4VUkFUQBBYYJ+375y7e2N3724K03Rm7u73ne+c8zvfd893zrkSrpOOIyMlGupcFZgKSNlCLUs8Fp96j/hvBNR6CdjtgFQzDs3t12NayEemBmRmq1BfFVJF4jFFltZXXWJULUFaPRJN9ZF0wjpwArZ4E6LfFsqLxBNWLpJxsSYODO+74Hj5drR1G8kaGj6NEeNleGqEwmgjpUHwznihzB2D80eDdUMcaERGvhfSTuG8NVh4aHOpS4b6YBaa9w+0E+CAtnPv3sGAm8aOQvTEHHivdqKvdg9UJ4VBMEldXsgFA09Cd8D3zv8UKjd87Emvv4Skihd0NNfpRrTcXwJ3U0uwBzQ/I2Ii1x8Tsl/CF3ARwZWUJAZiMEnzPabgHh3ceeQE1L5+mMZkwfJksRE48Ub7sHg9in58V42iPSxZSufAtn4V5KQElunasAmeliuImXIvz/v3/YGLBXNgvuM2WOYXo3vj5rC2xMIigVlJV5RPwHfPQwLSbyHxlSVI++I9BvdcbkXb0goGJ/Je026XHK/lJOexU7i6YpW+HsYLyYcJhTKcAnwkBMVfKMXPfwy2dW/ygn3LDrRMK0X/zwd4bho5ArGzpiEmbwKUNBuibsmAkpIM99lzUB3OUGOBnLHlsK6XGnDzUyqkDUbSSnoahp/cA9pdz6YaXJm3VByXkDabkLxmJazPl0Ey8VsMIE97B9qXVbBOJJKgPh2l5XZjscTl5QzuPteE1meWMzhR2ufrEFc8k8f9vxxkfsykPBCwpCjiFJJYRk6woqvyE2PjgkvYIga4sISSMGSZV8j8ztXroPb28Tiu6GEGV91uXCl9DhcnFfI6k8uN81n5sH/zPU9t774Bc854bc3wV8qmIMwyWjOPGw0lNYWX7F9/p4tYFz/B42trP0DP5q087vtpH9R+B5SbUkUcZArHyuH47bCIKgXJb63QdQ0GWeSAv6QGrEfdOpzn7gsX+Wj9ZJ5wJw/t1dt1Ht19x4FDPI++O5uz4NWVa3ge+9AUfiVhyKInomABOSGeWaq9N3DJ4+G5ZAqszJ6OTubLVm0/fbv3arqyDHPuXYE2BszIAWomQsjr27WSPixgzbGfsjWQsGyhzqdXFXPfRJ67TpzR+MJR9+U2HvuTl67wz6CHHBCdTChRWiWikxgYSB0UcF4v4kpmI/PQTqRuXIvMw7VQbMlwnTyLvh9/1Y0piVpBVXvsoQAap1E4oBp2LO7mS3DWH2ex+IWlugE6gdayFznoyLH4BSWgfOH+6wIuFz+rV0Fak5MTWc95+JiuHzgQLVykREQRb6tcDVVcr+YJD8B59JSuT6BxRTOhCBDauX3rLi5EfhpW/SHiCmfAcbAOzXmzDB2gRCRRKjYDVDdD+j3Kchl1u7jAuBrOo2VyIehk/o2sSxbo6fvSI2Xo/bbWSMUlknW67Oteq40kaOeUbLxd3Zz3M37fgdgZU41EmSfFxSLlndd08O6PvwwHTuLVhM0V0FeO68gGWwqi6PxcpG//VI9muvOUB5xHjnM1pNcRMzmfM6f/ztu/2sa1w7gzgio65hwqxzrgWWRUCtzFweD+eVRmOvcDsbOnhxNhPjlESYhrgK92GChUjUJzOfFvuCWjTGd5/FFuREwiW0rRZng6rsFZdxS9235Az2db4A1/7QgzoCULOPKhNKUGuzRghTalAamYulVqncXBdBloD5GlteXB3wYhtYD6dmqd6aiGiDhQXXyYyAXB3wQkEOIAMclLap3FsEo8WhcyOG9It4psBe/cb87w2g3E+t8+ToM3/F99nv8NEHIVJRhx67UAAAAASUVORK5CYII=
// @grant        none
// @license      MIT
// @noframes
// ==/UserScript==

(function() {
    'use strict';
    function copy(text) {
        let txa = document.createElement('textarea');
        txa.value = text;
        document.body.appendChild(txa)
        txa.select()
        let res = document.execCommand('copy')
        document.body.removeChild(txa)
        console.log('复制成功!');
        //alert("复制成功！确认通知后自动暂停音乐");
    }
    function load() {
        var download_url = "NULL";
        var img = "NULL";
        var title = "NULL";
        var subtitle = "NULL";
        var artists = "NULL";
        var album = "NULL";
        let flag_info = 0;
        function local_copy(){
            if (JSON.stringify(subtitle) === '{}') {
                copy(download_url + "\n" + img[0].currentSrc + "\n" + title[0].innerText + "\n2\n" + artists + "\n" + album[0].innerText + "\n");
                local_notice(download_url, img[0].currentSrc, artists + " - " + title[0].innerText, album[0].innerText);
            } else {
                copy(download_url + "\n" + img[0].currentSrc + "\n" + title[0].innerText + "\n1\n" + subtitle[0].innerText + "\n" + artists + "\n" + album[0].innerText + "\n");
                local_notice(download_url, img[0].currentSrc, artists + " - " + title[0].innerText + "(" + subtitle[0].innerText + ")", album[0].innerText);
            }
        }
        // UI结果显示
        function local_notice(music_link, cover_link, filename_no_ext, album) {
            if (flag_info != 0) {
                document.getElementById("local_result").remove();
            }
            flag_info++;
            let dom = document.createElement('p');
            dom.id = "local_result";
            dom.innerHTML = "累计请求次数：" + flag_info;
            // dom.style = "max-width: 30%;";
            document.getElementById("sp-ac-container").appendChild(dom);
            dom.appendChild(document.createElement('br'));
            function copy_name() {
                copy(filename_no_ext);
            }
            let dom2 = document.createElement('a');
            dom2.id = "local_result_music";
            dom2.href = music_link;
            dom2.innerText = "音乐链接";
            if (music_link == 'NULL') {
                dom2.innerText = "音乐链接:NULL";
            }
            dom2.onclick = copy_name;
            dom.appendChild(dom2);
            dom.appendChild(document.createElement('br'));
            let dom3 = document.createElement('a');
            dom3.id = "local_result_cover";
            dom3.href = cover_link;
            dom3.innerText = "封面链接";
            dom3.onclick = copy_name;
            dom.appendChild(dom3);
            dom.appendChild(document.createElement('br'));
            let dom4 = document.createElement('p');
            dom4.innerText = "专辑：" + album;
            dom.appendChild(dom4);
            dom.appendChild(document.createElement('br'));
            let dom5 = document.createElement('a');
            dom5.id = "local_button_recopy";
            // dom5.href = cover_link;
            dom5.innerText = "点这重新复制";
            dom5.onclick = local_copy;
            dom.appendChild(dom5);
        }
        function local_func (){
            var temp = document.getElementById("g_iframe").contentWindow;  // 进入新的窗口
            img = temp.document.getElementsByClassName("j-img");       // 封面
            title = temp.document.getElementsByClassName("f-ff2");     // 歌曲名
            subtitle = temp.document.getElementsByClassName("subtit"); // 副标题

            var temp2 = temp.document.getElementsByClassName("s-fc4");     // 歌手 + 专辑
            var artist = temp2[0].getElementsByClassName("s-fc7");         // 歌手
            album = temp2[1].getElementsByClassName("s-fc7");          // 专辑

            var play = temp.document.getElementsByClassName("u-btn2");
            const event = new MouseEvent('click', {
                view: window,
                bubbles: true,
                cancelable: true
            });
            play[0].dispatchEvent(event);

            setTimeout(function (){
                var tmp=performance.getEntriesByType("resource");
                download_url = "NULL";
                if (JSON.stringify(tmp) === '{}') {
                    console.log("警告！您未有任何请求资源！");
                    download_url = "";
                }
                for (var i = tmp.length - 1; i >= 0; i--) {
                    if (tmp[i].initiatorType == "audio") {
                        console.log("下载链接：\n" + tmp[i].name);
                        download_url = tmp[i].name;
                        break;
                    }
                }
                if (download_url == "NULL") {
                    console.log("警告！未能成功获取下载链接！请在播放您想要下载的资源后重试！");
                }

                console.log("封面链接：\n" + img[0].currentSrc);
                console.log("曲名：\n" + title[0].innerText);
                if (JSON.stringify(subtitle) !== '{}') {
                    console.log("副标题：\n" + subtitle[0].innerText);
                }
                artists=artist[0].innerText;
                for(var i2 = 1, len = artist.length; i2 < len; i2++) {
                    artists = artists + "," + artist[i2].innerText;
                }
                console.log("歌手：\n" + artists);
                console.log("专辑：\n" + album[0].innerText);
                local_copy();
                // alert("下载链接" + download_url + "封面链接：\n" + img[0].currentSrc + "曲名：\n" + title[0].innerText + "歌手：\n" + my_local[1].innerText + "\n专辑：\n" + my_local[2].innerText);
                document.getElementsByClassName("ply")[0].click();
            }, 2000);
            /*   ^
                 |
                 |        (n000为n秒，默认为2秒)
                 `------  修改这里的数值以更改延迟
            */
        }
        let Container = document.createElement('div');
        Container.id = "sp-ac-container";
        Container.style.position="fixed";
        Container.style.left="0px";
        Container.style.top="20%";
        Container.style['z-index']="999999";
        let dom = document.createElement('button');
        dom.id = "myCustomize";
        dom.style = "position:relative;left:0px;top:0px;background-color: darkgray;padding: 5px;margin: 0px 0px 15px 0px;font-size: 13px;border: 1px;box-shadow: 0 0 5px;width: 2em;";
        dom.innerHTML = "点击查询";
        dom.onclick = local_func;
        Container.appendChild(dom);
        document.body.appendChild(Container);
    }
    window.onload = load;
})();

