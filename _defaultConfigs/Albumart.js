function coverPosition(coverId){
   var x, y, z;
   x = coverId*0.0001;
   y = 0;
   z = -Math.abs(coverId)*1.5;
   return new Array(x, y, z);
}
function coverRotation(coverId){
   var angle = coverId * 180;
   return new Array(angle,0,1,0);
}
function coverAlign(coverId){
   return new Array(0, 0);
}
function coverSizeLimits(coverId){
   return new Array(1, 1);
}

function drawCovers(){
   return new Array(-1, 1);
}
function aspectBehaviour(){
   return new Array(1, 0);
}

function eyePos(){
	return new Array(0, 0, 0.5/Math.tan(Math.PI/8));
}
function lookAt(){
   return new Array(0, 0, 0);
}
function upVector(){
   return new Array(0, 1, 0);   
}

function showMirrorPlane(){
   return false;
}