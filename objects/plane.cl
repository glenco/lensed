// simple lens plane

type = PLANE;

params
{
    { "f", SCALE }
};

data
{
    float f; // lens plane scaling
};

static float planescale(local data* this)
{
    // simple scaling of deflection
    return this->f;
}

static void set(local data* this, float f)
{
    // lens plane scaling
    this->f = f;
}
