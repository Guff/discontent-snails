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
    textures.block = document.getElementById("block");
    textures.slingshot = document.getElementById("slingshot");
    textures.bg = document.getElementById("bg");
    textures.ground = document.getElementById("ground");
    textures.snail = document.getElementById("snail");
    drawLevel(ctx, level);
}

function getBoundingBoxes(level) {
    boundingBoxes = new Array();
    
    boundingBoxes[0] = { type: "slingshot", body: level.slingshot,
                         x: level.slingshot.x - 7, y: level.slingshot.y - 40,
                         w: 13, h: 40 };
    for (i = 0; i < level.obstacles.length; i++) {
        obstacle = level.obstacles[i];
        boundingBoxes.push({ type: "obstacle", body: obstacle,
                             x: obstacle.x - 20, y: obstacle.y - 5,
                             w: 40, h: 10 });
    }
    return boundingBoxes;
}

function mouseOverBody(body, e) {
    var x = e.pageX - canvas.offsetLeft;
    var y = e.pageY - canvas.offsetTop;
    var bx0 = body.x, bx1 = body.x + body.w;
    var by0 = body.y, by1 = body.y + body.h;
    if (x >= bx0 && x <= bx1 && y >= by0 && y <= by1) {
        return [body, { x: bx1 - x, y: by1 - y }];
    } else {
        return [null, null];
    }
}

function onMouseMove(body, pos, e) {
    body.body.x = e.pageX - canvas.offsetLeft;
    body.body.y = e.pageY - canvas.offsetTop;
    switch (body.type) {
        case "slingshot":
            body.body.x += pos.x;
            body.body.y += pos.y;
            break;
        case "obstacle":    
            body.body.x += pos.x - body.w + 20;
            body.body.y += pos.y - body.h + 5;
            break;
        default:
            break;
    }
    drawLevel(ctx, level);
    document.levelJSON.levelJSONtext.value = JSON.stringify(level);
}

function onMouseDown(level, e) {
    var body, pos;
    var boundingBoxes = getBoundingBoxes(level);
    for (i = 0; i < boundingBoxes.length; i++) {
        var ret = mouseOverBody(boundingBoxes[i], e);
        body = ret[0];
        pos = ret[1];
        if (body) {
            break;
        }
    }
    if (!body) {
        return;
    }
    
    canvas.onmousemove = function (e) { onMouseMove(body, pos, e); };
}

function onMouseUp(e) {
    canvas.onmousemove = null;
}

function drawLevel(ctx, level) {
    //alert(ctx);
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
