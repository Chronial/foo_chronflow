// General Notes:
// Most of these functions return an array with 3 elements
// These are the x,y,z coordinates in 3d-space
// x is left to right
// y is bottom to top
// z is front to back


/************************* COVER DISPLAY *****************/
// These functions define the Display of the single Covers
// The given parameter coverId is a floating point number.
// It is 0 for the center cover, 1 for the one right
// beneath it, -1 for the one on the left side and so on.
// During movement the values float between the integer
// values.
function coverPosition(coverId){
   var x, y, z;
   x = coverId*1.1;
   y = coverId*coverId/5;
   z = -Math.abs(coverId);
   return new Array(x, y, z);
}
// return array is (angle, x, ,y, z) - this rotates
// the cover *angle* degrees around the vector (x,y,z)
// With (0,0,0,0) the cover is parallel to the y-z-Plane
function coverRotation(coverId){
   var angle;
   angle = -(coverId)*20;
   return new Array(angle,0,1,0);
}

// Sets which point of the cover coverPosition() defines
// (-1,-1) means bottom left, (0,0) means center,
// (1,1) means top right, (0, -1) means bottom center etc.
// The cover is also rotated around this point.
function coverAlign(coverId){
   return new Array(0, -1);
}

// Defines the the size boundaries for the cover.
// Aspect ratio is preserved.
// Return Array is (widht, height)
function coverSizeLimits(coverId){
   return new Array(1, 2);
}

// Defines the range of covers to draw.
// Return array is (leftmostCover, rightmostCover)
// This interval shouldn't be larger than 80
// The center cover is 0.
function drawCovers(){
   return new Array(-10, 10);
}


// In which direction should the fov be expanded/shrinked
// when the panel is resized?
// If this returns (0,1), the height is fixed.
// If this returns (1,0), the width is fixed.
// You can also return stuff like (0.5,0.5) or (7, 3)
// The values determine how important it is for this
// dimension to stay fixed.
function aspectBehaviour(){
   return new Array(0.5,0.5);
}

/************************** CAMMERA SETUP ****************/
// Position of the viewport
function eyePos(){
	return new Array(0, 1, 6);
}
// Defines the point for the eye to look at
function lookAt(){
   return new Array(0, 1, 0);
}
// Used to rotate the view.
// The returned Vector points upwards in the viewport.
// This vector must not be parallel to the line of sight from the
// eyePos point to the lookAt point.
function upVector(){
   return new Array(0, 1, 0);   
}

/************************** MIRROR SETUP *****************/
function showMirrorPlane(){
   return true; // return false to hide the mirror
}
// Any Point on the Mirror Plane
function mirrorPoint (){
   return new Array(0, 0, 0);
}
// Normal of the Mirror Plane
function mirrorNormal (){
   return new Array(0, 1, 0);
}