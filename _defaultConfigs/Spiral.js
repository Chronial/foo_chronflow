var outerRadius = 1.3;
var verticalSpacing = 1.7;


// Some geometry (this is a regular polygon)
var sidelength = 1;
var n = Math.PI / Math.asin(sidelength / (2*outerRadius))
var a = Math.PI / n;
var factor = 2*Math.PI / n;
var innerRadius = outerRadius * Math.cos(a);

function coverPosition(coverId){
   var x, y, z;
   x = Math.sin(coverId * factor) * innerRadius;
   y = coverId * verticalSpacing / n;
   z = Math.cos(coverId * factor) * innerRadius;
   return new Array(x, y, z);
}

function coverRotation(coverId){
   // We use radian but have to return degrees
   var angle = rad2deg(coverId*factor);
   return new Array(angle, 0, 1, 0);
}
function rad2deg(angle){
   return angle/Math.PI * 180;
}

function coverSizeLimits(coverId){ return new Array(sidelength, verticalSpacing*0.85) }
function coverAlign(coverId){ return new Array(0, -1) }

function drawCovers(){ return new Array(-15, 18) }
function aspectBehaviour(){ return new Array(1, 0) }


function eyePos(){ return new Array(0.5, 0.9, outerRadius*4) }
function lookAt(){ return new Array(0.5, 0.9, 0) }
function upVector(){ return new Array(0, 1, 0) }


function showMirrorPlane(){ return true }
function mirrorPoint(){ return new Array(outerRadius, 0, 0) }
function mirrorNormal(){ return new Array(1, 0, 0) }

// panel properties override
function enableCoverTitle(){ return true }
function enableCoverPngAlpha(){ return true }
