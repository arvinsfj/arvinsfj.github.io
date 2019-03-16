**2017年个人计划**
======================================

>*作者：arvin 日期：2017年2月10日*

-----------

>BEGIN

#### 生活目标

努力挣钱，2017年买房娶妻。

#### 工作目标

1. 打造自己的产品
2. 转型后端（c/c++、python、go、swift、javascript）

#### 学习目标

1. 研究人工智能
2. 研究操作系统
3. 研究3D游戏引擎

>END

-----------

>Best Of The Best.


#### 模版引擎测试
-----------

<div id="ctt"></div>
<div id="ctt0"></div>     
<div id="ctt1"></div>        
<script type="text/tpl" id="template">
    <p>name: {{this.name}}</p>
    <p>age: {{this.profile.age}}</p>
    {{if (this.sex) {}}
        <p>sex: {{this.sex}}</p>
    {{}}}
    <ul>
        {{for(var i in this.skills){}}
        <li>{{this.skills[i]}}</li>
        {{}}}
    </ul>
</script>
        
<script type="text/javascript">

    var simpleEngine = function(str, data) {
        //获取元素
        var element = document.getElementById(str);
        if (element) {
            //textarea或input则取value，其它情况取innerHTML
            var html = /^(textarea|input)$/i.test(element.nodeName) ? element.value : element.innerHTML;
            return tplEngine(html, data);
        } else {
            //是模板字符串，则生成一个函数
            //如果直接传入字符串作为模板，则可能变化过多，因此不考虑缓存
            return tplEngine(str, data);
        }
    };
    
    var tpl = document.getElementById("template").innerHTML.toString();
    document.getElementById("ctt").innerHTML = tplEngine(tpl,{
        name: "arvin",
        profile: { 
            age: 32
        },
        sex: 'man',
        skills: ['html5','javascript','ios']
    });

    document.getElementById("ctt0").innerHTML = simpleEngine("template",{
        name: "arvin1",
        profile: { 
            age: 32
        },
        sex: 'man',
        skills: ['html5','javascript','ios']
    });
</script>

<script type="text/javascript">
    const template = data => `
    <p>name: ${data.name}</p>
    <p>age: ${data.profile.age}</p>
    <ul>
        ${data.skills.map(skill => `
        <li>${skill}</li>
        `).join('')}
    </ul>`;

    const data = {
    name: 'teser',
    profile: { age: 22 },
    skills: ['H5', 'js', 'android']
    };

    document.getElementById("ctt1").innerHTML = template(data);
</script>
