var canvas;
var ctx;
var textures;
var level;

function initEditor() {
    canvas = document.getElementById("leveleditor");
    ctx = canvas.getContext("2d");
    
    textures = { block: new Image() };
    textures.block.src = "data/wooden-block-40x10.png";
    drawLevel(ctx);
}

function canvasToCp(pos) {
    return { x: pos.x - canvas.width / 2, y: pos.y - canvas.height / 2 };
}

function cpToCanvas(pos) {
    return { x: pos.x + canvas.width / 2, y: pos.y + canvas.height / 2 };
}

function drawLevel(ctx) {
    var level = JSON.parse(document.levelJSON.levelJSONtext.value);
    ctx.fillStyle = "#000000";
    ctx.rect(0, 0, canvas.width, canvas.height);
    ctx.fill();
    level.obstacles.forEach(function (body) { drawBody(ctx, body) });
}

function drawBody(ctx, body) {
    ctx.fillStyle = "#ffffff";
    var pos = cpToCanvas({ x: body.x, y: body.y });
    var angle = body.angle / 180 * Math.PI;
    ctx.save();
    ctx.translate(pos.x, pos.y);
    ctx.rotate(angle);
    ctx.scale(0.4, 0.4);
    ctx.drawImage(textures.block, -50, -12.5);
    ctx.restore();
}
