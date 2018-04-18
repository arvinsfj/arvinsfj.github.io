
/**
 * 创建渲染对象
 *
 * @param  width    画布宽度
 * @param  height   画布高度
 * @param  jsonObject  json对象，不包括（msg， code）
 * @param  bgColor   画布背景色
 *
 */
function Render2DObject(width, height, top, jsonObject, bgColor) {
    
    // 获取body
    this.body = document.getElementsByTagName('body')[0]
    this.body.style.backgroundColor = bgColor
    var top = top
    var bottom = 10
    this.bgColor = bgColor
    //获取绘制app
    let Application = PIXI.Application,
    Container = PIXI.Container,
    loader = PIXI.loader,
    resources = PIXI.loader.resources,
    Graphics = PIXI.Graphics,
    TextureCache = PIXI.utils.TextureCache,
    Sprite = PIXI.Sprite;
    //Create a Pixi Application
    this.app = new Application({ width: height-top-bottom, height: height-top-bottom, antialiasing: true, transparent: false, resolution: 1 , backgroundColor: bgColor} );
    //Add the canvas that Pixi automatically created for you to the HTML document
    document.body.appendChild(this.app.view);
    this.app.renderer.view.style.position = "absolute"
    this.app.renderer.view.style.marginLeft = (width - this.app.view.width) * 0.5 + "px";
    this.app.renderer.view.style.marginTop = top + "px";
    this.app.renderer.view.style.display = "block";
    this.width = this.app.view.width
    this.height = this.app.view.height
    // json数据
    this.jsonObject = jsonObject
    
    // 获取比例
    this._getScales = function () {
        // 找出户型中所有点的最大值和最小值
        var minPointX = Infinity
        var minPointY = Infinity
        var maxPointX = -Infinity
        var maxPointY = -Infinity
        
        for (var i = 0; i < this.jsonObject.length; i++) {
        	var sect = this.jsonObject[i].sector
        	
        	for (var j = 0; j < sect.length; j++) {
        	
        		var corner = sect[j]
            	var cornerX = corner.x
            	var cornerY = corner.y
            	// 过滤0点
            	if (cornerX <= 0 || cornerY <= 0) {
                	continue
            	}
            	if (cornerX < minPointX) {
                	minPointX = cornerX
            	}
            	if (cornerY < minPointY) {
                	minPointY = cornerY
           		}
           		if (cornerX > maxPointX) {
                	maxPointX = cornerX
            	}
            	if (cornerY > maxPointY) {
                	maxPointY = cornerY
            	}
        	}
        }
        
        // 缩放比例
        var dis = (maxPointX - minPointX) > (maxPointY - minPointY) ? (maxPointX - minPointX) : (maxPointY - minPointY)
        var add = 0.01 * this.width
        var scale = (dis + add) / this.width
        
        // 平移
        var scale_minPointX = minPointX/scale
        var scale_minPointY = minPointY/scale
        var scale_maxPointX = maxPointX/scale
        var scale_maxPointY = maxPointY/scale
        var scale_house_center = {
        x: (scale_maxPointX + scale_minPointX) * 0.5,
        y: (scale_maxPointY + scale_minPointY) * 0.5
        }
        var canvas_center = {
            x : this.width * 0.5,
            y : this.height * 0.5
        }
        var scale_x_dis = scale_house_center.x - canvas_center.x
        var scale_y_dis = scale_house_center.y - canvas_center.y
        
        return {
            scale : scale,
            scale_x_dis : scale_x_dis,
            scale_y_dis : scale_y_dis
        }
    }
    
    
    // 绘制墙
    this.renderWalls = function (color) {
        var {scale, scale_x_dis, scale_y_dis} = this._getScales()
        let wall_ctx = new Graphics();
        wall_ctx.lineStyle(1, color, 1);
        for (var i = 0; i < this.jsonObject.length; i++) {
        	var sect = this.jsonObject[i].sector
        	for (var j = 0; j < sect.length; j++) {
        		var start = sect[j]
            	var end = (j == sect.length-1) ? sect[0] : sect[j+1]
            	wall_ctx.moveTo(start.x/scale - scale_x_dis, start.y/scale - scale_y_dis)
            	wall_ctx.lineTo(end.x/scale - scale_x_dis, end.y/scale - scale_y_dis)
        	}
        }
        this.app.stage.addChild(wall_ctx);
    }
}



