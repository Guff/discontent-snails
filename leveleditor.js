var canvas;
var ctx;
var textures;
var level;

function initEditor() {
    canvas = document.getElementById("leveleditor");
    ctx = canvas.getContext("2d");
    
    level = JSON.parse(document.levelJSON.levelJSONtext.value);
    
    textures = { block: new Image(), slingshot: new Image(), bg: new Image(),
                 ground: new Image(), snail: new Image() };
    textures.block.src = "data/wooden-block-40x10.png";
    textures.slingshot.src = "data/slingshot.png";
    textures.bg.src = "data/sky-bg.png";
    textures.ground.src = "data/ground.png";
    textures.snail.src = "data/snail-normal.png";
    drawLevel(ctx, level);
}

function getBoundingBoxes(level) {
    boundingBoxes = new Array();
    
    boundingBoxes[0] = { body: level.slingshot, x: level.slingshot.x - 7,
                         y: level.slingshot.y - 40, w: 13, h: 40 };
    for (i = 0; i < level.obstacles.length; i++) {
        obstacle = level.obstacles[i];
        boundingBoxes.push({ body: obstacle, x: obstacle.x - 20,
                             y: obstacle.y - 5, w: 40, h: 10 });
    }
    return boundingBoxes;
}

function mouseOverBody(body, e) {
    var x = e.pageX - canvas.offsetLeft;
    var y = e.pageY - canvas.offsetTop;
    var bx0 = body.x, bx1 = body.x + body.w;
    var by0 = body.y, by1 = body.y + body.h;
    if (x >= bx0 && x <= bx1 && y >= by0 && y <= by1) {
        return body;
    } else {
        return null;
    }
}

function onMouseMove(body, e) {
    body.body.x = e.pageX - canvas.offsetLeft;
    body.body.y = e.pageY - canvas.offsetTop;
    drawLevel(ctx, level);
    document.levelJSON.levelJSONtext.value = JSON.stringify(level);
}

function onMouseDown(level, e) {
    var boundingBoxes = getBoundingBoxes(level);
    for (i = 0; i < boundingBoxes.length; i++) {
        var body = mouseOverBody(boundingBoxes[i], e);
        if (body) {
            break;
        }
    }
    if (!body) {
        return;
    }
    
    canvas.onmousemove = function (e) { onMouseMove(body, e); };
}

function onMouseUp(e) {
    canvas.onmousemove = null;
}

function drawLevel(ctx, level) {
    ctx.drawImage(textures.bg, 0, 0);
    ctx.drawImage(textures.ground, 0, 430);
    drawSlingshot(ctx, level.slingshot);
    level.obstacles.forEach(function (body) { drawBody(ctx, body); });
    canvas.onmousedown = function (e) { onMouseDown(level, e); };
    canvas.onmouseup = onMouseUp;
}

function drawSlingshot(ctx, slingshot) {
    ctx.save();
    ctx.translate(slingshot.x, slingshot.y);
    ctx.scale(1 / 3, 1 / 3);
    ctx.drawImage(textures.slingshot, -7, -120);
    ctx.restore();
}

function drawBody(ctx, body) {
    var angle = body.angle / 180 * Math.PI;
    ctx.save();
    ctx.translate(body.x, body.y);
    ctx.rotate(angle);
    ctx.scale(0.4, 0.4);
    ctx.drawImage(textures.block, -50, -12.5);
    ctx.restore();
}
