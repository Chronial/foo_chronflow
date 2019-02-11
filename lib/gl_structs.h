#pragma once
#include <cmath>

struct glTextureCoords {
  float u;
  float v;
};

union glVectord;
struct glVertex {  // GL_V3F
  float x;
  float y;
  float z;
  operator glVectord();
  glVertex operator+(const glVertex& v) const {
    glVertex res;
    res.x = x + v.x;
    res.y = y + v.y;
    res.z = z + v.z;
    return res;
  }
  float* as_3fv() {
    return &x;
  }
};

typedef glVertex glVectorf;
union glVectord {  // GL_V3F
  struct {
    double x;
    double y;
    double z;
  };
  double contents[3];

  glVectord(double x, double y, double z) : x(x), y(y), z(z){};
  glVectord(){};

  glVectord operator+(const glVectord& v) const {
    glVectord res;
    res.x = x + v.x;
    res.y = y + v.y;
    res.z = z + v.z;
    return res;
  }
  glVectord operator/(double s) const {
    glVectord res;
    res.x = x / s;
    res.y = y / s;
    res.z = z / s;
    return res;
  }
  glVectord operator*(double s) const {
    glVectord res;
    res.x = x * s;
    res.y = y * s;
    res.z = z * s;
    return res;
  }
  glVectord operator-(const glVectord& v) const {
    glVectord res;
    res.x = x - v.x;
    res.y = y - v.y;
    res.z = z - v.z;
    return res;
  }
  double operator*(const glVectord& a) const { return x * a.x + y * a.y + z * a.z; }
  glVectord cross(const glVectord& v) const {
    glVectord res;
    res.x = y * v.z - z * v.y;
    res.y = z * v.x - x * v.z;
    res.z = x * v.y - y * v.x;
    return res;
  }
  double intersectAng(const glVectord& v) const {
    return acos(((*this) * v) / (this->length() * v.length()));
  }
  double length() const { return sqrt(x * x + y * y + z * z); }
  glVectord normalize() const {
    double l = length();
    if (l > 0)
      return (*this) / l;
    else
      return (*this);
  }
  operator glVectorf() {
    glVectorf res;
    res.x = (float)x;
    res.y = (float)y;
    res.z = (float)z;
    return res;
  }
};

inline glVectord operator*(double s, const glVectord& v) {
  return v * s;
}

inline glVertex::operator glVectord() {
  glVectord res;
  res.x = (double)x;
  res.y = (double)y;
  res.z = (double)z;
  return res;
}

struct glTexturedVertex {  // GL_T2F_V3F
  glTextureCoords tCoords;
  glVertex vCoords;
};
struct glTexturedQuad {  // GL_T2F_V3F
  glTexturedVertex topLeft;
  glTexturedVertex topRight;
  glTexturedVertex bottomRight;
  glTexturedVertex bottomLeft;
};

union glMatrix_3x3 {
  // glVectord columns[3];
  double contents[9];
  double matrix[3][3];
  static glMatrix_3x3 getRotationMatrix(double a, glVectord axis) {
    glMatrix_3x3 out;
    double cosa = cos(a);
    double sina = sin(a);
    out.contents[0] = cosa + axis.x * axis.x * (1 - cosa);
    out.contents[1] = axis.y * axis.x * (1 - cosa) + axis.z * sina;
    out.contents[2] = axis.z * axis.x * (1 - cosa) - axis.y * sina;

    out.contents[3] = axis.x * axis.y * (1 - cosa) - axis.z * sina;
    out.contents[4] = cosa + axis.y * axis.y * (1 - cosa);
    out.contents[5] = axis.z * axis.y * (1 - cosa) + axis.x * sina;

    out.contents[6] = axis.x * axis.z * (1 - cosa) + axis.y * sina;
    out.contents[7] = axis.y * axis.z * (1 - cosa) - axis.x * sina;
    out.contents[8] = cosa + axis.z * axis.z * (1 - cosa);
    return out;
  }
  glVectord operator*(const glVectord& v) const {
    glVectord out;
    for (int i = 0; i < 3; i++) {
      out.contents[i] = 0;
      for (int j = 0; j < 3; j++) {
        out.contents[i] += this->matrix[j][i] * v.contents[j];
      }
    }
    return out;
  }
};

struct glQuad {  // GL_V3F
  glVertex topLeft;
  glVertex topRight;
  glVertex bottomRight;
  glVertex bottomLeft;
  void rotate(double a, glVectord axis) {
    glMatrix_3x3 trans = glMatrix_3x3::getRotationMatrix(a, axis);
    topLeft = trans * topLeft;
    topRight = trans * topRight;
    bottomLeft = trans * bottomLeft;
    bottomRight = trans * bottomRight;
  }
};
