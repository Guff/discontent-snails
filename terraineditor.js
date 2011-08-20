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

function indexOfTriangle(verts, selection) {
    for (var i = 0; i < selection.length; i++) {
        if (verts == selection[i].verts)
            return i;
    }
    return -1;
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
    verts["x" + j] = (e.pageX - canvas.offsetLeft + view.x) / view.zoom;
    verts["y" + j] = (e.pageY - canvas.offsetTop - view.y) / view.zoom;
    drawLevel(ctx, level);
}

function mouseOverVertex(terrain, e) {
    for (var i = 0; i < terrain.length; i++) {
        var verts = terrain[i];
        for (var j = 0; j < 3; j++) {
            if (distance((e.pageX - canvas.offsetLeft + view.x) / view.zoom,
                         (e.pageY - canvas.offsetTop - view.y) / view.zoom,
                         verts["x" + j],
                         verts["y" + j]) <= 5)
                return [j, verts];
        }
    }
    return [null, null];
}

function endVertexSelection(pos, e) {
    var x0 = (Math.min(pos.x, e.pageX - canvas.offsetLeft) + view.x) / view.zoom,
        y0 = (Math.min(pos.y, e.pageY - canvas.offsetTop) - view.y) / view.zoom,
        x1 = (Math.max(pos.x, e.pageX - canvas.offsetLeft) + view.x) / view.zoom,
        y1 = (Math.max(pos.y, e.pageY - canvas.offsetTop) - view.y) / view.zoom;
    
    for (var i = 0; i < level.terrain.length; i++) {
        var verts = level.terrain[i];
        for (var j = 0; j < 3; j++) {
            var vx = verts["x" + j], vy = verts["y" + j];
            if (vx >= x0 && vx <= x1 && vy >= y0 && vy <= y1)
                selection.push({ j: j, verts: verts })
        }
    }
    
    canvas.onmousemove = function (e) { lastMousePos = e; };
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
    var x = lastMousePos.pageX - canvas.offsetLeft,
        y = lastMousePos.pageY - canvas.offsetTop;
    terrain.push({ x0: x, y0: y, x1: x + 20, y1: y + 20, x2: x + 20, y2: y - 20 });
    drawLevel(ctx, level);
}

function deleteTriangles() {
    var triangles = [];
    for (var i = 0; i < selection.length; i++) {
        triangles.push(selection[i].verts);
        level.terrain.splice(level.terrain.indexOf(triangles[i]), 1);
        while (indexOfTriangle(triangles[i], selection) != -1) {
            selection.splice(indexOfTriangle(triangles[i], selection), 1);
        }
    }
    console.log(triangles);
    console.log(level.terrain);
    drawLevel(ctx, level);
}

function distance(x0, y0, x1, y1) {
    return Math.sqrt(Math.pow(x0 - x1, 2) + Math.pow(y0 - y1, 2));
}
