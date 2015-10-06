// 2x2 subsampling (without error estimate)

const double QUAD_SUB2_PTS[][2] = {
    {-0.25, -0.25},
    {-0.25,  0.25},
    { 0.25, -0.25},
    { 0.25,  0.25},
};

const double QUAD_SUB2_WHT[] = {
    0.25,
    0.25,
    0.25,
    0.25
};

const double QUAD_SUB2_ERR[] = {
    0,
    0,
    0,
    0
};
