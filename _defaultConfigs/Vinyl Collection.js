// Original Concept: Martin Gloderer

var viewFromLeftSide = false;
var coverSpacing = 0.03;


function coverPosition(coverId){
	var z = -coverId * coverSpacing;
	return new Array(0, 0, z);
}

function coverRotation(coverId){
	var angle;
	if (coverId < -0.7){
	   angle = 32;
	} else if (coverId >= -0.7 && coverId <= -0.3) {
	   angle = -32 * (1 + (coverId + 0.3) * 5);
	} else { // coverId > -0.3
	   angle = -32;
	}
	angle += 22;
	return new Array(angle,1,0,0);
}

function coverAlign(coverId){	return new Array(0, -1) }
function coverSizeLimits(coverId){ return new Array(1, 1) }

function drawCovers(){ return new Array(-7, 20) }
function aspectBehaviour(){ return new Array(0,1) }

function eyePos(){
   var x = 0.4;
   if (viewFromLeftSide) x *= -1;
	return new Array(x, 1.1, 1.3);
}
function lookAt(){ return new Array(0, 0.5, 0) }
function upVector(){ return new Array(0, 1, 0) }

function showMirrorPlane(){ return false; }
