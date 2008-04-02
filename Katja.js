var outerRadius = 2;
var verticalSpacing = 1.7;

// Some geometry (this is a regular polygon)
var sidelength = 1;
var n = Math.PI / Math.asin(sidelength / (2*outerRadius))
var a = Math.PI / n;
var factor = 2*Math.PI / n;
var innerRadius = outerRadius * Math.cos(a);

function circlePos(coverId){
   var x, y, z;
   x = Math.sin(coverId*factor)*innerRadius;
   y = 0;
   z = Math.cos(coverId*factor)*innerRadius;
   return new Array(x, y, z);
}
function circleRot(coverId){
   var angle = (coverId*factor)/Math.PI * 180;
   return new Array(angle, 0, 1, 0);
}


var leftCircLC = -7;
var leftCircCC = leftCircLC-n/4-n/2;
var leftCircRC = leftCircLC-n/2;
var leftCircPos = new Array(-2, 0, 0);
var rightCircPos = new Array(2, 0, 0);

var rightCircRC = -leftCircLC
var rightCircCC = -leftCircCC;
var rightCircLC = -leftCircRC;


function coverPosition(coverId){
   var pos;
   coverId -= 0.5;
   if (coverId >= leftCircRC && coverId <= leftCircLC){
      pos = circlePos(coverId - leftCircCC);
      pos[0] += leftCircPos[0];
      pos[1] += leftCircPos[1];
      pos[2] += leftCircPos[2];
   } else if (coverId >= rightCircRC && coverId <= rightCircLC){
      pos = circlePos(coverId - rightCircCC);
      pos[0] += rightCircPos[0];
      pos[1] += rightCircPos[1];
      pos[2] += rightCircPos[2];
   } else if (coverId > leftCircLC && coverId < rightCircRC){
      return new Array(4 * coverId/rightCircRC, 0, 6*(1-Math.abs(coverId/rightCircRC)));
   } else {
      return new Array(0, -1000, 05);
   }
   return pos;
}

function coverRotation(coverId){
   coverId -= 0.5;
   if (coverId >= leftCircRC && coverId <= leftCircLC){
      return circleRot(coverId - leftCircCC);
   } else if (coverId >= rightCircRC && coverId <= rightCircLC){
      return circleRot(coverId - rightCircCC);
   } else if (coverId > leftCircLC && coverId < rightCircRC){
      var angle = Math.atan(1.4) / (2*Math.PI) * 360;
      if (coverId > 0)
         return new Array(angle, 0, 1, 0);  
      else
         return new Array(-angle, 0, 1, 0);         
   } else {
      return new Array(0, 0, 0, 0);
   }
}

function coverAlign(coverId){
   return new Array(0, -1);
}

function coverSizeLimits(coverId){
   return new Array(sidelength, verticalSpacing*0.85);
}

function drawCovers(){
   return new Array(-15, 18);
	
}

function aspectBehaviour(){
	return new Array(1, 0);
}

/************************** CAMMERA SETUP ****************/
function eyePos(){
   return new Array(0, 7, 12);
}
function lookAt(){
   return new Array(0.5, 0.9, 3);
}
function upVector(){
   return new Array(0, 1, 0);
}

/************************** MIRROR SETUP *****************/
function showMirrorPlane(){
   return true; // return false to hide the mirror
}
// Any Point on the Mirror Plane
function mirrorPoint (){
   return new Array(0 , 0, 0);
}
// Normal of the Mirror Plane
function mirrorNormal (){
   return new Array(0, 1, 0);
}