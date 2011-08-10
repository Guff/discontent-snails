var canvas;
var ctx;

function initEditor() {
    canvas = document.getElementById("leveleditor");
    ctx = canvas.getContext("2d");
    
    level = {
        "obstacles":
        [
            {
                "type": "block",
                "x": 200,
                "y": 235,
                "angle": 0
            },
            {
                "type": "block",
                "x": 105,
                "y": 220,
                "angle": 90
            },
            {
                "type": "block",
                "x": 135,
                "y": 220,
                "angle": 90
            },
            {
                "type": "block",
                "x": 120,
                "y": 200,
                "angle": 0
            }
        ]
    }
    drawLevel(ctx, level);
}

function canvasToCp(pos) {
    return { x: pos.x - canvas.width / 2, y: pos.y - canvas.height / 2 };
}

function cpToCanvas(pos) {
    return { x: pos.x + canvas.width / 2, y: pos.y + canvas.height / 2 };
}

function drawLevel(ctx, level) {
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
    ctx.drawImage(document.getElementById("block"), -50, -12.5);
    ctx.restore();
}
