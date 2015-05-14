type = FOREGROUND;

params
{
    { "bg" },
    { "dx" },
    { "dy" }
};

data
{
    float bg;
    float2 grad;
};

static float foreground(constant data* this, float2 x)
{
    return this->bg + dot(this->grad, x - (float2)(1, 1));
}

static void set(global data* this, float bg, float dx, float dy)
{
    this->bg = bg;
    this->grad = (float2)(dx, dy);
}
