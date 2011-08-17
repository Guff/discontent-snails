var vertexColor = "rgba(40, 40, 200, 0.6)";
var selectedVertexColor = "rgba(40, 200, 40, 0.6)";
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
    ctx.lineWidth = 2;
    ctx.stroke();
    ctx.fillStyle = pattern;
    ctx.fill();
    ctx.closePath();
}

function vertexInSelection(x, y) {
    for (var i = 0; i < selection.length; i++) {
        for (var j = 0; j < 3; j++) {
            var selX = selection[i].verts["x" + j],
                selY = selection[i].verts["y" + j];
            
            if (selX == x && selY == y)
                return true;
        }
    }
    return false;
}

function drawVertices(ctx, terrain) {
    ctx.beginPath();
    ctx.fillStyle = vertexColor;
    for (var i = 0; i < terrain.length; i++) {
        var verts = terrain[i];
        for (var j = 0; j < 3; j++) {
            ctx.arc(verts["x" + j], verts["y" + j], 5, 0, 2 * Math.PI);
            ctx.closePath();
            //if (vertexInSelection(verts["x" + j], verts["y" + j]))
                //ctx.fillStyle = selectedVertexColor;
            ctx.fill();
            //ctx.fillStyle = vertexColor;
        }
    }
    
}

function onTerrainMouseMove(j, verts, e) {
    verts["x" + j] = e.pageX - canvas.offsetLeft;
    verts["y" + j] = e.pageY - canvas.offsetTop;
    drawLevel(ctx, level);
    drawVertices(ctx, level.terrain);
}

function mouseOverVertex(terrain, e) {
    for (var i = 0; i < terrain.length; i++) {
        var verts = terrain[i];
        for (var j = 0; j < 3; j++) {
            if (distance(e.pageX - canvas.offsetLeft, e.pageY - canvas.offsetTop,
                         verts["x" + j], verts["y" + j]) <= 5)
                return [j, verts];
        }
    }
    return [null, null];
}

function addTriangle(terrain) {
    terrain.push({ x0: 320, y0: 240, x1: 340, y1: 260, x2: 340, y2: 220 });
    drawLevel(ctx, level);
    drawVertices(ctx, terrain);
}

function distance(x0, y0, x1, y1) {
    return Math.sqrt(Math.pow(x0 - x1, 2) + Math.pow(y0 - y1, 2));
}
