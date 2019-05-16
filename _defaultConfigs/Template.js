/*                 3D Album Cover Setup

The functions defined in this JavaScript file configure the
3D setup of the album covers. You can costumize the
position, rotation and animation of the covers here.

Feel free to expirement a bit – the built-in configurations
will always be there for you to return to. If you create
a setup you like, why not share it in the forum for others
to enjoy, too?

Some general notes:
* Many of the functions return an array with 3 elements.
  These elements are x,y,z coordinates in 3d space, where
  x is left to right, y bottom to top and z back to front.
* Many of the functions take a cooverId argument. That
  argument specifies for which cover to return data. It is
  0 for the selected cover, 1 for the one to the right of
  it, -1 for the one on the left etc.
  But because the covers are animated, coverId can also
  take on non-integer values. For example: when the
  selected cover is moving one to the right and is halfway
  between its starting position and end position, coverId 
  will be 0.5.
* Due to performance optimizations, it is not possible to
  create non-continuous animation (i.e. have the covers
  jump in space. Even if your functions have a sudden
  change in output values, the covers will move in a
  continuous fashion.                                    */

/*********************************************************/
/********************** COVER SETUP **********************/
/*********************************************************/

// Set the range of covers to draw.
//
// Returns: (leftmostCover, rightmostCover)
function drawCovers(){
   return new Array(-10, 10);
}

// Set the position of a point on each cover.
//
// See also coverAlign() below
function coverPosition(coverId){
   var x, y, z;
   x = coverId*1.1;
   y = 0;
   z = -Math.abs(coverId)*0.6;
   return new Array(x, y, z);
}

// Set which point of the cover coverPosition() defines.
//
// (-1,-1) means bottom left, (0,0) means center,
// (1,1) means top right, (0, -1) means bottom center etc.
//
// Returns: (x, y)
function coverAlign(coverId){
   return new Array(0, -1);
}

// Set the rotation of each cover.
// 
// Returns: (angle, x, y, z) - this rotates the cover
//   `angle` degrees around the axis along (x,y,z). Per
//   default, the covers are parallel to the x-y-plane.
function coverRotation(coverId){
   return new Array(0, 0, 1, 0);
}

// Set the the size boundaries for the cover.
//
// Preserving its aspect ratio, the cover is scaled to fit
// inside the given rectangle.
//
// Returns: (width, height)
function coverSizeLimits(coverId){
   return new Array(1, 2);
}

/*********************************************************/
/********************* CAMERA SETUP **********************/
/*********************************************************/

// Defines how the viewport behaves when the aspect ratio
// of the window changes.
//
// Returns: Expand into (width, height)
//   If this returns (0,1), the height is fixed.
//   If this returns (1,0), the width is fixed.
//   Can also return mixed values, i.e. (1,2) for mixed 
//   scaling.
function aspectBehaviour(){
   return new Array(0, 1);
}


// Set the position of the camera.
function eyePos(){
	return new Array(0, 0.5, 2);
}

// Set the point that the camera is looking at.
function lookAt(){
   return new Array(0, 0.5, 0);
}

// Set where up is.
//
// This can be used to rotate the view. The returned vector
// will point upwards in the viewport.
function upVector(){
   return new Array(0, 1, 0);   
}

/*********************************************************/
/********************* MIRROR SETUP **********************/
/*********************************************************/

// Decide whether the mirror should be rendered.
function showMirrorPlane(){
   return true;
}

// Set an arbitrary point on the mirror plane.
function mirrorPoint(){
   return new Array(0, 0, 0);
}

// Set the normal vector of the mirror plane.
function mirrorNormal(){
   return new Array(0, 1, 0);
}
