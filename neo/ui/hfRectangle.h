#ifndef IDRECTANGLE_H_
#define IDRECTANGLE_H_
//
// simple rectangle
//
extern void RotateVector(hfVec3 &v, hfVec3 origin, float a, float c, float s);
class hfRectangle {
public:
	float x;    // horiz position
	float y;    // vert position
	float w;    // width
	float h;    // height;
	hfRectangle() { x = y = w= h = 0.0; }
	hfRectangle(float ix, float iy, float iw, float ih) { x = ix; y = iy; w = iw; h = ih; }
	float Bottom() const { return y + h; }
	float Right() const { return x + w; }
	void Offset (float x, float y) { 
		this->x += x;
		this->y += y;
	}
	bool Contains(float xt, float yt) {
		if (w == 0.0 && h == 0.0) {
			return false;
		}
		if (xt >= x && xt <= Right() && yt >= y && yt <= Bottom()) {
			return true;
		}
		return false;
	}
	void Empty() { x = y = w = h = 0.0; };

	void ClipAgainst(hfRectangle r, bool sizeOnly) {
		if (!sizeOnly) {
			if (x < r.x) {
				w -= r.x - x;
				x = r.x;
			}
			if (y < r.y) {
				h -= r.y - y;
				y = r.y;
			}
		}
		if (x + w > r.x + r.w) {
			w = (r.x + r.w) - x;
		}
		if (y + h > r.y + r.h) {
			h = (r.y + r.h) - y;
		}
	}



	void Rotate(float a, hfRectangle &out) {
		hfVec3 p1, p2, p3, p4, p5;
		float c, s;
		hfVec3 center;
		center.Set((x + w) / 2.0, (y + h) / 2.0, 0);
		p1.Set(x, y, 0);
		p2.Set(Right(), y, 0);
		p4.Set(x, Bottom(), 0);
		if (a) {
			s = sin( DEG2RAD( a ) );
			c = cos( DEG2RAD( a ) );
		}
		else {
			s = c = 0;
		}
		RotateVector(p1, center, a, c, s);
		RotateVector(p2, center, a, c, s);
		RotateVector(p4, center, a, c, s);
		out.x = p1.x;
		out.y = p1.y;
		out.w = (p2 - p1).Length();
		out.h = (p4 - p1).Length();
	}

	hfRectangle & operator+=( const hfRectangle &a );
	hfRectangle & operator-=( const hfRectangle &a );
	hfRectangle & operator/=( const hfRectangle &a );
	hfRectangle & operator/=( const float a );
	hfRectangle & operator*=( const float a );
	hfRectangle & operator=( const hfVec4 v );
	int operator==(const hfRectangle &a) const;
	float &	operator[]( const int index );
	char * String() const;
	const hfVec4& ToVec4() const;

};

HF_INLINE const hfVec4 &hfRectangle::ToVec4() const {
	return *reinterpret_cast<const hfVec4 *>(&x);
}


HF_INLINE hfRectangle &hfRectangle::operator+=( const hfRectangle &a ) {
	x += a.x;
	y += a.y;
	w += a.w;
	h += a.h;

	return *this;
}

HF_INLINE hfRectangle &hfRectangle::operator/=( const hfRectangle &a ) {
	x /= a.x;
	y /= a.y;
	w /= a.w;
	h /= a.h;

	return *this;
}

HF_INLINE hfRectangle &hfRectangle::operator/=( const float a ) {
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	w *= inva;
	h *= inva;

	return *this;
}

HF_INLINE hfRectangle &hfRectangle::operator-=( const hfRectangle &a ) {
	x -= a.x;
	y -= a.y;
	w -= a.w;
	h -= a.h;

	return *this;
}

HF_INLINE hfRectangle &hfRectangle::operator*=( const float a ) {
	x *= a;
	y *= a;
	w *= a;
	h *= a;

	return *this;
}


HF_INLINE hfRectangle &hfRectangle::operator=( const hfVec4 v ) {
	x = v.x;
	y = v.y;
	w = v.z;
	h = v.w;
	return *this;
}

HF_INLINE int hfRectangle::operator==( const hfRectangle &a ) const {
	return (x == a.x && y == a.y && w == a.w && a.h);
}

HF_INLINE float& hfRectangle::operator[]( int index ) {
	return ( &x )[ index ];
}

class hfRegion {
public:
	hfRegion() { };

	void Empty() {
		rects.Clear();
	}

	bool Contains(float xt, float yt) {
		int c = rects.Num();
		for (int i = 0; i < c; i++) {
			if (rects[i].Contains(xt, yt)) {
				return true;
			}
		}
		return false;
	}

	void AddRect(float x, float y, float w, float h) {
		rects.Append(hfRectangle(x, y, w, h));
	}

	int GetRectCount() {
		return rects.Num();
	}

	hfRectangle *GetRect(int index) {
		if (index >= 0 && index < rects.Num()) {
			return &rects[index];
		}
		return NULL;
	}

protected:

	hfList<hfRectangle, TAG_OLD_UI> rects;
};


#endif

