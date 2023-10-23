#ifndef DEF_MMATH_HPP
#define DEF_MMATH_HPP

#include "Platform.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <float.h>
#include <limits>
#include "../Include/VWBTypes.h"

#define PI 3.14159265358979323846
#define PIDIV2 (3.14159265358979323846 / 2)
#define DEG2RAD( v ) ( (v) * VWB_float(PI / 180) )
#define RAD2DEG( v ) ( (v) * VWB_float(180 / PI) )
#define DEG2RADf( v ) ( (v) * VWB_float(PI / 180) )
#define RAD2DEGf( v ) ( (v) * VWB_float(180 / PI) )
#define DEG2RADd( v ) ( (v) * VWB_double(PI / 180) )
#define RAD2DEGd( v ) ( (v) * VWB_double(180 / M_PI) )

/// calculates barycentric coordinates from a point p in relation to triangle formed by p1, p2, p3 all vec2, l is a vec3
template< class T >
_inline_ bool Cart2Bary2D( T const* p1, T const* p2, T const* p3, T const* p, T* l )
{
	T v0[2] = { p2[0] - p1[0], p2[1] - p1[1] };
	T v1[2] = { p3[0] - p1[0], p3[1] - p1[1] };
	T v2[2] = { p[0] - p1[0],  p[1] - p1[1] };
	T den = v0[0] * v1[1] - v1[0] * v0[1];
	if( std::numeric_limits<T>::epsilon() > abs( den ) )
		return false;

	den = T( 1 ) / den;
	l[1] = ( v2[0] * v1[1] - v1[0] * v2[1] ) * den;
	l[2] = ( v0[0] * v2[1] - v2[0] * v0[1] ) * den;
	l[0] = 1.0f - l[2] - l[1];
	return true;
}

/// calculates cartesian from barycentric coordinates, all vectors except l are vec2, which is vec3
template< class T >
_inline_ void Bary2Cart2D( T const* p1, T const* p2, T const* p3, T const* l, T* p )
{
	p[0] = l[0] * p1[0] + l[1] * p2[0] + l[2] * p3[0];
	p[1] = l[0] * p1[1] + l[1] * p2[1] + l[2] * p3[1];
}

template<class T>
struct VWB_MATRIX;

template<class T>
struct VWB_MATRIX33;

template<class T>
struct VWB_VECTOR4;

#pragma pack( push, 4 )
template<class T>
struct VWB_VECTOR3
{
	T x;
	T y;
	T z;
	_inline_ VWB_VECTOR3() = default;
	_inline_ VWB_VECTOR3( VWB_VECTOR3 const& ) = default;
	template< class _T2 >
	_inline_ explicit VWB_VECTOR3( VWB_VECTOR3<_T2> const& other ) : x( (T)other.x ), y( (T)other.y ), z( (T)other.z ) {}
	template< class _T2 >
	_inline_ explicit VWB_VECTOR3( VWB_VECTOR4<_T2> const& other, bool divideByW = false ) : x( (T)( other.x ) ), y( (T)( other.y ) ), z( (T)( other.z ) )
	{
		if( divideByW && T(0) != other.w )
		{
			T w = T(1) / (T)other.w;
			x *= w;
			y *= w;
			z *= w;
		}
	}
	_inline_ explicit VWB_VECTOR3( T const* p ) : x( p[0] ), y( p[1] ), z( p[2] ) {}
	_inline_ VWB_VECTOR3( T _x, T _y, T _z ) : x(_x), y(_y), z(_z) {}

	_inline_ static VWB_VECTOR3 const& ptr( T const* p ) { return *(VWB_VECTOR3 const*)p; }
	_inline_ static VWB_VECTOR3& ptr( T* p ) { return *(VWB_VECTOR3*)p; }
	_inline_ static VWB_VECTOR3 O() { return VWB_VECTOR3(0,0,0); }
	_inline_ static VWB_VECTOR3 Ex() { return VWB_VECTOR3(1,0,0); }
	_inline_ static VWB_VECTOR3 Ey() { return VWB_VECTOR3(0,1,0); }
	_inline_ static VWB_VECTOR3 Ez() { return VWB_VECTOR3(0,0,1); }

	
	// calculates barycentric coordinates from a point p in relation to triangle formed by p1, p2, p3 all are vec3, l is vec3
	_inline_ static bool Cart2Bary( VWB_VECTOR3 const& p1, VWB_VECTOR3 const& p2, VWB_VECTOR3 const& p3, VWB_VECTOR3 const& p, VWB_VECTOR3& l )
	{

		T v0[3] = { p2[0] - p1[0], p2[1] - p1[1], p2[2] - p1[2] };
		T v1[3] = { p3[0] - p1[0], p3[1] - p1[1], p3[2] - p1[2] };
		T v2[3] = { p[0] - p1[0],  p[1] - p1[1],  p[2] - p1[2] };
		T d00 = v0[0] * v0[0] + v0[1] * v0[1] + v0[2] * v0[2];
		T d01 = v0[0] * v1[0] + v0[1] * v1[1] + v0[2] * v1[2];
		T d11 = v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2];
		T d20 = v2[0] * v0[0] + v2[1] * v0[1] + v2[2] * v0[2];
		T d21 = v2[0] * v1[0] + v2[1] * v1[1] + v2[2] * v1[2];

		T den = d00 * d11 - d01 * d01;
		if( std::numeric_limits<T>::epsilon() > abs( den ) )
			return false;

		den = T( 1 ) / den;
		l[1] = ( d11 * d20 - d01 * d21 ) * den;
		l[2] = ( d00 * d21 - d01 * d20 ) * den;
		l[0] = T( 1 ) - l[2] - l[1];
		return true;
	}
	_inline_ VWB_VECTOR3 Cart2Bary( VWB_VECTOR3 const& p1, VWB_VECTOR3 const& p2, VWB_VECTOR3 const& p3, VWB_VECTOR3 const& p )
	{
		VWB_VECTOR3& l;
		if( Cart2Bary( p1, p2, p3, p, l ) )
			return l;
		else
			return VWB_VECTOR3::O();
	}

	/// calculates cartesian from barycentric coordinates, all vectors are vec3
	_inline_ static void Bary2Cart( VWB_VECTOR3 const& p1, VWB_VECTOR3 const& p2, VWB_VECTOR3 const& p3, VWB_VECTOR3 const& l, VWB_VECTOR3& p )
	{
		p[0] = l[0] * p1[0] + l[1] * p2[0] + l[2] * p3[0];
		p[1] = l[0] * p1[1] + l[1] * p2[1] + l[2] * p3[1];
		p[2] = l[0] * p1[2] + l[1] * p2[2] + l[2] * p3[2];
	}
	_inline_ VWB_VECTOR3 Bary2Cart( VWB_VECTOR3 const& p1, VWB_VECTOR3 const& p2, VWB_VECTOR3 const& p3, VWB_VECTOR3 const& l ) 
	{
		VWB_VECTOR3 p;
		Bary2Cart( p1, p2, p3, l, p );
		return p;
	}

	_inline_ operator T const* () const { return &x; }
	_inline_ operator T* () { return &x; }
	_inline_ T lenSq() const { return x * x + y * y + z * z; }
	_inline_ T len() const { return sqrt( lenSq() ); }
	_inline_ T norm() const { return sqrt( lenSq() ); }

	_inline_ VWB_VECTOR3 operator-() const { return VWB_VECTOR3( -x, -y, -z ); }
	_inline_ bool operator==(VWB_VECTOR3 const& other) const { return x == other.x && y == other.y && z == other.z; }
	_inline_ bool operator!=(VWB_VECTOR3 const& other) const { return x != other.x || y != other.y || z != other.z; }

	// multiply and divide
	_inline_ T dot( VWB_VECTOR3 const& other ) const// dot
	{
		T res = x * other.x + y * other.y + z * other.z;
		return res;
	}
	_inline_ VWB_VECTOR3 operator*( T const& other ) const// scalar
	{
		return VWB_VECTOR3( x * other, y * other, z * other );
	}
	_inline_ VWB_VECTOR3 operator/( T const& other ) const// scalar
	{
		T const rec = T( 1 ) / other;
		VWB_VECTOR3 res( x * rec, y * rec, z * rec );
		return res;
	}
	_inline_ VWB_VECTOR3 operator*( VWB_VECTOR3 const& other ) const// cross
	{
		return VWB_VECTOR3( y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x );
	}
	_inline_ VWB_VECTOR3& operator*=( T const other )
	{
		x*= other; 
		y*= other; 
		z*= other; 
		return *this;
	}
	_inline_ VWB_VECTOR3& operator/=( T const other )
	{
		x/= other; 
		y/= other; 
		z/= other; 
		return *this;
	}
	// add and subtract
	_inline_ VWB_VECTOR3 operator+( VWB_VECTOR3 const& other ) const
	{
		VWB_VECTOR3 res( x + other.x, y + other.y, z + other.z );
		return res;
	}
	_inline_ VWB_VECTOR3 operator-( VWB_VECTOR3 const& other ) const
	{
		VWB_VECTOR3 res( x - other.x, y - other.y, z - other.z );
		return res;
	}
	_inline_ VWB_VECTOR3& operator+=( VWB_VECTOR3 const& other )
	{
		x+= other.x; 
		y+= other.y; 
		z+= other.z; 
		return *this;
	}
	_inline_ VWB_VECTOR3& operator-=( VWB_VECTOR3 const& other )
	{
		x-= other.x; 
		y-= other.y; 
		z-= other.z; 
		return *this;
	}
	// with other types
	_inline_ VWB_VECTOR3& operator*=( VWB_MATRIX33<T> const& M )
	{
		T xa = x * M._11 + y * M._21 + z * M._31;
		T ya = x * M._12 + y * M._22 + z * M._32;
		z = x * M._13 + y * M._23 + z * M._33;
		x = xa;
		y = ya;
		return *this;
	}
	_inline_ VWB_VECTOR3& operator*=( VWB_MATRIX<T> const& M )
	{
		T xa = x * M._11 + y * M._21 + z * M._31 + M._41;
		T ya = x * M._12 + y * M._22 + z * M._32 + M._42;
		T w  = T(1) / ( x * M._14 + y * M._24 + z * M._34 + M._44 );
		z    = x * M._13 + y * M._23 + z * M._33 + M._43;
		x = xa * w;
		y = ya * w;
		z*= w;	
		return *this;
	}
	_inline_ VWB_VECTOR3 operator*( VWB_MATRIX33<T> const& M ) const
	{
		return VWB_VECTOR3( 
			x * M._11 + y * M._21 + z * M._31,
			x * M._12 + y * M._22 + z * M._32,
			x * M._13 + y * M._23 + z * M._33 );
	}
	_inline_ VWB_VECTOR3 operator*( VWB_MATRIX<T> const& M ) const
	{
		T w  = T(1) / ( x * M._14 + y * M._24 + z * M._34 + M._44 );
		return VWB_VECTOR3( 
			( x * M._11 + y * M._21 + z * M._31 + M._41 ) * w,
			( x * M._12 + y * M._22 + z * M._32 + M._42 ) * w,
			( x * M._13 + y * M._23 + z * M._33 + M._43 ) * w );
	}


	_inline_ void SetPtr( T* p ) const
	{
		p[0] = x;
		p[1] = y;
		p[2] = z;
	}
	_inline_ VWB_VECTOR3& Normalize()
	{
		T l = T(1) / len();
		if( FLT_MIN < l || -FLT_MIN > l )
		{
			x*= l;
			y*= l;
			z*= l;
		}
		return *this;
	}
	_inline_ VWB_VECTOR3 normalized() const
	{
		VWB_VECTOR3 r;
		T l = T(1) / len();
		r.x = x * l;
		r.y = y * l;
		r.z = z * l;
		return r;
	}

};

template< class T >
struct VWB_VECTOR4
{
	T x;
	T y;
	T z;
	T w;
	_inline_ VWB_VECTOR4() = default;
	_inline_ VWB_VECTOR4( VWB_VECTOR4 const& ) = default;
	template< class _T2>
	_inline_ explicit VWB_VECTOR4( VWB_VECTOR4<_T2> const& other ) : x( (T)other.x ), y( (T)other.y ), z( (T)other.z ), w( (T)other.w ) {}
	template< class _T2>
	_inline_ explicit VWB_VECTOR4( VWB_VECTOR3<_T2> const& other, _T2 _w = _T2(1) ) : x( (T)other.x ), y( (T)other.y ), z( (T)other.z ), w( (T)_w ) {}
	_inline_ explicit VWB_VECTOR4( T const* p ) : x( p[0] ), y( p[1] ), z( p[2] ), w( p[3] ) {}
	_inline_ VWB_VECTOR4( T _x, T _y, T _z, T _w ) : x(_x), y(_y), z(_z), w(_w) {}
	_inline_ static VWB_VECTOR4 const& ptr( T const* p ) { return *(VWB_VECTOR4 const*)p; }
	_inline_ static VWB_VECTOR4& ptr( T* p ) { return *(VWB_VECTOR4*)p; }
	_inline_ static VWB_VECTOR4 O() { return VWB_VECTOR4(0,0,0,0); }
	_inline_ static VWB_VECTOR4 O1() { return VWB_VECTOR4(0,0,0,1); }
	_inline_ static VWB_VECTOR4 Ex() { return VWB_VECTOR4(1,0,0,1); }
	_inline_ static VWB_VECTOR4 Ey() { return VWB_VECTOR4(0,1,0,1); }
	_inline_ static VWB_VECTOR4 Ez() { return VWB_VECTOR4(0,0,1,1); }
	//_inline_ operator VWB_VECTOR3<T>& () { return *(VWB_VECTOR3<T>*)this; } // dangerous...
	//_inline_ operator VWB_VECTOR3<T> const& () const { return *(VWB_VECTOR3<T>*)this; }
	_inline_ operator T const* () const { return &x; }
	_inline_ operator T* () { return &x; }

	_inline_ VWB_VECTOR4 operator-() const { return VWB_VECTOR4( -x, -y, -z, -w ); }
	_inline_ bool operator==(VWB_VECTOR4 const& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }
	_inline_ bool operator!=(VWB_VECTOR4 const& other) const { return x != other.x || y != other.y || z != other.z || w != other.w; }

	_inline_ T lenSq() const { return x * x + y * y + z * z + w * w; }
	_inline_ T len() const { return sqrt( lenSq() ); }
	_inline_ T norm() const { return sqrt( lenSq() ); }

	// multiply and divide
	_inline_ T dot( VWB_VECTOR4 const& other ) const // dot
	{
		T res = x * other.x + y * other.y + z * other.z + w * other.w;
		return res;
	}
	_inline_ VWB_VECTOR4 operator*( T const& other ) const// scalar
	{
		VWB_VECTOR4 res( x * other, y * other, z * other, w * other );
		return res;
	}
	_inline_ VWB_VECTOR4 operator/( T const& other ) const// scalar
	{
		VWB_VECTOR4 res( x / other, y / other, z / other, w / other );
		return res;
	}
	_inline_ VWB_VECTOR4& operator*=( T const other )
	{
		x*= other; 
		y*= other; 
		z*= other; 
		w*= other;
		return *this;
	}
	_inline_ VWB_VECTOR4& operator/=( T const other )
	{
		x/= other; 
		y/= other; 
		z/= other; 
		w/= other;
		return *this;
	}

	// add and subtract
	_inline_ VWB_VECTOR4 operator+( VWB_VECTOR4 const& other ) const
	{
		VWB_VECTOR4 res( x + other.x, y + other.y, z + other.z, w + other.w );
		return res;
	}
	_inline_ VWB_VECTOR4 operator-( VWB_VECTOR4 const& other ) const
	{
		VWB_VECTOR4 res( x - other.x, y - other.y, z - other.z, w - other.w );
		return res;
	}
	_inline_ VWB_VECTOR4& operator+=( VWB_VECTOR4 const& other )
	{
		x+= other.x; 
		y+= other.y; 
		z+= other.z; 
		w+= other.w; 
		return *this;
	}
	_inline_ VWB_VECTOR4& operator-=( VWB_VECTOR4 const& other )
	{
		x-= other.x; 
		y-= other.y; 
		z-= other.z; 
		w-= other.w; 
		return *this;
	}

	_inline_ VWB_VECTOR4& Cons()
	{
		x/= w; 
		y/= w; 
		z/= w; 
		w = 1; 
		return *this;
	}

	_inline_ VWB_VECTOR4& operator*=( VWB_MATRIX<T> const& M )
	{
		T xa = x * M._11 + y * M._21 + z * M._31 + w * M._41;
		T ya = x * M._12 + y * M._22 + z * M._32 + w * M._42;
		T za = x * M._13 + y * M._23 + z * M._33 + w * M._43;
		w  =   x * M._14 + y * M._24 + z * M._34 + w * M._44;
		x = xa;
		y = ya;
		z = za;	
		return *this;
	}
	_inline_ VWB_VECTOR4 operator* ( VWB_MATRIX<T> const& M ) const
	{
		return VWB_VECTOR4( 
			x * M._11 + y * M._21 + z * M._31 + w * M._41,
			x * M._12 + y * M._22 + z * M._32 + w * M._42,
			x * M._13 + y * M._23 + z * M._33 + w * M._43,
			x * M._14 + y * M._24 + z * M._34 + w * M._44 );
	}
	_inline_ void SetPtr( T* p ) const
	{
		p[0] = x;
		p[1] = y;
		p[2] = z;
		p[3] = w;
	}
};

template< class _T >
struct VWB_MATRIX
{
    union {
        struct {
            _T        _11, _12, _13, _14;
            _T        _21, _22, _23, _24;
            _T        _31, _32, _33, _34;
            _T        _41, _42, _43, _44;

        };
        _T m[4][4];
        _T p[16];
    };
	_inline_ VWB_MATRIX() = default;
	_inline_ VWB_MATRIX( VWB_MATRIX const& ) = default;
	template<class _T2>
	_inline_ explicit VWB_MATRIX( VWB_MATRIX33<_T2> const& other ) :
		_11( (_T)other._11 ), _12( (_T)other._12 ), _13( (_T)other._13 ), _14(0),
		_21( (_T)other._21 ), _22( (_T)other._22 ), _23( (_T)other._23 ), _24(0),
		_31( (_T)other._31 ), _32( (_T)other._32 ), _33( (_T)other._33 ), _34(0),
		_41(0), _42(0), _43(0), _44(1) {}
	template<class _T2>
	_inline_ explicit VWB_MATRIX( VWB_MATRIX<_T2> const& other ) : 
		_11( (_T)other._11 ), _12( (_T)other._12 ), _13( (_T)other._13 ), _14( (_T)other._14 ),
		_21( (_T)other._21 ), _22( (_T)other._22 ), _23( (_T)other._23 ), _24( (_T)other._24 ), 
		_31( (_T)other._31 ), _32( (_T)other._32 ), _33( (_T)other._33 ), _34( (_T)other._34 ),
		_41( (_T)other._41 ), _42( (_T)other._42 ), _43( (_T)other._43 ), _44( (_T)other._44 ){}
	_inline_ explicit VWB_MATRIX( _T const* p ) { memcpy( this, p, sizeof( *this ) ); }
	_inline_ VWB_MATRIX( _T f11, _T f12, _T f13, _T f14,
			  _T f21, _T f22, _T f23, _T f24,
			  _T f31, _T f32, _T f33, _T f34,
			  _T f41, _T f42, _T f43, _T f44 ) :
		_11(f11), _12(f12), _13(f13), _14(f14),
		_21(f21), _22(f22), _23(f23), _24(f24),
		_31(f31), _32(f32), _33(f33), _34(f34),
		_41(f41), _42(f42), _43(f43), _44(f44) {}

	_inline_ static VWB_MATRIX & ptr( _T* p ) { return *(VWB_MATRIX*)p; }
	_inline_ static VWB_MATRIX const& ptr( _T const* p ) { return *(VWB_MATRIX const*)p; }
	_inline_ static VWB_MATRIX I() { return VWB_MATRIX( 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1);}
	_inline_ static VWB_MATRIX O() { VWB_MATRIX res; memset( res, 0, sizeof( res ) ); return res;}
	_inline_ static VWB_MATRIX S( VWB_VECTOR4<_T> const& scale) { return VWB_MATRIX( scale.x, 0, 0, 0,  0, scale.y, 0, 0,  0, 0, scale.z, 0,  0, 0, 0, scale.w );}
	_inline_ static VWB_MATRIX S( _T const& sx, _T const& sy, _T const& sz, _T const& sw ) { return VWB_MATRIX( sx, 0, 0, 0,  0, sy, 0, 0,  0, 0, sz, 0,  0, 0, 0, sw );}
	_inline_ static VWB_MATRIX FlipHanddedness() { return VWB_MATRIX( 1,0,0,0,  0,1,0,0, 0,0,-1,0, 0,0,0,1 ); }

	_inline_ static VWB_MATRIX T( VWB_VECTOR3<_T> const& t ) { VWB_MATRIX res( 1,0,0,t.x,0,1,0,t.y,0,0,1,t.z,0,0,0,1); return res;}
	_inline_ static VWB_MATRIX T( _T x, _T y, _T z ) { VWB_MATRIX res( 1,0,0,x,0,1,0,y,0,0,1,z,0,0,0,1); return res;}

	_inline_ static VWB_MATRIX Rx( _T x ) { return VWB_MATRIX( VWB_MATRIX33<_T>::Rx( x ) ); }
	_inline_ static VWB_MATRIX Ry( _T y ) { return VWB_MATRIX( VWB_MATRIX33<_T>::Ry( y ) ); }
	_inline_ static VWB_MATRIX Rz( _T z ) { return VWB_MATRIX( VWB_MATRIX33<_T>::Rz( z ) ); }

	// this is Rz(z) * Rx(x) * Ry(y)
	_inline_ static VWB_MATRIX R( _T x, _T y, _T z ) {	return VWB_MATRIX( VWB_MATRIX33<_T>::R( x, y, z ) ); }
	_inline_ static VWB_MATRIX R( VWB_VECTOR3<_T> r ) { return R( r.x, r.y, r.z ); }

	_inline_ static VWB_MATRIX Rx_LH( _T x ) { return VWB_MATRIX::Rx( -x ); }
	_inline_ static VWB_MATRIX Ry_LH( _T y ) { return VWB_MATRIX::Ry( -y ); }
	_inline_ static VWB_MATRIX Rz_LH( _T z ) { return VWB_MATRIX::Rz( -z ); }

	// this is Ry_T(y) * Rx_T(x) * Rz_T(z)
	_inline_ static VWB_MATRIX R_LH( _T x, _T y, _T z ) { return VWB_MATRIX::R( -x, -y, -z ); }
	_inline_ static VWB_MATRIX R_LH( VWB_VECTOR3<_T> r ) { return R_LH( r.x, r.y, r.z ); }

	// DX frusta, to be multiplied from right side "pre-multiplied": v' = v * P
	_inline_ static VWB_MATRIX DXFrustumLH( _T const* pClip )
	{
		_T TwoNearZ = pClip[4] + pClip[4];
		_T ReciprocalWidth = _T(1) / ( pClip[0] + pClip[2] );
		_T ReciprocalHeight = _T(1) / ( pClip[1] + pClip[3] );
		_T fRange = pClip[5] / ( pClip[5] - pClip[4] );
		
		// nDisplay: Support unlimited far plane (f==n)
		const _T n = pClip[4];
		const _T f = pClip[5];
		static const _T Z_PRECISION = 0;
		const _T mc = (f == n) ? (1.0f - Z_PRECISION) : fRange;
		const _T md = (f == n) ? (-n * (1.0f - Z_PRECISION)) : (-fRange * pClip[4]);

		return VWB_MATRIX(
			TwoNearZ * ReciprocalWidth,
			(_T)0,
			(_T)0,
			(_T)0,

			(_T)0,
			TwoNearZ * ReciprocalHeight,
			(_T)0,
			(_T)0,

			( pClip[0] - pClip[2] ) * ReciprocalWidth,
			( pClip[3] - pClip[1] ) * ReciprocalHeight,
			mc,
			(_T)1,

			(_T)0,
			(_T)0,
			md,
			(_T)0 );
	}

	_inline_ static VWB_MATRIX DXFrustumRH( _T const* pClip )
	{
		VWB_MATRIX P = DXFrustumLH( pClip );
		return VWB_MATRIX(  P._11,  P._12,  P._13,  P._14, 
						    P._21,  P._22,  P._23,  P._24, 
						   -P._31, -P._32, -P._33, -P._34, 
						    P._41,  P._42,  P._43,  P._44 );
	}

	// GL frusta to be multiplied from left side "post-multiplied": v' = P * v
	_inline_ static VWB_MATRIX GLFrustumRH( _T const* pClip )
	{
		VWB_MATRIX P = DXFrustumLH( pClip );
		return VWB_MATRIX(
			P._11, P._21, -P._31, P._41,
			P._12, P._22, -P._32, P._42,
			P._13, P._24, -( pClip[5] + pClip[4] ) / pClip[5] * P._33, _T(2) * P._43,
			P._14, P._24, -P._34, P._44
			);
	}

	_inline_ static VWB_MATRIX GLFrustumLH( _T const* pClip )
	{
		VWB_MATRIX P = GLFrustumRH( pClip );
		return VWB_MATRIX( P._11,  P._12, -P._13,  P._14,
						   P._21,  P._22, -P._23,  P._24,
						  -P._31, -P._32, -P._33, -P._34,
						   P._41,  P._42, -P._43,  P._44 );
	}

	_inline_ static VWB_MATRIX DXOrthoLH( _T const* pClip )
	{
		_T ReciprocalWidth = _T( 1 ) / ( pClip[0] + pClip[2] );
		_T ReciprocalHeight = _T( 1 ) / ( pClip[1] + pClip[3] );
		_T fRange = _T(1) / ( pClip[5] - pClip[4] );

		return VWB_MATRIX(
			ReciprocalWidth + ReciprocalWidth,
			(_T)0,
			(_T)0,
			(_T)0,

			(_T)0,
			ReciprocalHeight + ReciprocalHeight,
			(_T)0,
			(_T)0,

			(_T)0,
			(_T)0,
			fRange,
			(_T)0,

			( pClip[0] - pClip[2] ) * ReciprocalWidth,
			( pClip[3] - pClip[1] ) * ReciprocalHeight,
			-fRange * pClip[4],
			(_T)1 );
	}

	_inline_ static VWB_MATRIX DXOrthoRH( _T const* pClip )
	{
		VWB_MATRIX P = DXOrthoLH( pClip );
		return VWB_MATRIX(  P._11,  P._12,  P._13,  P._14,
						    P._21,  P._22,  P._23,  P._24,
						   -P._31, -P._32, -P._33, -P._34,
						    P._41,  P._42,  P._43,  P._44 );
	}

	_inline_ static VWB_MATRIX GLOrthoRH( _T const* pClip )
	{
		_T ReciprocalWidth = _T( 1 ) / ( pClip[0] + pClip[2] );
		_T ReciprocalHeight = _T( 1 ) / ( pClip[1] + pClip[3] );
		_T fRange = _T( 1 ) / ( pClip[5] - pClip[4] );

		return VWB_MATRIX(
			ReciprocalWidth + ReciprocalWidth,
			(_T)0,
			(_T)0,
			( pClip[0] - pClip[2] ) * ReciprocalWidth,

			(_T)0,
			ReciprocalHeight + ReciprocalHeight,
			(_T)0,
			( pClip[3] - pClip[1] )* ReciprocalHeight,

			(_T)0,
			(_T)0,
			(_T)-2 * fRange,
			( pClip[4] + pClip[5] ) * fRange,
		
			(_T)0,
			(_T)0,
			(_T)0,
			(_T)1 );
	}

	_inline_ static VWB_MATRIX GLOrthoLH( _T const* pClip )
	{
		VWB_MATRIX P = GLOrthoRH( pClip );
		return VWB_MATRIX( P._11, P._12, -P._13, P._14,
						   P._21, P._22, -P._23, P._24,
						   P._31, P._32, -P._33, P._34,
						   P._41, P._42, -P._43, P._44 );
	}


	_inline_ _T& operator()(unsigned int row, unsigned int col) { return m[row][col]; }
	_inline_ _T const& operator()(unsigned int row, unsigned int col) const { return m[row][col]; }
	_inline_ operator _T* () { return &_11; }
	_inline_ operator _T const* () const { return &_11; }
	//_inline_ _T* operator[]( int i ) { return m[i]; }
	//_inline_ _T* const* operator[]( int i ) const { return m[i]; }

	_inline_ VWB_MATRIX operator-() const { return VWB_MATRIX( -_11, -_12, -_13, -_14,
															   -_21, -_22, -_23, -_24,
															   -_31, -_32, -_33, -_34,
                                                               -_41, -_42, -_43, -_44 ); }
	_inline_ bool operator==(VWB_MATRIX const& other) const 
	{ 
		for( int i = 0; i != 16; i++ ) if( p[i] != other.p[i] ) return false;
		return true; 
	}
	_inline_ bool operator!=(VWB_MATRIX const& other) const 
	{ 
		for( int i = 0; i != 16; i++ ) if( p[i] != other.p[i] ) return true;
		return false; 
	}

	_inline_ VWB_MATRIX operator*( _T const other ) const
	{
		VWB_MATRIX res( _11 * other, _12 * other, _13 * other, _14 * other, 
					  _21 * other, _22 * other, _23 * other, _24 * other, 
					  _31 * other, _32 * other, _33 * other, _34 * other, 
					  _41 * other, _42 * other, _43 * other, _44 * other );
		return res;
	}
	_inline_ VWB_MATRIX& operator*=( _T const other )
	{
		_11*= other; _12*= other; _13*= other; _14*= other; 
		_21*= other; _22*= other; _23*= other; _24*= other; 
		_31*= other; _32*= other; _33*= other; _34*= other; 
		_41*= other; _42*= other; _43*= other; _44*= other;
		return *this;
	}
	_inline_ VWB_MATRIX& operator*=( VWB_MATRIX const& other )
	{
		VWB_MATRIX M = *this;
		*this = M * other;
		return *this;
	}
	_inline_ VWB_MATRIX operator*( VWB_MATRIX const& other ) const
	{
		VWB_MATRIX res;
		for( int i = 0; i != 4; i++ )
			for( int j = 0; j != 4; j++ )
			{
				res.m[i][j] = m[i][0] * other.m[0][j];
				for( int k = 1; k != 4; k++ )
					res.m[i][j]+= m[i][k] * other.m[k][j];
			}
		return res;
	}
	_inline_ VWB_VECTOR3<_T> operator*( VWB_VECTOR3<_T> const& other ) const
	{
		VWB_VECTOR3<_T> res;
		res.x = _11 * other.x + _12 * other.y + _13 * other.z + _14;
		res.y = _21 * other.x + _22 * other.y + _23 * other.z + _24;
		res.z = _31 * other.x + _32 * other.y + _33 * other.z + _34;
		_T w = _T(1) / ( _41 * other.x + _42 * other.y + _43 * other.z + _44 );
		res.x*= w;
		res.y*= w;
		res.z*= w;
		return res;
	}
	_inline_ VWB_VECTOR4<_T> operator*( VWB_VECTOR4<_T> const& other ) const
	{
		VWB_VECTOR4<_T> res;
		res.x = _11 * other.x + _12 * other.y + _13 * other.z + _14 * other.w;
		res.y = _21 * other.x + _22 * other.y + _23 * other.z + _24 * other.w;
		res.z = _31 * other.x + _32 * other.y + _33 * other.z + _34 * other.w;
		res.w = _41 * other.x + _42 * other.y + _43 * other.z + _44 * other.w;
		return res;
	}

	_inline_ VWB_MATRIX& operator+=( VWB_MATRIX const& other )
	{
		for( int i = 0; i != 16; i++ )
			p[i] += other.p[i];
		return *this;
	}
	_inline_ VWB_MATRIX& operator-=( VWB_MATRIX const& other )
	{
		for( int i = 0; i != 16; i++ )
			p[i] -= other.p[i];
		return *this;
	}
	_inline_ VWB_MATRIX operator+( VWB_MATRIX const& other ) const
	{
		VWB_MATRIX res;
		for( int i = 0; i != 16; i++ )
			res[i] = p[i] + other.p[i];
		return res;
	}
	_inline_ VWB_MATRIX operator-( VWB_MATRIX const& other ) const
	{
		VWB_MATRIX res;
		for( int i = 0; i != 16; i++ )
			res[i] = p[i] - other.p[i];
		return res;
	}

	_inline_ void SetPtr( _T* p ) const
	{
		memcpy( p, this, sizeof( *this ) );
	}

	_inline_ VWB_MATRIX& Transpose()
	{
		_T a = _12;
		_12 = _21;
		_21 = a;
		
		a = _13;
		_13 = _31;
		_31 = a;

		a = _14;
		_14 = _41;
		_41 = a;

		a = _23;
		_23 = _32;
		_32 = a;

		a = _24;
		_24 = _42;
		_42 = a;

		a = _34;
		_34 = _43;
		_43 = a;

		return *this;
	}

	_T Det() const
	{
		_T inv0 = p[5] * p[10] * p[15] -
			p[5] * p[11] * p[14] -
			p[9] * p[6] * p[15] +
			p[9] * p[7] * p[14] +
			p[13] * p[6] * p[11] -
			p[13] * p[7] * p[10];

		_T inv4 = -p[4] * p[10] * p[15] +
			p[4] * p[11] * p[14] +
			p[8] * p[6] * p[15] -
			p[8] * p[7] * p[14] -
			p[12] * p[6] * p[11] +
			p[12] * p[7] * p[10];

		_T inv8 = p[4] * p[9] * p[15] -
			p[4] * p[11] * p[13] -
			p[8] * p[5] * p[15] +
			p[8] * p[7] * p[13] +
			p[12] * p[5] * p[11] -
			p[12] * p[7] * p[9];

		_T inv12 = -p[4] * p[9] * p[14] +
			p[4] * p[10] * p[13] +
			p[8] * p[5] * p[14] -
			p[8] * p[6] * p[13] -
			p[12] * p[5] * p[10] +
			p[12] * p[6] * p[9];

		_T det = p[0] * inv0 + p[1] * inv4 + p[2] * inv8 + p[3] * inv12;

		return det;
	}

	// get a single scalar weight
	_inline_ _T Norm() const { 
		_T l = p[0] * p[0];
		for( int i = 1; i != 16; i++ )
			l += p[i] * p[i];
		return sqrt( l ); 
	}


	_inline_ VWB_MATRIX Transposed() const { return VWB_MATRIX ( _11, _21, _31, _41, _12, _22, _32, _42, _13, _23, _33, _43,_14, _24, _34, _44 ); }

	_inline_ VWB_MATRIX& Invert()
	{
		*this = Inverted();
		return *this;
	}
	_inline_ VWB_MATRIX Inverted() const
	{
		VWB_MATRIX inv;
		_T det;
		int i;

		inv[0] = p[5]  * p[10] * p[15] - 
				 p[5]  * p[11] * p[14] - 
				 p[9]  * p[6]  * p[15] + 
				 p[9]  * p[7]  * p[14] +
				 p[13] * p[6]  * p[11] - 
				 p[13] * p[7]  * p[10];

		inv[4] = -p[4]  * p[10] * p[15] + 
				  p[4]  * p[11] * p[14] + 
				  p[8]  * p[6]  * p[15] - 
				  p[8]  * p[7]  * p[14] - 
				  p[12] * p[6]  * p[11] + 
				  p[12] * p[7]  * p[10];

		inv[8] = p[4]  * p[9] * p[15] - 
				 p[4]  * p[11] * p[13] - 
				 p[8]  * p[5] * p[15] + 
				 p[8]  * p[7] * p[13] + 
				 p[12] * p[5] * p[11] - 
				 p[12] * p[7] * p[9];

		inv[12] = -p[4]  * p[9] * p[14] + 
				   p[4]  * p[10] * p[13] +
				   p[8]  * p[5] * p[14] - 
				   p[8]  * p[6] * p[13] - 
				   p[12] * p[5] * p[10] + 
				   p[12] * p[6] * p[9];

		inv[1] = -p[1]  * p[10] * p[15] + 
				  p[1]  * p[11] * p[14] + 
				  p[9]  * p[2] * p[15] - 
				  p[9]  * p[3] * p[14] - 
				  p[13] * p[2] * p[11] + 
				  p[13] * p[3] * p[10];

		inv[5] = p[0]  * p[10] * p[15] - 
				 p[0]  * p[11] * p[14] - 
				 p[8]  * p[2] * p[15] + 
				 p[8]  * p[3] * p[14] + 
				 p[12] * p[2] * p[11] - 
				 p[12] * p[3] * p[10];

		inv[9] = -p[0]  * p[9] * p[15] + 
				  p[0]  * p[11] * p[13] + 
				  p[8]  * p[1] * p[15] - 
				  p[8]  * p[3] * p[13] - 
				  p[12] * p[1] * p[11] + 
				  p[12] * p[3] * p[9];

		inv[13] = p[0]  * p[9] * p[14] - 
				  p[0]  * p[10] * p[13] - 
				  p[8]  * p[1] * p[14] + 
				  p[8]  * p[2] * p[13] + 
				  p[12] * p[1] * p[10] - 
				  p[12] * p[2] * p[9];

		inv[2] = p[1]  * p[6] * p[15] - 
				 p[1]  * p[7] * p[14] - 
				 p[5]  * p[2] * p[15] + 
				 p[5]  * p[3] * p[14] + 
				 p[13] * p[2] * p[7] - 
				 p[13] * p[3] * p[6];

		inv[6] = -p[0]  * p[6] * p[15] + 
				  p[0]  * p[7] * p[14] + 
				  p[4]  * p[2] * p[15] - 
				  p[4]  * p[3] * p[14] - 
				  p[12] * p[2] * p[7] + 
				  p[12] * p[3] * p[6];

		inv[10] = p[0]  * p[5] * p[15] - 
				  p[0]  * p[7] * p[13] - 
				  p[4]  * p[1] * p[15] + 
				  p[4]  * p[3] * p[13] + 
				  p[12] * p[1] * p[7] - 
				  p[12] * p[3] * p[5];

		inv[14] = -p[0]  * p[5] * p[14] + 
				   p[0]  * p[6] * p[13] + 
				   p[4]  * p[1] * p[14] - 
				   p[4]  * p[2] * p[13] - 
				   p[12] * p[1] * p[6] + 
				   p[12] * p[2] * p[5];

		inv[3] = -p[1] * p[6] * p[11] + 
				  p[1] * p[7] * p[10] + 
				  p[5] * p[2] * p[11] - 
				  p[5] * p[3] * p[10] - 
				  p[9] * p[2] * p[7] + 
				  p[9] * p[3] * p[6];

		inv[7] = p[0] * p[6] * p[11] - 
				 p[0] * p[7] * p[10] - 
				 p[4] * p[2] * p[11] + 
				 p[4] * p[3] * p[10] + 
				 p[8] * p[2] * p[7] - 
				 p[8] * p[3] * p[6];

		inv[11] = -p[0] * p[5] * p[11] + 
				   p[0] * p[7] * p[9] + 
				   p[4] * p[1] * p[11] - 
				   p[4] * p[3] * p[9] - 
				   p[8] * p[1] * p[7] + 
				   p[8] * p[3] * p[5];

		inv[15] = p[0] * p[5] * p[10] - 
				  p[0] * p[6] * p[9] - 
				  p[4] * p[1] * p[10] + 
				  p[4] * p[2] * p[9] + 
				  p[8] * p[1] * p[6] - 
				  p[8] * p[2] * p[5];

		det = p[0] * inv[0] + p[1] * inv[4] + p[2] * inv[8] + p[3] * inv[12];

		det = ((_T)1) / det;

		for (i = 0; i < 16; i++)
			inv[i]*= det;

		return inv;
	}

	_inline_ VWB_VECTOR4<_T>& X() { return *(VWB_VECTOR4<_T>*)&_11; }
	_inline_ VWB_VECTOR4<_T> const& X() const { return *(VWB_VECTOR4<_T>*)&_11; }
	_inline_ VWB_VECTOR4<_T>& Y() { return *(VWB_VECTOR4<_T>*)&_21; }
	_inline_ VWB_VECTOR4<_T> const& Y() const { return *(VWB_VECTOR4<_T>*)&_21; }
	_inline_ VWB_VECTOR4<_T>& Z() { return *(VWB_VECTOR4<_T>*)&_31; }
	_inline_ VWB_VECTOR4<_T> const& Z() const { return *(VWB_VECTOR4<_T>*)&_31; }
	_inline_ VWB_VECTOR4<_T>& W() { return *(VWB_VECTOR4<_T>*)&_41; }
	_inline_ VWB_VECTOR4<_T> const& W() const { return *(VWB_VECTOR4<_T>*)&_41; }
	_inline_ VWB_MATRIX33<_T> Upper() const
	{
		return VWB_MATRIX33<_T>( _11, _12, _13,
								 _21, _22, _23,
								 _31, _32, _33 );
	}
	_inline_ void SetUpper( VWB_MATRIX33<_T> const& other )
	{
		_11 = other._11; _12 = other._12; _13 = other._13;
		_21 = other._21; _22 = other._22; _23 = other._23;
		_31 = other._31; _32 = other._32; _33 = other._33;
	}

	_inline_ bool IsZero() const
	{
		for( int i = 0; i != 16; i++ )
			if( -std::numeric_limits<_T>::epsilon() > +p[i] || p[i] > std::numeric_limits<_T>::epsilon() )
				return false;
		return true;
	}

	_inline_ bool IsIdentity()
	{
		for( int i = 0; i != 15;  )
		{
			if( T( 1 ) - std::numeric_limits<_T>::epsilon() > p[i] || p[i] > T( 1 ) + std::numeric_limits<_T>::epsilon() )
				return false;
			for( int iE = ++i + 4; i != iE; i++ )
				if( -std::numeric_limits<_T>::epsilon() > p[i] || p[i] > std::numeric_limits<_T>::epsilon() )
					return false;
		}
		if( T( 1 ) - std::numeric_limits<_T>::epsilon() > p[15] || p[15] > T( 1 ) + std::numeric_limits<_T>::epsilon() )
			return false;
		return true;
	}
};

template< class _T >
struct VWB_MATRIX33
{
    union {
        struct {
            _T        _11, _12, _13;
            _T        _21, _22, _23;
            _T        _31, _32, _33;

        };
        _T m[3][3];
		_T p[9];
    };
	_inline_ VWB_MATRIX33() = default;
	_inline_ VWB_MATRIX33( VWB_MATRIX33 const& ) = default;
	_inline_ explicit VWB_MATRIX33( VWB_MATRIX<_T> const& other ) : 
		_11( other._11 ), _12( other._12 ), _13( other._13 ),
		_21( other._21 ), _22( other._22 ), _23( other._23 ), 
		_31( other._31 ), _32( other._32 ), _33( other._33 ) {}
	template<class _T2>
	_inline_ explicit VWB_MATRIX33( VWB_MATRIX33<_T2> const& other ) : 
		_11( (_T)other._11 ), _12( (_T)other._12 ), _13( (_T)other._13 ),
		_21( (_T)other._21 ), _22( (_T)other._22 ), _23( (_T)other._23 ), 
		_31( (_T)other._31 ), _32( (_T)other._32 ), _33( (_T)other._33 ){}
	_inline_ explicit VWB_MATRIX33( _T const* p ) { memcpy( this, p, sizeof( *this ) ); }
	_inline_ VWB_MATRIX33( _T f11, _T f12, _T f13,
			  _T f21, _T f22, _T f23,
			  _T f31, _T f32, _T f33 )
	{
		_11 = f11; _12 = f12; _13 = f13;
		_21 = f21; _22 = f22; _23 = f23;
		_31 = f31; _32 = f32; _33 = f33;
	}
	_inline_ static VWB_MATRIX33 const& ptr( _T const* p ) { return *(VWB_MATRIX33 const*)p; }
	_inline_ static VWB_MATRIX33& ptr( _T* p ) { return *(VWB_MATRIX33*)p; }
	_inline_ static VWB_MATRIX33 I() { VWB_MATRIX33 res( 1,0,0, 0,1,0, 0,0,1 ); return res;}
	_inline_ static VWB_MATRIX33 O() { VWB_MATRIX33 res; memset( res, 0, sizeof( res ) ); return res;}
	_inline_ static VWB_MATRIX33 S( VWB_VECTOR3<_T> const& scale) { return VWB_MATRIX33( scale.x, 0, 0,  0, scale.y, 0,  0, 0, scale.z );}
	_inline_ static VWB_MATRIX33 S( _T const& sx, _T const& sy, _T const& sz ) { return VWB_MATRIX33( sx, 0, 0,  0, sy, 0,  0, 0, sz );}

	// DONE:
	_inline_ static VWB_MATRIX33 Rx( _T x ) {
		const _T sx = std::sin( x );
		const _T cx = std::cos( x );
		return VWB_MATRIX33( 1, 0, 0,   0, cx, sx,   0, -sx, cx ); 
	}
	_inline_ static VWB_MATRIX33 Ry( _T y ) { 
		const _T sy = std::sin( y );
		const _T cy = std::cos( y );
		return VWB_MATRIX33( cy, 0, sy,   0, 1, 0,   -sy, 0, cy ); 
	}
	_inline_ static VWB_MATRIX33 Rz( _T z ) { 
		const _T sz = std::sin( z );
		const _T cz = std::cos( z );
		return VWB_MATRIX33( cz, -sz, 0,   sz, cz, 0,   0, 0, 1 );
	}

	_inline_ static VWB_MATRIX33 R( _T x, _T y, _T z )  // this is Rz(z) * Rx(x) * Ry(y)
	{ 
		const _T sx = std::sin( x );
		const _T sy = std::sin( y );
		const _T sz = std::sin( z );
		const _T cx = std::cos( x );
		const _T cy = std::cos( y );
		const _T cz = std::cos( z );
		return VWB_MATRIX33( 
			cy * cz + sx * sy * sz,		-cx * sz,		cz * sy - cy * sx * sz,
			cy * sz - cz * sx * sy,		 cx * cz,		cy * cz * sx + sy * sz,
			-cx * sy,					-sx,			cx * cy
		);
	}
	_inline_ static VWB_MATRIX33 R( VWB_VECTOR3<_T> r ) { return R( r.x, r.y, r.z ); }

	// transposed matrices
	_inline_ static VWB_MATRIX33 Rx_LH( _T x ) { return VWB_MATRIX33::Rx( -x ); }
	_inline_ static VWB_MATRIX33 Ry_LH( _T y ) { return VWB_MATRIX33::Ry( -y ); }
	_inline_ static VWB_MATRIX33 Rz_LH( _T z ) { return VWB_MATRIX33::Rz( -z ); }
	// this is RyLHT(y) * RxLHT(x) * RzLHT(z)
	_inline_ static VWB_MATRIX33 R_LH( _T x, _T y, _T z ) { return VWB_MATRIX33::R( -x, -y, -z ); }
	_inline_ static VWB_MATRIX33 R_LH( VWB_VECTOR3<_T> r ) { return R_LH( r.x, r.y, r.z ); }

	// give device x and y axis
	_inline_ static VWB_MATRIX33 Base( VWB_VECTOR3<_T> dx, VWB_VECTOR3<_T> dy )
	{
		VWB_MATRIX33 res;
		res._11 = dx.x;
		res._12 = dx.y;
		res._13 = dx.z;
		// x * y gives us a view direction aka base z in right handed system
		res._31 = dx.y * dy.z - dx.z * dy.y;
		res._32 = dx.z * dy.x - dx.x * dy.z;
		res._33 = dx.x * dy.y - dx.y * dy.x;
		// x * z gives us a up aka y
		res._21 = dx.z * res._32 - dx.y * res._33;
		res._22 = dx.x * res._33 - dx.z * res._31;
		res._23 = dx.y * res._31 - dx.x * res._32;
		res.X().Normalize();
		res.Y().Normalize();
		res.Z().Normalize();
		return res;
	}

	_inline_ _T& operator()(unsigned int row, unsigned int col) { return m[row][col]; }
	_inline_ _T const& operator()(unsigned int row, unsigned int col) const { return m[row][col]; }
	_inline_ operator _T* () { return &_11; }
	_inline_ operator _T const* () const { return &_11; }
	//_inline_ _T* operator[]( int i ) { return m[i]; }
	//_inline_ _T const* operator[]( int i ) const { return m[i]; }

	_inline_ VWB_MATRIX33 operator-() const { return VWB_MATRIX33( -_11, -_12, -_13,
																   -_21, -_22, -_23,
                                                                  -_31, -_32, -_33 ); }
	_inline_ bool operator==(VWB_MATRIX33 const& other) const 
	{ 
		for( int i = 0; i != 9; i++ ) if( p[i] != other.p[i] ) return false;
		return true; 
	}
	_inline_ bool operator!=(VWB_MATRIX33 const& other) const 
	{ 
		for( int i = 0; i != 9; i++ ) if( p[i] != other.p[i] ) return true;
		return false; 
	}

	_inline_ VWB_MATRIX33 operator*( _T const other ) const
	{
		return VWB_MATRIX33( _11 * other, _12 * other, _13 * other,
					  _21 * other, _22 * other, _23 * other,
					  _31 * other, _32 * other, _33 * other);
	}
	_inline_ VWB_MATRIX33& operator*=( _T const other )
	{
		_11*= other; _12*= other; _13*= other;
		_21*= other; _22*= other; _23*= other;
		_31*= other; _32*= other; _33*= other;
		return *this;
	}
	_inline_ VWB_MATRIX33& operator*=( VWB_MATRIX33 const& other )
	{
		VWB_MATRIX33 M = *this;
		*this = M * other;
		return *this;
	}
	_inline_ VWB_MATRIX33 operator*( VWB_MATRIX33 const& other ) const
	{
		VWB_MATRIX33 res;
		for( int i = 0; i != 3; i++ )
			for( int j = 0; j != 3; j++ )
			{
				res.m[i][j] = m[i][0] * other.m[0][j];
				for( int k = 1; k != 3; k++ )
					res.m[i][j]+= m[i][k] * other.m[k][j];
			}
		return res;
	}
	_inline_ VWB_VECTOR3<_T> operator*( VWB_VECTOR3<_T> const& other ) const
	{
		return VWB_VECTOR3<_T>(
			_11 * other.x + _12 * other.y + _13 * other.z,
			_21 * other.x + _22 * other.y + _23 * other.z,
			_31 * other.x + _32 * other.y + _33 * other.z );
	}

	_inline_ VWB_MATRIX33& operator+=( VWB_MATRIX33 const& other )
	{
		for( int i = 0; i != 9; i++ )
			p[i] += other.p[i];
		return *this;
	}
	_inline_ VWB_MATRIX33& operator-=( VWB_MATRIX33 const& other )
	{
		for( int i = 0; i != 9; i++ )
			p[i] -= other.p[i];
		return *this;
	}
	_inline_ VWB_MATRIX33 operator+( VWB_MATRIX33 const& other ) const
	{
		VWB_MATRIX33 res;
		for( int i = 0; i != 9; i++ )
			res[i] = p[i] + other.p[i];
		return res;
	}
	_inline_ VWB_MATRIX33 operator-( VWB_MATRIX33 const& other ) const
	{
		VWB_MATRIX33 res;
		for( int i = 0; i != 9; i++ )
			res[i] = p[i] - other.p[i];
		return res;
	}

	_inline_ void SetPtr( _T* p ) const
	{
		memcpy( p, this, sizeof( *this ) );
	}

	// gets euler angles from a GL-style 3x3 rotation matrix in (right-handed coordinate system, z backward, y up and x right)
	// assumptions:
	// positive rotation around x turns (pitch) up
	// positive rotation around y turns (yaw) right
	// positive rotation around z turns (roll) clockwise
	// rotation order is y-x-z, this corresponds to R()
	_inline_ VWB_VECTOR3<_T> GetR() const
	{
		VWB_VECTOR3<_T> a; 
		a.y = -atan2( p[6], p[8] );
		_T s = std::sin( a.y );
		_T c = std::cos( a.y );
		_T l = sqrt( p[6] * p[6] + p[8] * p[8] );
		a.x = -atan2( p[7], l );
		a.z = atan2( s * p[5] + c * p[3], s * p[2] + c * p[0] );
		return a;
	}

	_inline_ VWB_MATRIX33& Transpose()
	{
		_T a = _12;
		_12 = _21;
		_21 = a;
		
		a = _13;
		_13 = _31;
		_31 = a;

		a = _23;
		_23 = _32;
		_32 = a;

		return *this;
	}

	_inline_ VWB_MATRIX33 Transposed() const	{ return VWB_MATRIX33 ( _11, _21, _31,  _12, _22, _32,  _13, _23, _33 ); }

	// get a single scalar weight
	_inline_ _T Norm() const {
		_T l = p[0] * p[0];
		for( int i = 1; i != 9; i++ )
			l += p[i] * p[i];
		return sqrt( l );
	}

	_inline_ VWB_MATRIX33 Det() const
	{
		_T x = p[4] * p[8] - p[5] * p[7];
		_T y = p[3] * p[8] - p[5] * p[6];
		_T z = p[3] * p[7] - p[4] * p[6];
		_T det = p[0] * x - p[1] * y + p[2] * z;
		return det;
	}

	_inline_ VWB_MATRIX33& Invert()
	{
		*this = Inverted();
		return *this;
	}

	_inline_ VWB_MATRIX33 Inverted() const
	{
		VWB_MATRIX33 pA;
		_T x = p[4] * p[8] - p[5] * p[7];
		_T y = p[3] * p[8] - p[5] * p[6];
		_T z = p[3] * p[7] - p[4] * p[6];
		_T det = p[0] * x - p[1] * y + p[2] * z;

		det = ((_T)1)/det;

		pA[0] = x * det;
		pA[1] = -( p[1] * p[8] - p[2] * p[7] ) * det;
		pA[2] = ( p[1] * p[5] - p[2] * p[4] ) * det;
		pA[3] = -y * det;
		pA[4] = ( p[0] * p[8] - p[2] * p[6] ) * det;
		pA[5] = -( p[0] * p[5] - p[2] * p[3] ) * det;
		pA[6] = z * det;
		pA[7] = -( p[0] * p[7] - p[1] * p[6] ) * det;
		pA[8] = ( p[0] * p[4] - p[3] * p[1] ) * det;

		return pA;
	}

	_inline_ VWB_MATRIX33& Normalize()
	{
		_T l = _11 * _11;
		for( int i = 1; i != 9; i++ )
			l+= p[i] * p[i];
		l = sqrt(l/3);
		if( FLT_MIN < l || -FLT_MIN > l )
		{
			for( int i = 0; i != 9; i++ )
				p[i]/= l;
		}
		return *this;
	}
	_inline_ VWB_VECTOR3<_T>& X() { return *(VWB_VECTOR3<_T>*)&_11; }
	_inline_ VWB_VECTOR3<_T> const& X() const { return *(VWB_VECTOR3<_T>*)&_11; }
	_inline_ VWB_VECTOR3<_T>& Y() { return *(VWB_VECTOR3<_T>*)&_21; }
	_inline_ VWB_VECTOR3<_T> const& Y() const { return *(VWB_VECTOR3<_T>*)&_21; }
	_inline_ VWB_VECTOR3<_T>& Z() { return *(VWB_VECTOR3<_T>*)&_31; }
	_inline_ VWB_VECTOR3<_T> const& Z() const { return *(VWB_VECTOR3<_T>*)&_31; }

	_inline_ bool IsZero() const
	{
		for( int i = 0; i != 9; i++ )
			if( -std::numeric_limits<_T>::epsilon() > p[i] || p[i] > std::numeric_limits<_T>::epsilon() )
				return false;
		return true;
	}

	// checks if a matrix is really close to identity matrix.
    // it often happens after a series of transformations, it comes clost but not really 1.0
    // for testing
	_inline_ bool IsIdentity()
	{
		for( int i = 0; i != 8; )
		{
			if( _T( 1 ) - std::numeric_limits<_T>::epsilon() > p[i] || p[i] > _T( 1 ) + std::numeric_limits<_T>::epsilon() )
				return false;
			for( int iE = ++i + 3; i != iE; i++ )
				if( -std::numeric_limits<_T>::epsilon() > p[i] || p[i] > std::numeric_limits<_T>::epsilon() )
					return false;
		}
		if( _T( 1 ) - std::numeric_limits<_T>::epsilon() > p[8] || p[8] > _T( 1 ) + std::numeric_limits<_T>::epsilon() )
			return false;
		return true;
	}

};

template< class T >
struct VWB_BOX
{
	VWB_VECTOR3<T> vMin, vMax;
	_inline_ VWB_BOX() = default;
	_inline_ VWB_BOX( VWB_BOX const& ) = default;
	_inline_ VWB_BOX( VWB_VECTOR3<T> const& _tl, VWB_VECTOR3<T> const& _br ) : vMin( _tl ), vMax( _br ) {}
	_inline_ VWB_BOX& operator+=( VWB_BOX const& other )
	{
		if( other.vMin[0] < vMin[0] )
			vMin[0] = other.vMin[0];
		if( other.vMax[0] > vMax[0] )
			vMax[0] = other.vMax[0];

		if( other.vMin[1] < vMin[1] )
			vMin[1] = other.vMin[1];
		if( other.vMax[1] > vMax[1] )
			vMax[1] = other.vMax[1];

		if( other.vMin[2] < vMin[2] )
			vMin[2] = other.vMin[2];
		if( other.vMax[2] > vMax[2] )
			vMax[2] = other.vMax[2];
		return *this;
	};
	_inline_ VWB_BOX& operator+=( VWB_VECTOR3<T> const& other )
	{
		if( other[0] < vMin[0] )
			vMin[0] = other[0];
		if( other[0] > vMax[0] )
			vMax[0] = other[0];

		if( other[1] < vMin[1] )
			vMin[1] = other[1];
		if( other[1] > vMax[1] )
			vMax[1] = other[1];

		if( other[2] < vMin[2] )
			vMin[2] = other[2];
		if( other[2] > vMax[2] )
			vMax[2] = other[2];
		return *this;
	};
	_inline_ VWB_BOX operator+( VWB_BOX const& other ) const
	{
		VWB_BOX r( *this );
		if( other.vMin[0] < r.vMin[0] )
			r.vMin[0] = other.r.tl[0];
		if( other.vMax[0] > r.vMax[0] )
			r.vMax[0] = other.vMax[0];

		if( other.vMin[1] < r.vMin[1] )
			r.vMin[1] = other[1];
		if( other.vMax[1] > r.vMax[1] )
			r.vMax[1] = other.vMax[1];

		if( other.vMin[2] < r.vMin[2] )
			r.vMin[2] = other.vMin[2];
		if( other.vMax[2] > r.vMax[2] )
			r.vMax[2] = other.vMax[2];
		return r;
	};

	_inline_ VWB_BOX operator+( VWB_VECTOR3<T> const& other ) const
	{
		VWB_BOX r( *this );
		if( other[0] < r.r.tl[0] )
			r.vMin[0] = other[0];
		if( other[0] > r.vMin[0] )
			r.vMin[0] = other[0];

		if( other[1] < r.vMin[1] )
			r.vMin[1] = other[1];
		if( other[1] > r.vMin[1] )
			r.vMin[1] = other[1];

		if( other[2] < r.vMin[2] )
			r.vMin[2] = other[2];
		if( other[2] > r.vMax[2] )
			r.vMax[2] = other[2];
		return r;
	};
	_inline_ bool IsEmpty() { return vMin.x < vMax.x && vMin.y < vMax.y && vMin.z < vMax.z; }
	_inline_ static VWB_BOX M() { return VWB_BOX( VWB_VECTOR3<T>( FLT_MAX,FLT_MAX,FLT_MAX ), VWB_VECTOR3<T>( -FLT_MAX,-FLT_MAX,-FLT_MAX ) ); }
	_inline_ static VWB_BOX O() { return VWB_BOX( VWB_VECTOR3<T>( 0,0,0 ), VWB_VECTOR3<T>( 0,0,0 ) ); }
};

template< class T, bool rh >
_inline_ void MakeSymmetric( VWB_MATRIX<T>& V, T( &clip )[6] )
{
	// calc reciprocal to avoid extra division
	T nearRec = T( 1 ) / clip[4];

	// angles
	T a[] = {
        std::atan( clip[0] * nearRec ),
        std::atan( clip[1] * nearRec ),
        std::atan( clip[2] * nearRec ),
        std::atan( clip[3] * nearRec ) };

	// normalized distance in x and y
    T dxn = std::tan( ( a[2] - a[0] ) / 2 );
    T dyn = std::tan( ( a[3] - a[1] ) / 2 );

	// new clip
	clip[0] = clip[2] = tan( ( a[0] + a[2] ) / 2 ) * clip[4];
	clip[1] = clip[3] = tan( ( a[1] + a[3] ) / 2 ) * clip[4];

	// add up clip differences in direction of base vectors
	VWB_VECTOR3<T> mx;
	VWB_VECTOR3<T> my;
	if( rh )
	{
		mx = VWB_VECTOR3<T>( V.X() ) + VWB_VECTOR3<T>( V.Z() ) * dxn;
		my = VWB_VECTOR3<T>( V.Y() ) - VWB_VECTOR3<T>( V.Z() ) * dyn;
	}
	else
	{
		mx = VWB_VECTOR3<T>( V.X() ) - VWB_VECTOR3<T>( V.Z() ) * dxn;
		my = VWB_VECTOR3<T>( V.Y() ) + VWB_VECTOR3<T>( V.Z() ) * dyn;
	}

	// create new base
	VWB_MATRIX33<T> R = VWB_MATRIX33<T>::Base( mx, my );

	// set new rotation
	V.SetUpper( R );

}

template< class T>
_inline_ void MakeSymmetricRH( VWB_MATRIX<T>& V, T( &clip )[6] )
{
	MakeSymmetric<T, true>( V, clip );
}

template< class T>
_inline_ void MakeSymmetricLH( VWB_MATRIX<T>& V, T( &clip )[6] )
{
	MakeSymmetric<T, false>( V, clip );
}

#pragma pack(pop)

typedef VWB_MATRIX33<VWB_float> VWB_MAT33f;
typedef VWB_VECTOR3<VWB_float> VWB_VEC3f;

typedef VWB_MATRIX<VWB_float> VWB_MAT44f;
typedef VWB_VECTOR4<VWB_float> VWB_VEC4f;

typedef VWB_BOX<VWB_float> VWB_BOXf;

typedef VWB_MATRIX33<VWB_double> VWB_MAT33d;
typedef VWB_VECTOR3<VWB_double> VWB_VEC3d;

typedef VWB_MATRIX<VWB_double> VWB_MAT44d;
typedef VWB_VECTOR4<VWB_double> VWB_VEC4d;

typedef VWB_BOX<VWB_double> VWB_BOXd;

#endif
