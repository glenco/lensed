OBJECT(sky) = FOREGROUND;

PARAMS(sky) = {
    { "bg" },
    { "dx" },
    { "dy" }
};

struct sky
{
    float bg;
    float2 grad;
};

static float sky(constant struct sky* sky, float2 x)
{
    return sky->bg + dot(sky->grad, x - (float2)(1, 1));
}

static void set_sky(global struct sky* sky, float bg, float dx, float dy)
{
    sky->bg = bg;
    sky->grad = (float2)(dx, dy);
}
