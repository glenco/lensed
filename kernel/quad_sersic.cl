OBJECT(quad_sersic) = SOURCE;

PARAMS(quad_sersic) = {
    { "x1" },
    { "y1" },
    { "r1" },
    { "mag1" },
    { "n1" },
    { "q1" },
    { "pa1", true },
    { "x2" },
    { "y2" },
    { "r2" },
    { "mag2" },
    { "n2" },
    { "q2" },
    { "pa2", true },
    { "x3" },
    { "y3" },
    { "r3" },
    { "mag3" },
    { "n3" },
    { "q3" },
    { "pa3", true },
    { "x4" },
    { "y4" },
    { "r4" },
    { "mag4" },
    { "n4" },
    { "q4" },
    { "pa4", true },
};

struct quad_sersic
{
    float2 x[4];   // source position
    mat22 t[4];    // coordinate transformation matrix
    float log0[4]; // profile constants
    float log1[4];
    float m[4];
};

static float quad_sersic(constant struct quad_sersic* data, float2 x)
{

    if( data->log0[0] < data->log0[1] || data->log0[1] < data->log0[2] || data->log0[2] < data->log0[3] ) return 0.0;

    float sum = 0;
    float2 y;
    for(int i=0 ; i< 4 ; ++i){
      y = mv22(data->t[i], x - data->x[i]);
      sum += exp(data->log0[i] - exp(data->log1[i] + data->m[i]*log(dot(y, y))));
    }
    return sum;
}

static void set_quad_sersic(global struct quad_sersic* data
       , float x1, float y1, float r1, float mag1, float n1, float q1, float pa1
       , float x2, float y2, float r2, float mag2, float n2, float q2, float pa2
       , float x3, float y3, float r3, float mag3, float n3, float q3, float pa3
       , float x4, float y4, float r4, float mag4, float n4, float q4, float pa4
){

        // brightest source
	int i = 0;
        float b = 1.9992f*n1 - 0.3271f; // approximation valid for 0.5 < n < 8
    
	float c = cos(pa1*DEG2RAD);
    	float s = sin(pa1*DEG2RAD);
    
        // source position
    	data->x[i] = (float2)(x1, y1);
    
        // transformation matrix: rotate and scale
   	data->t[i] = (mat22)(q1*c, q1*s, -s, c);
    
        data->log0[i] = -0.4f*mag1*LOG_10 + 2*n1*log(b) - LOG_PI - 2*log(r1) - log(tgamma(2*n1+1));
        data->log1[i] = log(b) - (0.5f*log(q1) + log(r1))/n1;
        data->m[i] = 0.5f/n1;

        // ********************************
	i = 1;
        b = 1.9992f*n2 - 0.3271f; // approximation valid for 0.5 < n < 8
	c = cos(pa2*DEG2RAD);
    	s = sin(pa2*DEG2RAD);
        // source position
    	data->x[i] = (float2)(x2, y2);
        // transformation matrix: rotate and scale
   	data->t[i] = (mat22)(q2*c, q2*s, -s, c);
        data->log0[i] = -0.4f*mag2*LOG_10 + 2*n2*log(b) - LOG_PI - 2*log(r2) - log(tgamma(2*n2+1));
        data->log1[i] = log(b) - (0.5f*log(q2) + log(r2))/n2;
        data->m[i] = 0.5f/n2;
    
       // ********************************
	i = 2;
        b = 1.9992f*n3 - 0.3271f; // approximation valid for 0.5 < n < 8
	c = cos(pa3*DEG2RAD);
    	s = sin(pa3*DEG2RAD);
        // source position
    	data->x[i] = (float2)(x3, y3);
        // transformation matrix: rotate and scale
   	data->t[i] = (mat22)(q3*c, q3*s, -s, c);
        data->log0[i] = -0.4f*mag3*LOG_10 + 2*n3*log(b) - LOG_PI - 2*log(r3) - log(tgamma(2*n3+1));
        data->log1[i] = log(b) - (0.5f*log(q3) + log(r3))/n3;
        data->m[i] = 0.5f/n3;
 
       // ********************************
	i = 3;
        b = 1.9992f*n4 - 0.3271f; // approximation valid for 0.5 < n < 8
	c = cos(pa4*DEG2RAD);
    	s = sin(pa4*DEG2RAD);
        // source position
    	data->x[i] = (float2)(x4, y4);
        // transformation matrix: rotate and scale
   	data->t[i] = (mat22)(q4*c, q4*s, -s, c);
        data->log0[i] = -0.4f*mag4*LOG_10 + 2*n4*log(b) - LOG_PI - 2*log(r4) - log(tgamma(2*n4+1));
        data->log1[i] = log(b) - (0.5f*log(q4) + log(r4))/n4;
        data->m[i] = 0.5f/n4;
   
}
