#pragma once
#include <iostream>
#include <cmath>

template <class t> struct Vec2
{
	union 
	{
		struct { t x, y; };
		struct { t u, v; };
		t raw[2];
	};

    Vec2()             : x(0),  y(0)  {}
    explicit Vec2(t s) : x(s),  y(s)  {}
	Vec2(t _x, t _y)   : x(_x), y(_y) {}

	Vec2<t> operator + (const Vec2<t> &v) const { return Vec2<t>(x + v.x, y + v.y); }
	Vec2<t> operator - (const Vec2<t> &v) const { return Vec2<t>(x - v.x, y - v.y); }
	Vec2<t> operator * (t s)              const { return Vec2<t>(x * s,   y * s);   }
	t       operator * (const Vec2<t> &v) const { return (x * v.x) + (y * v.y);     }
    t       operator ^ (const Vec2<t> &v) const { return (x * v.y) - (y * v.x);     }

	float   length()                     const { return std::sqrt((x * x) + (y * y)); }
	Vec2<t> normalized()                 const { return (*this) * (1.0f/length());    }
    Vec2<t> hadamarded(const Vec2<t>& v) const { return Vec2<t>(x * v.x, y * v.y);    }

    template <class > friend std::ostream& operator<<(std::ostream& s, Vec2<t>& v);
};

template <class t> struct Vec4;

template <class t> struct Vec3 
{
	union 
	{
		struct { t x, y, z; };
        struct { t r, g, b; };
		t raw[3];
	};

	Vec3()                 : x(0),  y(0),  z(0)  {}
	explicit Vec3(t s)     : x(s),  y(s),  z(s)  {}
	Vec3(t _x, t _y, t _z) : x(_x), y(_y), z(_z) {}
	Vec3(Vec4<t> v)		   : x(v.x), y(v.y), z(v.z) {}

	Vec3<t> operator + (const Vec3<t> &v) const { return Vec3<t>(x + v.x, y + v.y, z + v.z); }
	Vec3<t> operator - (const Vec3<t> &v) const { return Vec3<t>(x - v.x, y - v.y, z - v.z); }
	Vec3<t> operator * (t s)              const { return Vec3<t>(x * s, y * s, z * s);       }
	t       operator * (const Vec3<t> &v) const { return (x * v.x) + (y * v.y) + (z * v.z);  }
	Vec3<t> operator ^ (const Vec3<t> &v) const { return Vec3<t>((y * v.z) - (z * v.y), (z * v.x) - (x * v.z), (x * v.y) - (y * v.x)); }

	float   length()                     const { return std::sqrt((x * x) + (y * y) + (z * z)); }
	Vec3<t> normalized()                 const { return (*this) * (1.0f/length());              }
	Vec3<t> hadamarded(const Vec3<t>& v) const { return Vec3<t>(x * v.x, y * v.y, z * v.z);     }

	Vec2<t> xy() const { return Vec2<t>(x, y); }

	template <class > friend std::ostream& operator<<(std::ostream& s, Vec3<t>& v);
};

template <class t> struct Vec4
{
	union 
	{
		struct { t x, y, z, w; };
		t raw[4];
	};

	Vec4()                       : x(0),   y(0),   z(0),   w(0)  {}
	explicit Vec4(t s)			 : x(s),   y(s),   z(s),   w(s)  {}
	Vec4(Vec3<t> v, t w)         : x(v.x), y(v.y), z(v.z), w(w)  {}
	Vec4(t _x, t _y, t _z, t _w) : x(_x),  y(_y),  z(_z),  w(_w) {}

	Vec4<t> operator + (const Vec4<t> &v) const { return Vec4<t>(x + v.x, y + v.y, z + v.z, w + v.w); }
	Vec4<t> operator - (const Vec4<t> &v) const { return Vec4<t>(x - v.x, y - v.y, z - v.z, w - v.w); }
	Vec4<t> operator * (float s)          const { return Vec4<t>(x * s, y * s, z * s, w * s);         }

	Vec3<t> homogenized() const { return Vec3<t>(x / w, y / w, z / w); }
	Vec3<t> xyz()         const { return Vec3<t>(x, y, z);             }

	template <class > friend std::ostream& operator<<(std::ostream& s, Vec4<t>& v);
};

typedef Vec2<int>   Vec2i;
typedef Vec2<float> Vec2f;
typedef Vec3<int>   Vec3i;
typedef Vec3<float> Vec3f;
typedef Vec4<float> Vec4f;

template <class t> std::ostream& operator<<(std::ostream& s, Vec2<t>& v) {
	s << "(" << v.x << ", " << v.y << ")\n";
	return s;
}

template <class t> std::ostream& operator<<(std::ostream& s, Vec3<t>& v) {
	s << "(" << v.x << ", " << v.y << ", " << v.z << ")\n";
	return s;
}

template <class t> std::ostream& operator<<(std::ostream& s, Vec4<t>& v) {
	s << "(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")\n";
	return s;
}


/**
 * CREDIT: Some of this code is from Dmitry V. Sokolov. Note, I did modify it.
 * REPO: https://github.com/ssloy/tinyrenderer/tree/a175be75a8a9a773bdfae7543a372e3bc859e02f
 */


// Utility functions
Vec3f clampedVec3f(const Vec3f& v, float min, float max);
Vec2f clampedVec2f(const Vec2f& v, float min, float max);
Vec3f lerpVec3f(const Vec3f& a, const Vec3f& b, float t);