var canvas;
var ctx;
var textures;

function initEditor() {
    canvas = document.getElementById("leveleditor");
    ctx = canvas.getContext("2d");
    
    textures = { block: new Image(), slingshot: new Image(), bg: new Image(),
                 ground: new Image(), snail: new Image() };
    textures.block.src = "data/wooden-block-40x10.png";
    textures.slingshot.src = "data/slingshot.png";
    textures.bg.src = "data/sky-bg.png";
    textures.ground.src = "data/ground.png";
    textures.snail.src = "data/snail-normal.png";
    drawLevel(ctx);
}

function drawLevel(ctx) {
    var level = JSON.parse(document.levelJSON.levelJSONtext.value);
    ctx.drawImage(textures.bg, 0, 0);
    ctx.drawImage(textures.ground, 0, 430);
    drawSlingshot(ctx, level.slingshot);
    level.obstacles.forEach(function (body) { drawBody(ctx, body) });
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
