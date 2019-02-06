// This config is very fast, since it draws only 5 covers

function coverPosition(coverId){
   var x, y, z;
   x = coverId * 1.1;
   y = 0;
   z = -1.5 - Math.abs(coverId);
   return new Array(x, y, z);
}

function coverRotation(coverId){
   var angle;
   // this makes the covers disappera smoothly
   if (Math.abs(coverId) >= 2){
      var f = Math.abs(coverId) - 2 // this is in [0-1]
      // at this angle the covers are not visible
      var hideAngle = rad2deg(Math.atan(coverPosition(3)[2] / coverPosition(3)[0]));
      angle = hideAngle * f;
      if (coverId > 0)
          angle *= -1;
   } else {
      angle = 0;
   }
   return new Array(angle,0,1,0);
}
function rad2deg(angle){
   return angle/Math.PI * 180;
}


function coverSizeLimits(coverId){ return new Array(1, 1) }
function coverAlign(coverId){ return new Array(0, 0) }

function drawCovers(){ return new Array(-3, 3) }
function aspectBehaviour(){ return new Array(0, 1) }


function eyePos(){ return new Array(0, 0, 0) }
function lookAt(){ return new Array(0, 0, -1.5) }
function upVector(){ return new Array(0, 1, 0) }

function showMirrorPlane(){ return false }
