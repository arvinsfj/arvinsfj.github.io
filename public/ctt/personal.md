
# About Me
----------------------

Hello!

My name is Cheng Ze. You can call me Arvin. I was born in 1980s. I live in Shanghai, China with my soulmate. I work at aifuwo as an Project Manager (or Senior Software Engineer) on the iOS Development team. I love programing and am usually busy with side projects or software hacking. My favorite languages are Go and Python, but I have broad experience with many languages and technologies. Much of my computing interest is geared toward graphics, 3d render, emulator, os, hacking. In spare time, I like reading, gardening, 80's game, hacking and all kinds of music.

[Email me](mailto:arvin.sfj@gmail.com)

# Photo
----------------------

[here](#)

# External Links
----------------------

[Github](https://github.com/arvinsfj) And [Website](https://github.com/arvinsfj/arvinsfj.github.io)

# Thanks
----------------------

To [Vencent](http://blog.sina.com.cn/vincentgao0520) and [Eric Raymond](http://www.catb.org/~esr/), because you inspired me. [Others](detail.html?doc=sitemap.md). [QuickJS](https://bellard.org/quickjs/).


<!--采用模版技术-->

<div id="ext_info" style="float: right; border:2px dashed #E0E0E8; padding: 10px; margin: 10px;"></div>

<script type="text/tpl" id="person_tpl">
    <p>name: {{this.name}}</p>
    <p>borthday: {{this.profile.borthday}}</p>
    {{if (this.sex) {}}
        <p>sex: {{this.sex}}</p>
    {{}}}
    <ul>
        {{for(var i in this.contacts){}}
        <li>{{this.contacts[i]}}</li>
        {{}}}
    </ul>
</script>
        
<script type="text/javascript">
    
    var tpl = document.getElementById("person_tpl").innerHTML.toString();
    document.getElementById("ext_info").innerHTML = tplEngine(tpl,{
        name: "arvin",
        profile: { 
            borthday: "9.1987"
        },
        sex: 'man',
        contacts: ['https://arvinsfj.github.io/','https://github.com/arvinsfj/','arvin.sfj@gmail.com']
    });

</script>
