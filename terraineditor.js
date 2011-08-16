var stoneBorder = "rgb(20, 30, 70)";

function drawTerrain(ctx, terrain, pattern) {
    ctx.beginPath();
    for (var i = 0; i < terrain.length; i++) {
        var verts = terrain[i].verts;
        ctx.save();
        ctx.moveTo(verts[0].x, verts[0].y);
        for (var v = 1; v < verts.length; v++)
            ctx.lineTo(verts[v].x, verts[v].y);
        ctx.closePath();
        ctx.restore();
    }
    ctx.fillStyle = pattern;
    ctx.fill();
    ctx.lineWidth = 2;
    ctx.strokeStyle = stoneBorder;
    ctx.stroke();
}
