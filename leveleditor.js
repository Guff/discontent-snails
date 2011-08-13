var canvas;
var ctx;
var textures;
var level;
var selection = [];
var imageRect = null;
var selectionColor = "rgba(180, 40, 40, 0.5)";
var borderColor = "rgba(160, 20, 20, 0.65)";

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
    window.onkeydown = onKeyDown;
    canvas.onmousedown = function (e) { onMouseDown(level, e); };
    canvas.onmouseup = onMouseUp;
}

function getBoundingBoxes(level) {
    boundingBoxes = new Array();
    
    boundingBoxes[0] = { type: "slingshot", body: level.slingshot,
                         x: level.slingshot.x - 7, y: level.slingshot.y - 40,
                         w: 14, h: 40 };
    for (var i = 0; i < level.obstacles.length; i++) {
        obstacle = level.obstacles[i];
        boundingBoxes.push({ type: "obstacle", body: obstacle,
                             x: obstacle.x - 20, y: obstacle.y - 5,
                             w: 40, h: 10 });
    }
    for (var i = 0; i < level.enemies.length; i++) {
        enemy = level.enemies[i];
        boundingBoxes.push({ type: "enemy", body: enemy, x: enemy.x - 25,
                             y: enemy.y - 25,
                             w: 50, h: 50 });
    }
    return boundingBoxes;
}

function mouseOverBody(body, e) {
    var x = e.pageX - canvas.offsetLeft;
    var y = e.pageY - canvas.offsetTop;
    var bx0 = body.x, bx1 = body.x + body.w;
    var by0 = body.y, by1 = body.y + body.h;
    if (x >= bx0 && x <= bx1 && y >= by0 && y <= by1)
        return [body, { x: bx1 - x, y: by1 - y }];
    else
        return [null, null];
}

function onMouseMove(body, pos, e) {
    body.body.x = e.pageX - canvas.offsetLeft;
    body.body.y = e.pageY - canvas.offsetTop;
    switch (body.type) {
        case "slingshot":
            body.body.x += pos.x - 7;
            body.body.y += pos.y;
            break;
        case "obstacle":    
            body.body.x += pos.x - body.w + 20;
            body.body.y += pos.y - body.h + 5;
            break;
        case "enemy":
            body.body.x += pos.x - body.w + 25;
            body.body.y += pos.y - body.h + 25;
            break;
        default:
            break;
    }
    drawLevel(ctx, level);
}

function onMouseDown(level, e) {
    var body, pos;
    var boundingBoxes = getBoundingBoxes(level);
    for (i = 0; i < boundingBoxes.length; i++) {
        var ret = mouseOverBody(boundingBoxes[i], e);
        body = ret[0];
        pos = ret[1];
        if (body) break;
    }
    if (!body) {
        if (!e.ctrlKey)
            selection = [];
        drawLevel(ctx, level);
        pos = { x: e.pageX - canvas.offsetLeft, y: e.pageY - canvas.offsetTop };
        canvas.onmousemove = function (e) { beginSelection(pos, e); };
        canvas.onmouseup = function (e) { endSelection(pos, e); };
        return;
    }
    if (e.ctrlKey)
        selection.push(body);
    else if (e.shiftKey) {
        if (selection.indexOf(body) != -1)
            selection.pop(selection.indexOf(body));
        else
            selection.push(body);
    } else
        selection = [body];
        
    drawLevel(ctx, level);
    
    canvas.onmousemove = function (e) { onMouseMove(body, pos, e); };
    canvas.onmouseup = onMouseUp;
}

function onMouseUp(e) {
    canvas.onmousemove = null;
}

function onKeyDown(e) {
    switch (e.keyCode) {
        case 68: // d
            selection.forEach(duplicateSelected);
            drawLevel(ctx, level);
            break;
        case 46: // delete
            selection.forEach(deleteSelected);
            break;
        default:
            break;
    }
}

function beginSelection(pos, e) {
    var x = e.pageX - canvas.offsetLeft, y = e.pageY - canvas.offsetTop;
    drawLevel(ctx, level);
    ctx.fillStyle = selectionColor;
    ctx.fillRect(Math.min(pos.x, x), Math.min(pos.y, y), Math.abs(x - pos.x),
                 Math.abs(y - pos.y));
    ctx.strokeStyle = borderColor;
    ctx.lineWidth = 2;
    ctx.strokeRect(Math.min(pos.x, x), Math.min(pos.y, y), Math.abs(x - pos.x),
                   Math.abs(y - pos.y));
}

function endSelection(pos, e) {
    drawLevel(ctx, level);
    imageRect = null;
    canvas.onmousemove = null;
    selection = [];
}

function deleteSelected(body) {
    if (body) {
        if (body.type == "obstacle")
            level.obstacles.pop(level.obstacles.indexOf(body.body));
        if (body.type == "enemy")
            level.enemies.pop(level.enemies.indexOf(body.body));
        
        selection = [];
        drawLevel(ctx, level);
    }
}

function duplicateSelected(body) {
    if (body) {
        if (body.type == "obstacle")
            level.obstacles.push(clone(body.body));
        else if (body.type == "enemy")
            level.enemies.push(clone(body.body));
    }
    selection = [];
    drawLevel(ctx, level);
}

function bodyInSelection(body) {
    for (var i = 0; i < selection.length; i++) {
        if (body == selection[i].body)
            return true;
    }
    return false;
}

function drawLevel(ctx, level) {
    document.levelJSON.levelJSONtext.value = JSON.stringify(level, null, 4);
    ctx.drawImage(textures.bg, 0, 0);
    ctx.drawImage(textures.ground, 0, 430);
    drawSlingshot(ctx, level.slingshot);
    level.obstacles.forEach(function (body) { drawBody(ctx, body); });
    level.enemies.forEach(function (enemy) { drawEnemy(ctx, enemy); });
}

function drawSlingshot(ctx, slingshot) {
    ctx.save();
    ctx.translate(slingshot.x, slingshot.y);
    ctx.scale(1 / 3, 1 / 3);
    ctx.drawImage(textures.slingshot, -20, -120);
    if (bodyInSelection(slingshot)) {
        ctx.fillStyle = selectionColor;
        ctx.fillRect(-20, -120, 40, 120);
        ctx.strokeStyle = borderColor;
        ctx.lineWidth = 2;
        ctx.strokeRect(-20, -120, 40, 120);
    }
    ctx.restore();
}

function drawEnemy(ctx, enemy) {
    ctx.save();
    ctx.translate(enemy.x, enemy.y);
    ctx.fillStyle = "#ffffff";
    ctx.fillRect(-25, -25, 50, 50);
    if (bodyInSelection(enemy)) {
        ctx.fillStyle = selectionColor;
        ctx.fillRect(-25, -25, 50, 50);
        ctx.strokeStyle = borderColor;
        ctx.lineWidth = 2;
        ctx.strokeRect(-25, -25, 50, 50);
    }
    ctx.restore();
}

function drawBody(ctx, body) {
    var angle = body.angle / 180 * Math.PI;
    ctx.save();
    ctx.translate(body.x, body.y);
    ctx.rotate(angle);
    ctx.scale(0.4, 0.4);
    ctx.drawImage(textures.block, -50, -12.5);
    if (bodyInSelection(body)) {
        ctx.fillStyle = selectionColor;
        ctx.fillRect(-50, -12.5, textures.block.width, textures.block.height);
        ctx.strokeStyle = borderColor;
        ctx.lineWidth = 2;
        ctx.strokeRect(-50, -12.5, textures.block.width, textures.block.height);
    }
    ctx.restore();
}

function clone(obj) {
    // Handle the 3 simple types, and null or undefined
    if (null == obj || "object" != typeof obj) return obj;

    // Handle Array
    if (obj instanceof Array) {
        var copy = [];
        for (var i = 0, len = obj.length; i < len; ++i) {
            copy[i] = clone(obj[i]);
        }
        return copy;
    }

    // Handle Object
    if (obj instanceof Object) {
        var copy = {};
        for (var attr in obj) {
            if (obj.hasOwnProperty(attr)) copy[attr] = clone(obj[attr]);
        }
        return copy;
    }
}
