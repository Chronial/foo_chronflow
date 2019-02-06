function coverPosition(coverId){
   var x, y, z;
   y = 0;
   if (Math.abs(coverId) <= 1){ // The centered cover
      z = 1 + 3 * (1 - Math.abs(coverId));
      x = coverId;
   } else { // The covers on the side
      z = 1 - (Math.abs(coverId)-1) * 0.15;
      x = 1 + 0.5 * (Math.abs(coverId)-1);
      if (coverId < 0)
         x *= -1;
   }
   return new Array(x, y, z);
}

function coverRotation(coverId){
   var angle;
   if (Math.abs(coverId) < 1){ // The centered cover
      angle = coverId * -70;
   } else { // The covers on the side
      if (coverId > 0)
         angle = -70;
      else
         angle = 70;
   }
   return new Array(angle, 0, 1, 0);
}

function coverSizeLimits(coverId){
   if (Math.abs(coverId) < 1){ // The centered cover
      var w, h;
      w = 1;
      h = 1.2 + Math.abs(coverId) * 0.8;
      return new Array(w, h);
   } else { // The covers on the side
      return new Array(1, 2);
   }
}

function coverAlign(coverId){ return new Array(0, -1) }

function drawCovers(){ return new Array(-20, 20) }
function aspectBehaviour(){ return new Array(0,1) }

function eyePos(){ return new Array(0, 0.5, 6) }
function lookAt(){ return new Array(0, 0.5, 0) }
function upVector(){ return new Array(0, 1, 0) }

function showMirrorPlane(){ return true }
function mirrorPoint(){ return new Array(0, 0, 0) }
function mirrorNormal(){ return new Array(0, 1, 0) }
