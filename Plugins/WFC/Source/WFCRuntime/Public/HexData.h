#pragma once

#include "CoreMinimal.h"
#include "Containers/Array.h"

struct FHexCoord
{
private:
	int32 q = 0;
	int32 r = 0;
	int32 s = 0;

public:
	FHexCoord() = default;
	int32 GetQ() const { return q; }
	int32 GetR() const { return r; }
	int32 GetS() const { return s; }

	FHexCoord(int32 inq, int32 inr, int32 ins)
	{
		q = inq;
		r = inr;
		s = ins;
	}

	static const FHexCoord Directions[6];

	static FHexCoord Direction(int32 direction)
	{
		checkf(direction >= 0 && direction < 6, TEXT("Invalid hex direction: %d"), direction);
		return Directions[direction];
	}

	FHexCoord Add(const FHexCoord& b) const
	{
		return FHexCoord(q + b.q, r + b.r, s + b.s);
	}

	FHexCoord Neighbor(int32 direction) const
	{
		return Add(Direction(direction));
	}

	FHexCoord Scale(int32 k) const
	{
		return FHexCoord(q * k, r * k, s * k);
	}

	static TArray<FHexCoord> FHexCoord_Ring(int32 radius)
	{
		TArray<FHexCoord> results;
		if (radius == 0)
		{
			results.Add(FHexCoord(0, 0, 0));
			return results;
		}

		FHexCoord hex = Direction(4).Scale(radius);

		for (int32 i = 0; i < 6; i++)
		{
			for (int32 j = 0; j < radius; j++)
			{
				results.Add(hex);
				hex = hex.Neighbor(i);
			}
		}

		return results;
	}

	// 生成半径为 Radius 的六边形网格（包含中心到 Ring(Radius) 的所有格子）
	static TArray<FHexCoord> FHexCoord_Spiral(int32 radius)
	{
		TArray<FHexCoord> results;
		results.Add(FHexCoord(0, 0, 0));
		for (int32 r = 1; r <= radius; r++)
		{
			results.Append(FHexCoord_Ring(r));
		}
		return results;
	}

	static TArray<FVector> BuildSpiralPoints(int32 radius, float hexSize)
	{
		TArray<FVector> vertices;
		vertices.Add(FVector::ZeroVector);

		for (int32 ringRadius = 1; ringRadius <= radius; ++ringRadius)
		{
			const TArray<FHexCoord> ring = FHexCoord_Ring(ringRadius);
			for (const FHexCoord& hex : ring)
			{
				vertices.Add(hex.ToWorldPosition(hexSize));
			}
		}

		return vertices;
	}

	static TArray<FVector> GrabRing(const TArray<FVector>& vertices, int32 radius)
	{
		TArray<FVector> result;

		if (radius < 0)
		{
			return result;
		}

		if (radius == 0)
		{
			if (vertices.Num() > 0)
			{
				result = vertices;
				result.SetNum(1);
			}
			return result;
		}

		const int32 startIndex = 1 + 3 * radius * (radius - 1);
		const int32 count = 6 * radius;

		if (!vertices.IsValidIndex(startIndex) || startIndex + count > vertices.Num())
		{
			return result;
		}

		result.Append(&vertices[startIndex], count);
		return result;
	}

	// cube 坐标转世界坐标（Flat-top 六边形）
	// hexSize = 六边形外接圆半径（中心到顶点的距离）
	// hexWidth = hexSize * 2
	// hexHeight = sqrt(3) * hexSize
	// Flat-top 世界坐标公式：
	//   x = hexSize * (3/2 * q)
	//   y = hexSize * (sqrt(3)/2 * q + sqrt(3) * r)
	FVector ToWorldPosition(float hexSize) const
	{
		float X = hexSize * 1.5f * q;
		float Sqrt3 = FMath::Sqrt(3.f);
		float Y = hexSize * (Sqrt3 * 0.5f * q + Sqrt3 * r);
		return FVector(X, Y, 0.0f);
	}
};

inline const FHexCoord FHexCoord::Directions[6] = {
	FHexCoord(1, 0, -1),
	FHexCoord(1, -1, 0),
	FHexCoord(0, -1, 1),
	FHexCoord(-1, 0, 1),
	FHexCoord(-1, 1, 0),
	FHexCoord(0, 1, -1)
};
