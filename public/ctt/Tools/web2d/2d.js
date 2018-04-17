
/**
 *比较两个浮点型数字是否相等
 *
 * @param  srcFloat 源数据
 * @param  desFloat 目标数据
 *
 */
function compare(srcFloat, desFloat) {
    if (Math.abs(srcFloat-desFloat) < 0.1) {
        return true
    } else {
        return false
    }
}

/**
 *点到直线的垂点
 *
 * @param  point   点
 * @param  pointA   A点
 * @param  pointB   B点
 *
 */
function lineVerticalPoint(point, pointA, pointB) {
    
    var x = parseFloat(point.x)
    var y = parseFloat(point.y)
    var x1 = parseFloat(pointA.x)
    var y1 = parseFloat(pointA.y)
    var x2 = parseFloat(pointB.x)
    var y2 = parseFloat(pointB.y)
    
    var cross =(x2-x1)*(x-x1)+(y2-y1)*(y-y1)
    var d2 = (x2-x1)*(x2-x1) + (y2-y1)*(y2-y1)
    
    var r = cross/d2
    var px = x1+ (x2-x1)*r
    var py = y1+(y2-y1)*r
    
    result = {
        x : px,
        y : py
    }
    return result
}


/**
 *点是否在在两个点所在的线段上(包含起点和终点)
 *
 * @param  point  点
 * @param  pointA  A点
 * @param  pointB  B点
 *
 *@return  YES：在 NO:不在
 */
function pointInlineContainStartAndEnd(point, pointA, pointB) {
    point = {
        x : parseFloat(point.x),
        y : parseFloat(point.y)
    }
    pointA = {
        x : parseFloat(pointA.x),
        y : parseFloat(pointA.y)
    }
    pointB = {
        x : parseFloat(pointB.x),
        y : parseFloat(pointB.y)
    }
    var tempAC = Math.sqrt((pointA.x-point.x)*(pointA.x-point.x)+(pointA.y-point.y)*(pointA.y-point.y))
    var tempBC = Math.sqrt((pointB.x-point.x)*(pointB.x-point.x)+(pointB.y-point.y)*(pointB.y-point.y))
    var tempAB = Math.sqrt((pointB.x-pointA.x)*(pointB.x-pointA.x)+(pointB.y-pointA.y)*(pointB.y-pointA.y))
    
    if (compare(tempAC+tempBC, tempAB)) {
        return true
    } else {
        return false
    }
}

/**
 *  弧形墙控制点
 *
 *  @param startPos 弧形墙的起点
 *  @param linePos  弧形墙线上的点
 *  @param endPos   弧形墙的终点
 *
 */
function getControlPos(startPos, linePos, endPos) {
    // 从linePos做垂直于startPos和endPos组成的线段做垂线.
    var verticalPos = lineVerticalPoint(linePos, startPos, endPos)
    // linePos到垂点的中点,作为画弧形墙的线上的点
    var middle_linePosToVerticalPos = {
        x : (linePos.x + verticalPos.x) * 0.5,
        y : (linePos.y + verticalPos.y) * 0.5
    }
    // 根据计算公式求出控制点
    var controlPos = {
        x : 2 * middle_linePosToVerticalPos.x - 0.5 * startPos.x - 0.5 * endPos.x,
        y : 2 * middle_linePosToVerticalPos.y - 0.5 * startPos.y - 0.5 * endPos.y
    }
    return controlPos
}

/**
 *获得两条直线的交点
 *
 * @param  line1  直线1
 * @param  lone2   直线2
 *
 */
function intersectingPoint(lineStartPos, lineEndPos, otherLineStartPos, otherLineEndPos) {
    var x0 = parseFloat(lineStartPos.x)
    var y0 = parseFloat(lineStartPos.y)
    var x1 = parseFloat(lineEndPos.x)
    var y1 = parseFloat(lineEndPos.y)
    var x2 = parseFloat(otherLineStartPos.x)
    var y2 = parseFloat(otherLineStartPos.y)
    var x3 = parseFloat(otherLineEndPos.x)
    var y3 = parseFloat(otherLineEndPos.y)
    
    if (compare(y3, y2)) {
        if (compare(y0, y1)) {
            return {
            x: 0,
            y: 0
            }
        }
    }
    
    if (compare(x3, x2)) {
        if (compare(x0, x1)) {
            return {
                x : 0,
                y : 0
            }
        }
    }
    
    if (compare(y3, y2) === false && compare(x3, x2) === false) {
        var temp1 = (x1-x0)*(y3-y2)
        var temp2 = (y1-y0)*(x3-x2)
        if (compare(temp1, temp2)) {
            return {
                x : 0,
                y : 0
            }
        }
    }
    var kZero_flag1 = compare(x0, x1)
    var kZero_flag2 = compare(x2, x3)
    var x = 0
    var y = 0
    var k1 = 0
    var k2 = 0
    if (kZero_flag1) {
        k2 =(y2-y3)/(x2-x3)
        x = x0
        y = k2*(x0-x2)+y2
    } else if (kZero_flag2) {
        k1 = (y0-y1)/(x0-x1)
        x= x2
        y = k1*(x2-x0)+y0
    } else {
        k1 = (y0-y1)/(x0-x1)
        k2 = (y2-y3)/(x2-x3)
        x= (k1*x0-k2*x2+y2-y0)/(k1-k2)
        y = y0+(x-x0)*k1
    }
    return {
        x : x,
        y : y
    }
}


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
    this.settings = this.jsonObject.settings
    this.corners = this.jsonObject.corners
    this.walls = this.jsonObject.walls
    // this.texts = this.jsonObject.texts
    this.boxes = this.jsonObject.boxes
    this.rooms = this.jsonObject.rooms
    this.openings = this.jsonObject.openings
    this.out_walls = this.jsonObject.out_walls
    
    // 获取比例
    this._getScales = function () {
        // 找出户型中所有点的最大值和最小值
        var minPointX = Infinity
        var minPointY = Infinity
        var maxPointX = -Infinity
        var maxPointY = -Infinity
        for (var i = 0; i < this.corners.length; i++) {
            
            var corner = this.corners[i]
            var cornerX = parseFloat(corner.x)
            var cornerY = parseFloat(corner.y)
            
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
        
        // 控制点 (遍历所有的内墙，找出内墙的控制点)
        for (var i = 0; i < this.walls.length; i++) {
            var wall = this.walls[i]
            if (parseInt(wall.isArcwall) === 1) {
                var arcControlPoint = {
                    x : parseFloat(wall.arcControlPoint.x),
                    y : parseFloat(wall.arcControlPoint.y)
                }
                if (arcControlPoint.x < minPointX) {
                    minPointX = arcControlPoint.x
                }
                if (arcControlPoint.y < minPointY) {
                    minPointY = arcControlPoint.y
                }
                if (arcControlPoint.x > maxPointX) {
                    maxPointX = arcControlPoint.x
                }
                if (arcControlPoint.y > maxPointY) {
                    maxPointY = arcControlPoint.y
                }
            }
        }
        
        // 缩放比例
        var dis = (maxPointX - minPointX) > (maxPointY - minPointY) ? (maxPointX - minPointX) : (maxPointY - minPointY)
        var add = (parseInt(this.settings.type) === 0) ? 0.2 * this.width : 0.4 * this.width
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
    
    
    // 绘制内墙
    this.renderWalls = function (color) {
        var {scale, scale_x_dis, scale_y_dis} = this._getScales()
        // 绘制内墙
        let wall_ctx = new Graphics();
        wall_ctx.lineStyle(1, color, 1);
        for (var i = 0; i < this.walls.length; i++) {
            // 找出内墙对应的起始点和结束点
            var start = null
            var end = null
            var wall = this.walls[i]
            var length = (parseInt(this.settings.type) == 0) ? parseFloat(wall.length) : (parseFloat(wall.length)/parseFloat(this.settings.unitScale))
            for (var j = 0; j < this.corners.length; j++) {
                var corner = this.corners[j]
                if (wall.startCornerName === corner.name) {
                    start = corner
                }
                if ( wall.stopCornerName === corner.name) {
                    end = corner
                }
                if (start !== null && end !== null) {
                    break
                }
            }
            
            if (wall.isArcwall === undefined || parseInt(wall.isArcwall) === 0) {
                wall_ctx.moveTo(start.x/scale - scale_x_dis, start.y/scale - scale_y_dis)
                wall_ctx.lineTo(end.x/scale - scale_x_dis, end.y/scale - scale_y_dis)
            } else {
                start = {
                    x : parseFloat(start.x),
                    y : parseFloat(start.y)
                }
                end = {
                    x : parseFloat(end.x),
                    y : parseFloat(end.y)
                }
                var arcControlPoint = {
                    x : parseFloat(wall.arcControlPoint.x),
                    y : parseFloat(wall.arcControlPoint.y)
                }
                var controlPos = getControlPos(start, arcControlPoint, end)
                wall_ctx.moveTo(start.x/scale - scale_x_dis, start.y/scale - scale_y_dis)
                wall_ctx.quadraticCurveTo(controlPos.x/scale - scale_x_dis, controlPos.y/scale - scale_y_dis, end.x/scale - scale_x_dis, end.y/scale - scale_y_dis)
            }
        }
        this.app.stage.addChild(wall_ctx);
    }
    
    // 绘制外墙
    this.renderOutWalls = function (color) {
        // 绘制外墙
        var {scale, scale_x_dis, scale_y_dis} = this._getScales()
        let outwall_ctx = new Graphics();
        outwall_ctx.lineStyle(1, color, 1);
        for (var i = 0; i < this.out_walls.length; i++) {
            var arcControlPoint = null  // 正常的墙
            var out_wall = this.out_walls[i]
            var out_wall_start = {
                x : parseFloat(out_wall.startPoint.x),
            y: parseFloat(out_wall.startPoint.y)
            }
            var out_wall_end =  {
            x: parseFloat(out_wall.endPoint.x),
            y: parseFloat(out_wall.endPoint.y)
            }
            // 找出外墙对应的内墙
            for (var j = 0; j < this.walls.length; j++) {
                var wall = this.walls[j]
                var wall_start = null
                var wall_end = null
                for (var k = 0; k < this.corners.length; k++) {
                    var corner = this.corners[k]
                    if (wall.startCornerName == corner.name) {
                        wall_start = {
                            x : parseFloat(corner.x),
                            y : parseFloat(corner.y)
                        }
                    }
                    if (wall.stopCornerName == corner.name) {
                        wall_end = {
                            x : parseFloat(corner.x),
                            y : parseFloat(corner.y)
                        }
                    }
                }
                // 判断墙是否平行
                var result = intersectingPoint(out_wall_start, out_wall_end, wall_start, wall_end)
                if (result.x == 0 && result.y == 0) {
                    var wall_center = {
                    x: (parseFloat(wall_start.x) + parseFloat(wall_end.x)) * 0.5,
                    y: (parseFloat(wall_start.y) + parseFloat(wall_end.y)) * 0.5
                    }
                    var out_wall_center = {
                        x : (parseFloat(out_wall_start.x) + parseFloat(out_wall_end.x)) * 0.5,
                        y : (parseFloat(out_wall_start.y) + parseFloat(out_wall_end.y)) * 0.5
                    }
                    var wall_pos = lineVerticalPoint(wall_center, out_wall_end, out_wall_start)
                    var out_wall_pos = lineVerticalPoint(out_wall_center, wall_end, wall_start)
                    var wall_pos_res = pointInlineContainStartAndEnd(wall_pos, out_wall_end, out_wall_start)
                    var out_wall_pos_res = pointInlineContainStartAndEnd(out_wall_pos, wall_end, wall_start)
                    
                    // var disw = Math.sqrt((wall_center.x-wall_pos.x)*(wall_center.x-wall_pos.x)+(wall_center.y-wall_pos.y)*(wall_center.y-wall_pos.y))
                    // console.log(i,wall_start,wall_end, out_wall_end, out_wall_start, wall_pos,  disw);
                    
                    if (wall_pos_res || out_wall_pos_res) {
                        var dis = Math.sqrt((wall_center.x-wall_pos.x)*(wall_center.x-wall_pos.x)+(wall_center.y-wall_pos.y)*(wall_center.y-wall_pos.y))
                        var length = (parseInt(this.settings.type) == 0) ? parseFloat(wall.length) : (parseFloat(wall.length)/parseFloat(this.settings.unitScale))
                        // console.log(i,wall_start,wall_end, out_wall_end, out_wall_start, dis);
                        if (Math.abs(dis - length) < 0.5) {
                            // console.log(i,wall_start,wall_end, out_wall_end, out_wall_start, dis);
                            
                            if (wall.isArcwall != undefined && parseInt(wall.isArcwall) === 1) {
                                arcControlPoint = {
                                    x : parseFloat(wall.arcControlPoint.x),
                                    y : parseFloat(wall.arcControlPoint.y)
                                }
                            }
                        }
                    }
                }
            }
            
            if (arcControlPoint === null) {
                outwall_ctx.moveTo(out_wall_start.x/scale - scale_x_dis, out_wall_start.y/scale - scale_y_dis)
                outwall_ctx.lineTo(out_wall_end.x/scale - scale_x_dis, out_wall_end.y/scale - scale_y_dis)
            } else {
                var controlPos = getControlPos(out_wall_start, arcControlPoint, out_wall_end)
                outwall_ctx.moveTo(out_wall_start.x/scale - scale_x_dis, out_wall_start.y/scale - scale_y_dis)
                outwall_ctx.quadraticCurveTo(controlPos.x/scale - scale_x_dis, controlPos.y/scale - scale_y_dis, out_wall_end.x/scale - scale_x_dis, out_wall_end.y/scale - scale_y_dis)
            }
        }
        this.app.stage.addChild(outwall_ctx);
    }
    
    // 绘制门洞
    this.renderDoors = function (color) {
        var {scale, scale_x_dis, scale_y_dis} = this._getScales()
        // 绘制openings
        let opening_ctx = new Graphics();
        opening_ctx.lineStyle(1, color, 1);
        for (var i = 0; i < this.openings.length; i++) {
            var firstPt = this.openings[i].points[0]
            var secondPt = this.openings[i].points[1]
            var thirdPt = this.openings[i].points[2]
            var fourPt = this.openings[i].points[3]
            var fivePt = this.openings[i].points[4]
            var sixPt = this.openings[i].points[5]
            var sevenPt = this.openings[i].points[6]
            var eightPt = this.openings[i].points[7]
            
            if (parseInt(this.openings[i].openingType) == 1003) { // 门洞
                // 画4周
                opening_ctx.moveTo(firstPt.x/scale - scale_x_dis, firstPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(secondPt.x/scale - scale_x_dis, secondPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(sixPt.x/scale - scale_x_dis, sixPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(fivePt.x/scale - scale_x_dis, fivePt.y/scale - scale_y_dis)
                opening_ctx.lineTo(firstPt.x/scale - scale_x_dis, firstPt.y/scale - scale_y_dis)
                // 画内部线条
                opening_ctx.moveTo(thirdPt.x/scale - scale_x_dis, thirdPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(fourPt.x/scale - scale_x_dis, fourPt.y/scale - scale_y_dis)
            }
        }
        this.app.stage.addChild(opening_ctx);
    }
    
    // 绘制直窗
    this.renderStraightWindows = function (color) {
        var {scale, scale_x_dis, scale_y_dis} = this._getScales()
        // 绘制openings
        let opening_ctx = new Graphics();
        opening_ctx.lineStyle(1, color, 1);
        for (var i = 0; i < this.openings.length; i++) {
            var firstPt = this.openings[i].points[0]
            var secondPt = this.openings[i].points[1]
            var thirdPt = this.openings[i].points[2]
            var fourPt = this.openings[i].points[3]
            var fivePt = this.openings[i].points[4]
            var sixPt = this.openings[i].points[5]
            var sevenPt = this.openings[i].points[6]
            var eightPt = this.openings[i].points[7]
            
            if (parseInt(this.openings[i].openingType) == 1001) {  // 直窗
                
                // 画4周
                opening_ctx.moveTo(firstPt.x/scale - scale_x_dis, firstPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(secondPt.x/scale - scale_x_dis, secondPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(sixPt.x/scale - scale_x_dis, sixPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(fivePt.x/scale - scale_x_dis, fivePt.y/scale - scale_y_dis)
                opening_ctx.lineTo(firstPt.x/scale - scale_x_dis, firstPt.y/scale - scale_y_dis)
                
                // 画内部线条
                opening_ctx.moveTo(thirdPt.x/scale - scale_x_dis, thirdPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(fourPt.x/scale - scale_x_dis, fourPt.y/scale - scale_y_dis)
            }
        }
        this.app.stage.addChild(opening_ctx);
    }
    
    // 绘制飘窗
    this.renderBayWindows = function (color) {
        var {scale, scale_x_dis, scale_y_dis} = this._getScales()
        // 绘制openings
        let opening_ctx = new Graphics();
        opening_ctx.lineStyle(1, color, 1);
        for (var i = 0; i < this.openings.length; i++) {
            var firstPt = this.openings[i].points[0]
            var secondPt = this.openings[i].points[1]
            var thirdPt = this.openings[i].points[2]
            var fourPt = this.openings[i].points[3]
            var fivePt = this.openings[i].points[4]
            var sixPt = this.openings[i].points[5]
            var sevenPt = this.openings[i].points[6]
            var eightPt = this.openings[i].points[7]
            
            if (parseInt(this.openings[i].openingType) == 1002) {  // 飘窗
                
                // 画外圈
                opening_ctx.beginFill(this.bgColor);
                opening_ctx.moveTo(firstPt.x/scale - scale_x_dis, firstPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(fivePt.x/scale - scale_x_dis, fivePt.y/scale - scale_y_dis)
                opening_ctx.lineTo(sixPt.x/scale - scale_x_dis, sixPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(secondPt.x/scale - scale_x_dis, secondPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(firstPt.x/scale - scale_x_dis, firstPt.y/scale - scale_y_dis)
                opening_ctx.endFill();
                
                // 画内部线条
                opening_ctx.moveTo(thirdPt.x/scale - scale_x_dis, thirdPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(sevenPt.x/scale - scale_x_dis, sevenPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(eightPt.x/scale - scale_x_dis, eightPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(fourPt.x/scale - scale_x_dis, fourPt.y/scale - scale_y_dis)
            }
        }
        this.app.stage.addChild(opening_ctx);
    }
    
    // 绘制垭口
    this.renderSandBack = function (color) {
        var {scale, scale_x_dis, scale_y_dis} = this._getScales()
        // 绘制openings
        let opening_ctx = new Graphics();
        opening_ctx.lineStyle(1, color, 1);
        for (var i = 0; i < this.openings.length; i++) {
            var firstPt = this.openings[i].points[0]
            var secondPt = this.openings[i].points[1]
            var thirdPt = this.openings[i].points[2]
            var fourPt = this.openings[i].points[3]
            var fivePt = this.openings[i].points[4]
            var sixPt = this.openings[i].points[5]
            var sevenPt = this.openings[i].points[6]
            var eightPt = this.openings[i].points[7]
            
            if (parseInt(this.openings[i].openingType) == 1005) {  // 垭口
                
                // 画4周
                opening_ctx.moveTo(firstPt.x/scale - scale_x_dis, firstPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(secondPt.x/scale - scale_x_dis, secondPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(sixPt.x/scale - scale_x_dis, sixPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(fivePt.x/scale - scale_x_dis, fivePt.y/scale - scale_y_dis)
                opening_ctx.lineTo(firstPt.x/scale - scale_x_dis, firstPt.y/scale - scale_y_dis)
                
                // 画内部线条 0 5
                opening_ctx.moveTo(firstPt.x/scale - scale_x_dis, firstPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(sixPt.x/scale - scale_x_dis, sixPt.y/scale - scale_y_dis)
                
                // 画内部线条 1 4
                opening_ctx.moveTo(secondPt.x/scale - scale_x_dis, secondPt.y/scale - scale_y_dis)
                opening_ctx.lineTo(fivePt.x/scale - scale_x_dis, fivePt.y/scale - scale_y_dis)
            }
        }
        this.app.stage.addChild(opening_ctx);
    }
}



