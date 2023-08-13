function coverPosition(coverId){
	var x, y, z;
	x = coverId*0.0001;
	y = 0;
	z = -Math.abs(coverId)*60;
	return new Array(x, y, z);
}
function coverRotation(coverId){
	var angle = coverId * 0;
	return new Array(angle,0,1,0);
}
function coverAlign(coverId){ return new Array(0, -1) }
function coverSizeLimits(coverId){ return new Array(1.0, 1.0) }

function drawCovers(){ return new Array(0,0) }
function aspectBehaviour(){ return new Array(0,1) }

// the FoV is 45° -> this calculated distance shows us exactly one cover
function eyePos(){ return new Array(0, 0.42, 1.6) }
function lookAt(){ return new Array(0, 0.42, 0) }
function upVector(){ return new Array(0, 1, 0) }

/************************** MIRROR SETUP *****************/
function showMirrorPlane(){
	return true; // return false to hide the mirror
}
// Any Point on the Mirror Plane
function mirrorPoint (){
	return new Array(0, 0, 1);
}
// Normal of the Mirror Plane
function mirrorNormal (){
	return new Array(0, 1, 0);
}

// panel properties override
function enableCoverTitle(){ return true }
function enableCoverPngAlpha(){ return true }
function enableCarousel(){ return false }
