// Author: Martin Gloderer

function coverPosition(coverId){
   // Ensure smooth exit at the borders of drawCovers()
   if (coverId < drawCovers()[0]+1)
      return coverPosition(drawCovers()[0]+1);
   if (coverId > drawCovers()[1]-1) {
      array = coverPosition(drawCovers()[1]-1);
      array[2] -= 0.0001;
      return array;
   }

   var x, y, z;
   var cAbs = Math.abs(coverId);

   if (cAbs >= 1) {
      x = Math.ceil(cAbs/4) + 1.8;
      y = (cAbs-1) % 8;
      y = Math.min(y, 7-y);
      if (y > 3 || y < 0) { //transistion between columns
         x -= 1 - cAbs % 1;
      }
      y = Math.min(Math.max(y,0),3);
      z = -Math.abs(x - 2.8)/5;
   } else { //cAbs < 1
      x = sigmoidInterpolation(cAbs)*Math.sqrt(cAbs) * 2.8;
      y = 0;
      z = sigmoidInterpolation(1-cAbs) * 0.4;
   }
   if (coverId < 0) {
      x *= -1;
   }
   // Shrink scene to get a stronger mirror effect
   x /= 4;  y /= 4;  z /= 2.5;
   return new Array(x, y, z);
}

function coverSizeLimits(coverId){
   var cAbs = Math.abs(coverId);
   var w, h;
   if (cAbs < 1){ // The centered cover
      w = (sigmoidInterpolation(1-cAbs)*(1-cAbs) * 0.75)+0.25;
      h = (sigmoidInterpolation(1-cAbs)*(1-cAbs) * 0.75)+0.25;
   } else {
      // Shrink scene to get a stronger mirror effect
      w = 0.25;
      h = 0.25;
   }
   return new Array(h, w);
}

function sigmoidInterpolation(t) {
   t = (t-0.5)*8;
   t = 1/(1 + Math.pow(Math.E, -t));
   return t;
}

function coverRotation(coverId){ return new Array(0, 0, 0, 0) }
function coverAlign(coverId){ return new Array(0, -1) }


function drawCovers(){ return new Array(-25, 25) }
function aspectBehaviour(){ return new Array(0,1) }


function eyePos(){ return new Array(0, 0.42, 1.6) }
function lookAt(){ return new Array(0, 0.42, 0) }
function upVector(){ return new Array(0, 1, 0) }


function showMirrorPlane(){ return true }
function mirrorPoint (){ return new Array(0, 0, 0) }
function mirrorNormal (){ return new Array(0, 1, 0) }

// panel properties override
function enableCoverTitle(){ return true }
function enableCoverPngAlpha(){ return true }
