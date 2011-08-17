var stoneBorder = "rgb(20, 30, 70)";

function drawTerrain(ctx, terrain, pattern) {
    ctx.beginPath();
    for (var i = 0; i < terrain.length; i++) {
        var verts = terrain[i];
        ctx.save();
        ctx.moveTo(verts.x0, verts.y0);
        ctx.lineTo(verts.x1, verts.y1);
        ctx.lineTo(verts.x2, verts.y2);
        ctx.closePath();
        ctx.restore();
    }
    ctx.strokeStyle = stoneBorder;
    ctx.stroke();
    ctx.fillStyle = pattern;
    ctx.fill();
    ctx.lineWidth = 2;
}

function distance(a, b) {
    return Math.sqrt(Math.pow(a.x - b.x, 2) + Math.pow(a.y - b.y, 2));
}
