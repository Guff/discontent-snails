var canvas;
var ctx;
var textures;
var level;
var selection = [];
var imageRect = null;
var selectionColor = "rgba(180, 40, 40, 0.4)";
var borderColor = "rgba(160, 20, 20, 0.525)";
var mode = null;
var history = [];
var stonePattern;
var viewport = { x: 0, y: 0, zoom: 1 };

function initEditor() {
    canvas = document.getElementById("leveleditor");
    ctx = canvas.getContext("2d");
    level = JSON.parse(document.levelJSON.levelJSONtext.value);
    
    //updateHistory();
    
    textures = { block: new Image(), slingshot: new Image(), bg: new Image(),
                 ground: new Image(), snail: new Image() };
    textures.block =     document.getElementById("block");
    textures.slingshot = document.getElementById("slingshot");
    textures.bg =        document.getElementById("bg");
    textures.ground =    document.getElementById("ground");
    textures.snail =     document.getElementById("snail");
    textures.stone =     document.getElementById("stone");
    stonePattern = ctx.createPattern(textures.stone, "repeat");
    drawLevel(ctx, level);
    document.addEventListener("keydown", onKeyDown, true);
    canvas.addEventListener("mousedown", function (e) { onMouseDown(level, e); }, true);
    canvas.addEventListener("mouseup", onMouseUp, false);
    canvas.addEventListener("DOMMouseScroll", onScroll, true);
}

//function updateHistory() {
    //if (history.indexOf(level) > -1)
        //history = history.slice(0, history.indexOf(level));
    //level = clone(level);
    //history.push(level);
//}

//function rewindHistory

function boxesIntersect(b0, b1) {
    if (b0.y + b0.h <= b1.y || b0.y >= b1.y + b1.h)
        return false;
    if (b0.x + b0.w <= b1.x || b0.x >= b1.x + b1.w)
        return false;
        
    return true;
}

function getBoundingBoxes(level) {
    var s = viewport.zoom;
    var dx = viewport.x, dy = viewport.y;
    boundingBoxes = new Array();
    
    boundingBoxes[0] = { type: "slingshot", body: level.slingshot,
                         x: s * (level.slingshot.x + dx - 7),
                         y: s * (level.slingshot.y + dy - 40),
                         w: s * 14, h: s * 40 };
    for (var i = 0; i < level.obstacles.length; i++) {
        obstacle = level.obstacles[i];
        var cos = Math.cos(deg2rad(obstacle.angle));
        var sin = Math.sin(deg2rad(obstacle.angle));
        var w = cos * 50 + sin * 10;
        var h = sin * 50 + cos * 10;
        boundingBoxes.push({ type: "obstacle", body: obstacle,
                             x: s *(obstacle.x + dx - w / 2),
                             y: s * (obstacle.y + dy - h / 2),
                             w: s * Math.abs(w), h: s * Math.abs(h) });
    }
    for (var i = 0; i < level.enemies.length; i++) {
        enemy = level.enemies[i];
        var cos =  Math.cos(deg2rad(enemy.angle));
        var sin = Math.sin(deg2rad(enemy.angle));
        var w = cos * 30 + sin * 30;
        var h = sin * 30 + cos * 30
        boundingBoxes.push({ type: "enemy", body: enemy,
                             x: s * (enemy.x + dx - w / 2),
                             y: s * (enemy.y + dy - h / 2),
                             w: s * w, h: s * h });
    }
    return boundingBoxes;
}

function getBodyIndex(body) {
    for (var i = 0; i < selection.length; i++) {
        var matches = true;
        for (k in selection[i]) {
            if (body[k] != selection[i][k]) {
                matches = false;
                break;
            }
        }
        if (matches)
            return i;
    }
    return -1;
}

function onScroll(e) {
    if (e.axis == 1) {
        if (e.shiftKey)
            viewport.y += e.detail * 5;
        else
            viewport.x = Math.max(viewport.x + e.detail * 5, 0);
    } else if (e.axis == 2)
        if (e.ctrlKey)
            viewport.zoom = Math.max(Math.min(viewport.zoom - e.detail / 20, 3), 1 / 3);
        else
            viewport.y -= e.detail * 5;
    
    drawLevel(ctx, level);
    
    e.preventDefault();
    e.stopPropagation();
    
    return false;
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
            //var cos = Math.cos(body.body.angle / 180 * Math.PI);
            //var sin = Math.sin(body.body.angle / 180 * Math.PI);
            //var w = cos * body.w + sin * body.h;
            //var h = sin * body.w + cos * body.w;
            //body.body.x += pos.x - w / 2;
            //body.body.y += pos.y - h / 2;
            break;
        case "enemy":
            body.body.x += pos.x - body.w + 15;
            body.body.y += pos.y - body.h + 15;
            break;
        default:
            break;
    }
    drawLevel(ctx, level);
}

function onMouseDown(level, e) {
    switch (mode) {
        case null:
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
                pos = { x: e.pageX - canvas.offsetLeft,
                        y: e.pageY - canvas.offsetTop };
                canvas.onmousemove = function (e) { beginSelection(pos, e); };
                canvas.onmouseup = function (e) { endSelection(pos, e); };
                canvas.onmouseout = canvas.onmouseup;
                return;
            }
            if (e.ctrlKey) {
                if (getBodyIndex(body) != -1)
                    selection.splice(getBodyIndex(body), 1);
                else
                    selection.push(body);
            } else if (e.shiftKey) {
                if (getBodyIndex(body) != -1)
                    selection.splice(getBodyIndex(body), 1);
                else
                    selection.push(body);
            } else
                selection = [body];
            
            drawLevel(ctx, level);
            
            canvas.onmousemove = function (e) { onMouseMove(body, pos, e); };
            canvas.onmouseup = onMouseUp;
        case "rotate":
    }
}

function onMouseUp(e) {
    canvas.onmousemove = null;
}

function onKeyDown(e) {
    if (document.activeElement != canvas)
        return true;
    // global key bindings
    switch (e.keyCode) {
        case 49: // 1
            viewport = { x: 0, y: 0, zoom: 1 };
            drawLevel(ctx, level);
            break;
        case 27: // escape
            mode = null;
            break;
        default:
            break;
    }
    
    // context-specific keybindings
    switch (mode) {
        case null:
            switch (e.keyCode) {
                case 68: // d
                    selection.forEach(duplicateSelected);
                    drawLevel(ctx, level);
                    break;
                case 82: // r
                    mode = "rot";
                    break;
                case 84: // t
                    mode = "terrain";
                    break;
                case 46: // delete
                    selection.forEach(deleteSelected);
                    drawLevel(ctx, level);
                    break;
                case 37: // left
                    selection.forEach(function (body) {
                        body.body.x -= e.shiftKey ? 10 : 1;
                    });
                    drawLevel(ctx, level);
                    break;
                case 39: // right
                    selection.forEach(function (body) {
                        body.body.x += e.shiftKey ? 10 : 1;
                    });
                    drawLevel(ctx, level);
                    break;
                case 38: // up
                    selection.forEach(function (body) {
                        body.body.y -= e.shiftKey ? 10 : 1;
                    });
                    drawLevel(ctx, level);
                    break;
                case 40: // down
                    selection.forEach(function (body) {
                        body.body.y += e.shiftKey ? 10 : 1;
                    });
                    drawLevel(ctx, level);
                    break;
                default:
                    //console.log(e.keyCode);
                    break;
            }
            break;
        case "rot":
            switch (e.keyCode) {
                case 37: // left
                    selection.forEach(function (body) {
                        body.body.angle -= e.shiftKey ? 10 : 1;
                    });
                    drawLevel(ctx, level);
                    break;
                case 39: // right
                    selection.forEach(function (body) {
                        body.body.angle += e.shiftKey ? 10 : 1;
                    });
                    drawLevel(ctx, level);
                    break;
                default:
                    break;
            }
        case "terrain":
            switch (e.keyCode) {
                default:
                    break;
            }
            break;
        default:
            break;
    }
    
    e.preventDefault();
    e.stopPropagation();
    
    return false;
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
    var x = Math.min(pos.x, e.pageX - canvas.offsetLeft),
        y = Math.min(pos.y, e.pageY - canvas.offsetTop),
        w = Math.abs(e.pageX - canvas.offsetLeft - pos.x),
        h = Math.abs(e.pageY - canvas.offsetTop - pos.y);
    var selectionBox = { x: x, y: y, w: w, h: h };
    
    selection = getBoundingBoxes(level).filter(function (box) {
        return boxesIntersect(selectionBox, box);
    });
    
    drawLevel(ctx, level);
    imageRect = null;
    canvas.onmousemove = null;
    canvas.onmouseout = null;
}

function deleteSelected(body) {
    if (body) {
        if (body.type == "obstacle")
            level.obstacles.splice(level.obstacles.indexOf(body.body), 1);
        if (body.type == "enemy")
            level.enemies.splice(level.enemies.indexOf(body.body), 1);
        selection = [];
    }
}

function duplicateSelected(body) {
    if (body) {
        if (body.type == "obstacle")
            level.obstacles.push(clone(body.body));
        else if (body.type == "enemy")
            level.enemies.push(clone(body.body));
    }
    //selection = [];
}

function updateLevel() {
    level = JSON.parse(document.levelJSON.levelJSONtext.value);
    drawLevel(ctx, level);
}

function drawLevel(ctx, level) {
    document.levelJSON.levelJSONtext.value = JSON.stringify(level, null, 4);
    ctx.save();
    ctx.translate(-viewport.x, viewport.y);
    ctx.scale(viewport.zoom, viewport.zoom);
    ctx.drawImage(textures.bg, 0, 0);
    ctx.drawImage(textures.ground, 0, 430);
    drawTerrain(ctx, level.terrain, stonePattern);
    drawSlingshot(ctx, level.slingshot);
    level.obstacles.forEach(function (body) { drawBody(ctx, body); });
    level.enemies.forEach(function (enemy) { drawEnemy(ctx, enemy); });
    drawSelection();
    ctx.restore();
}

function drawSelection() {
    ctx.beginPath();
    for (var i = 0; i < selection.length; i++) {
        ctx.save();
        ctx.translate(selection[i].body.x, selection[i].body.y);
        switch (selection[i].type) {
            case "slingshot":
                ctx.rect(-7, -40, 13, 40);
                break;
            case "enemy":
                ctx.rotate(deg2rad(selection[i].body.angle));
                ctx.rect(-15, -15, 30, 30);
                break;
            case "obstacle":
                ctx.rotate(deg2rad(selection[i].body.angle));
                ctx.rect(-25, -5, 50, 10);
                break;
            default:
                break;
        }
        ctx.restore();
    }
    
    ctx.fillStyle = selectionColor;
    ctx.strokeStyle = borderColor;
    ctx.lineWidth = 2;
    ctx.fill();
    ctx.stroke();
}

function drawSlingshot(ctx, slingshot) {
    ctx.save();
    ctx.translate(slingshot.x, slingshot.y);
    ctx.scale(1 / 3, 1 / 3);
    ctx.drawImage(textures.slingshot, -20, -120);
    ctx.restore();
}

function drawEnemy(ctx, enemy) {
    ctx.save();
    ctx.translate(enemy.x, enemy.y);
    ctx.rotate(deg2rad(enemy.angle));
    ctx.fillStyle = "#ffffff";
    ctx.fillRect(-15, -15, 30, 30);
    ctx.restore();
}

function drawBody(ctx, body) {
    ctx.save();
    ctx.translate(body.x, body.y);
    ctx.rotate(deg2rad(body.angle));
    ctx.scale(0.25, 0.25);
    ctx.drawImage(textures.block, -100, -20);
    ctx.restore();
}

function deg2rad(ang) {
    return ang / 180 * Math.PI;
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
