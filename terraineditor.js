function drawTerrain(ctx, terrain) {
    ctx.beginPath();
    for (var i = 0; i < terrain.length; i++) {
        var verts = terrain[i].verts;
        ctx.save();
        ctx.moveTo(verts[0].x, verts[0].y);
        for (var v = 1; v < verts.length; v++)
            ctx.lineTo(verts[v].x, verts[v].y);
        ctx.closePath();
    }
    ctx.fillStyle = "rgb(0, 0, 0)";
    ctx.fill();
}
