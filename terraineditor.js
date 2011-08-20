var vertexColor = "rgba(40, 40, 200, 0.6)";
var selectedVertexColor = "rgba(40, 200, 40, 0.6)";
var vertexSelectionColor = "rgba(40, 40, 140, 0.4)";
var vertexBorderColor = "rgba(20, 20, 125, 0.5)";
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
        var j = selection[i].j;
        var selX = selection[i].verts["x" + j],
            selY = selection[i].verts["y" + j];
        if (selX == x && selY == y)
            return true;
    }
    return false;
}

function drawVertices(ctx, terrain) {
    ctx.fillStyle = vertexColor;
    for (var i = 0; i < terrain.length; i++) {
        var verts = terrain[i];
        for (var j = 0; j < 3; j++) {
            ctx.beginPath();
            ctx.arc(verts["x" + j], verts["y" + j], 5, 0, 2 * Math.PI);
            ctx.closePath();
            if (vertexInSelection(verts["x" + j], verts["y" + j]))
                ctx.fillStyle = selectedVertexColor;
            ctx.fill();
            ctx.fillStyle = vertexColor;
        }
    }
}

function onTerrainMouseMove(j, verts, e) {
    verts["x" + j] = e.pageX - canvas.offsetLeft;
    verts["y" + j] = e.pageY - canvas.offsetTop;
    drawLevel(ctx, level);
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

function endVertexSelection(pos, e) {
    var x0 = Math.min(pos.x, e.pageX - canvas.offsetLeft),
        y0 = Math.min(pos.y, e.pageY - canvas.offsetTop),
        x1 = Math.max(pos.x, e.pageX - canvas.offsetLeft),
        y1 = Math.max(pos.y, e.pageY - canvas.offsetTop);
    
    for (var i = 0; i < level.terrain.length; i++) {
        var verts = level.terrain[i];
        for (var j = 0; j < 3; j++) {
            var vx = verts["x" + j], vy = verts["y" + j];
            if (vx >= x0 && vx <= x1 && vy >= y0 && vy <= y1)
                selection.push({ j: j, verts: verts })
        }
    }
    drawLevel(ctx, level);
}

function moveVertices(dx, dy) {
    selection.forEach(function (vertices) {
        var j = vertices.j;
        var v = vertices.verts;
        v["x" + j] += dx;
        v["y" + j] += dy;
    });
    drawLevel(ctx, level);
}

function addTriangle(terrain) {
    terrain.push({ x0: 320, y0: 240, x1: 340, y1: 260, x2: 340, y2: 220 });
    drawLevel(ctx, level);
}

function distance(x0, y0, x1, y1) {
    return Math.sqrt(Math.pow(x0 - x1, 2) + Math.pow(y0 - y1, 2));
}
