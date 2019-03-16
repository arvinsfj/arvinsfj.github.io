// JavaScript to generate a compact date representation

//
// format date as dd-mmm-yyyyy
// example: 12-Jan-1999
//
function date_ddmmmyyyy(date)
{
    var d = date.getDate();
    var m = date.getMonth() + 1;
    var y = date.getFullYear();
    
    // could use splitString() here
    // but the following method is
    // more compatible
    var mmm =
    ( 1==m)?'Jan':( 2==m)?'Feb':(3==m)?'Mar':
    ( 4==m)?'Apr':( 5==m)?'May':(6==m)?'Jun':
    ( 7==m)?'Jul':( 8==m)?'Aug':(9==m)?'Sep':
    (10==m)?'Oct':(11==m)?'Nov':'Dec';
    
    return "" +
    (d<10?"0"+d:d) + " " + mmm + " " + y;
}


//
// get last modified date of the
// current document.
//
function date_lastmodified()
{
    var lmd = document.lastModified;
    var s   = "Unknown";
    var d1;
    
    // check if we have a valid date
    // before proceeding
    if(0 != (d1=Date.parse(lmd)))
    {
        s = "" + date_ddmmmyyyy(new Date(d1));
    }
    
    return s;
}

//document.writeln( date_lastmodified() );

// End

function GetRequest() {
    var url = location.search; //获取url中"?"符后的字串
    var theRequest = new Object();
    if (url.indexOf("?") != -1) {
        var str = url.substr(1);
        strs = str.split("&");
        for(var i = 0; i < strs.length; i ++) {
            theRequest[strs[i].split("=")[0]]=unescape(strs[i].split("=")[1]);
        }
    }
    return theRequest; 
}

var tplEngine = function(tpl, data) {
    var re = /{{(.+?)}}/g, 
        cursor = 0
        reExp = /(^( )?(var|if|for|else|switch|case|break|{|}|;))(.*)?/g,    
        code = 'var r=[];\n';

    // 解析html
    function parsehtml(line) {
        // 单双引号转义，换行符替换为空格,去掉前后的空格
        line = line.replace(/('|")/g, '\\$1').replace(/\n/g, ' ');//.replace(/(^\s+)|(\s+$)/g,"");
        code +='r.push("' + line + '");\n';
    }
    
    // 解析js代码        
    function parsejs(line) {   
        // 去掉前后的空格
        line = line.replace(/(^\s+)|(\s+$)/g,"");
        code += line.match(reExp)? line + '\n' : 'r.push(' + line + ');\n';
    }    
    
    while((match = re.exec(tpl))!== null) {
        // 开始标签  {{ 前的内容和结束标签 }} 后的内容
        parsehtml(tpl.slice(cursor, match.index))
        // 开始标签  {{ 和 结束标签 }} 之间的内容
        parsejs(match[1])
        // 每一次匹配完成移动指针
        cursor = match.index + match[0].length;
    }
    // 最后一次匹配完的内容
    parsehtml(tpl.substr(cursor, tpl.length - cursor));
    code += 'return r.join("");';
    return new Function(code.replace(/[\r\t\n]/g, '')).apply(data);
}

var mdDebug = function(){
    var request = GetRequest();
    console.log(request);
    for (var type in request) {
        if (type = "doc") {
            htmlobj = $.ajax({url:request[type],async:false});
            html = marked(htmlobj.responseText);
            console.log(html);
            break;
        }
    }
}

var mdRefresh = function(){
    var request = GetRequest();
    for (var type in request) {
        if (type = "doc") {
            htmlobj = $.ajax({
                url:request[type],
                beforeSend :function(xmlHttp){ 
                    xmlHttp.setRequestHeader("If-Modified-Since","0"); 
                    xmlHttp.setRequestHeader("Cache-Control","no-cache");
                },
                cache:false, 
                ifModified :true,
                async:false
            });
            var htmlstr = marked(htmlobj.responseText);
            document.getElementById("content").innerHTML = htmlstr;
            break;
        }
    }
}

var ulBlock = function()
{
    // 特殊样式：水平横排进行布置，垂直太长了向下滑动太烦了！
    var h1 = document.getElementsByTagName('h1');
    var ul = document.getElementsByTagName('ul');

    var count = ul.length;
    var htmlctt = "";
    for(var i = 0; i < count; i++)
    {
        var html = "<div style='float: left; padding: 5px;'>";
        html += "<h1>" + h1[i].innerHTML + "</h1>";
        html += "<hr>";
        html += "<ul>" + ul[i].innerHTML + "</ul>";
        html += "</div>"
        htmlctt += html;
    }
    document.getElementById('content').innerHTML = htmlctt;
}