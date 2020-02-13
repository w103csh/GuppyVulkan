//////////////////////////////////////////////////////////////////////
// Modifications copyright(C) 2020 Colin Hughes<colin.s.hughes @gmail.com>
// -------------------------------
// Copyright (C) 2009 - Filip Strugar.
// Distributed under the zlib License (see readme.txt)
//////////////////////////////////////////////////////////////////////

#ifndef COMMON_H
#define COMMON_H

struct MapDimensions {
    float MinX;
    float MinY;
    float MinZ;
    float SizeX;
    float SizeY;
    float SizeZ;

    float MaxX() const { return MinX + SizeX; }
    float MaxY() const { return MinY + SizeY; }
    float MaxZ() const { return MinZ + SizeZ; }
};

#endif  // !COMMON_H