type = FOREGROUND;

params
{
    { "bg" },
    { "dx", PARAMETER, UNBOUNDED, -0.0f },
    { "dy", PARAMETER, UNBOUNDED, -0.0f }
};

data
{
    float bg;
    float2 grad;
};

static float foreground(local data* this, float2 x)
{
    return this->bg + dot(this->grad, x - (float2)(1, 1));
}

static void set(local data* this, float bg, float dx, float dy)
{
    this->bg = bg;
    this->grad = (float2)(dx, dy);
}
