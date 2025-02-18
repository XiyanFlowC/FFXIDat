#pragma once

#ifndef XY_VEC__
#define XY_VEC__

namespace xybase
{
#pragma pack(push, 1)
	template <class ValT>
	struct Vec2
	{
		Vec2() : x(0), y(0) {}

		Vec2(ValT x, ValT y) : x(x), y(y) {}

		Vec2(const Vec2<ValT> &rhs) = default;

		Vec2(Vec2<ValT> &&) = default;

		ValT Module()
		{
			return sqrtf((float)x * x + y * y);
		}

		Vec2<ValT> ProjectOn(Vec2<ValT> axis)
		{
			float dot = (float)Dot(axis); // 计算a·b
			float axisSquared = (float)axis.Dot(axis); // 计算|b|²
			if (axisSquared == ValT(0)) { // 处理axis为零向量
				return Vec2<ValT>(); // 返回零向量或根据需求处理
			}
			return { (ValT)(axis.x * (dot / axisSquared)), (ValT)(axis.y * (dot / axisSquared)) }; // 正确投影公式
		}

		ValT Dot(const Vec2<ValT> &rhs) const
		{
			return x * rhs.x + y * rhs.y;
		}

		Vec2<ValT> operator + (const Vec2<ValT> &rhs)
		{
			return Vec2(x + rhs.x, y + rhs.y);
		}

		Vec2<ValT> operator - (const Vec2<ValT> &rhs)
		{
			return Vec2(x - rhs.x, y - rhs.y);
		}

		Vec2<ValT> operator * (ValT num)
		{
			return Vec2<ValT>((float)x * num, (float)y * num);
		}

		Vec2<ValT> operator / (ValT num)
		{
			return Vec2<ValT>((float)x / num, (float)y / num);
		}

		Vec2<ValT> operator - ()
		{
			return Vec2(-x, -y);
		}

		Vec2<ValT> &operator = (const Vec2<ValT> &rhs)
		{
			x = rhs.x;
			y = rhs.y;

			return *this;
		}


		template<class RValT>
		static Vec2<ValT> From(const Vec2<RValT> &rhs)
		{
			Vec2<ValT> ret;
			ret.x = (ValT)rhs.x;
			ret.y = (ValT)rhs.y;

			return ret;
		}

		ValT x, y;
	};
}
#pragma pack(pop)

#endif
